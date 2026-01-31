constexpr static const char* TASKS_CPP_COMMS_TAG = "TASK_COMMS.CPP";
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

#include "config.h"
#include "types.h"
#include "main.h"

extern QueueHandle_t dataQueue;
extern EventGroupHandle_t syncBits;

// --- MQTT command context (owned by taskComms) ---
struct MqttCmdContext {
    CmdMessage latestCmd;
    volatile bool hasLatestCmd;
};

static MqttCmdContext* g_cmdContext = nullptr; // set by taskComms when running

// --- Minimal JSON parsing helpers for CmdMessage ---
static bool parseBoolValue(const String &src, int valueStart, bool &out) {
    // Skip whitespace
    while (valueStart < (int)src.length() &&
           (src[valueStart] == ' ' || src[valueStart] == '\t' ||
            src[valueStart] == '\n' || src[valueStart] == '\r')) {
        ++valueStart;
    }

    if (valueStart >= (int)src.length()) {
        return false;
    }

    if (src.startsWith("true", valueStart)) {
        out = true;
        return true;
    }
    if (src.startsWith("false", valueStart)) {
        out = false;
        return true;
    }

    // Also accept numeric 1/0 just in case
    if (src[valueStart] == '1') {
        out = true;
        return true;
    }
    if (src[valueStart] == '0') {
        out = false;
        return true;
    }

    return false;
}

static bool parseBoolField(const String &json, const char *key, bool &out) {
    String pattern = String("\"") + key + "\"";
    int keyPos = json.indexOf(pattern);
    if (keyPos < 0) {
        return false; // field not present
    }

    int colonPos = json.indexOf(':', keyPos + pattern.length());
    if (colonPos < 0) {
        return false;
    }

    return parseBoolValue(json, colonPos + 1, out);
}

static bool parseCmdMessageJson(const String &json, CmdMessage &cmdOut) {
    CmdMessage tmp{};
    bool anyField = false;

    bool value = false;
    if (parseBoolField(json, "CmdPhotoReq", value)) {
        tmp.CmdPhotoReq = value;
        anyField = true;
    }
    if (parseBoolField(json, "CmdModeEco", value)) {
        tmp.CmdModeEco = value;
        anyField = true;
    }
    if (parseBoolField(json, "CmdStop", value)) {
        tmp.CmdStop = value;
        anyField = true;
    }

    if (!anyField) {
        return false;
    }

    cmdOut = tmp;
    return true;
}

// --- MQTT incoming message callback ---
static void mqttCallback(char *topic, byte *payload, unsigned int length) {
    String message;
    message.reserve(length + 1);
    for (unsigned int i = 0; i < length; ++i) {
        message += static_cast<char>(payload[i]);
    }

    ESP_LOGI(TASKS_CPP_COMMS_TAG,
             "Messaggio MQTT arrivato [%s]: %s",
             topic,
             message.c_str());

    if (g_cmdContext == nullptr) {
        ESP_LOGW(TASKS_CPP_COMMS_TAG,
                 "CmdContext non inizializzato, messaggio scartato");
        return;
    }

    CmdMessage parsed{};
    if (parseCmdMessageJson(message, parsed)) {
        g_cmdContext->latestCmd = parsed;
        g_cmdContext->hasLatestCmd = true;

        ESP_LOGI(TASKS_CPP_COMMS_TAG,
                 "CmdMessage aggiornato -> PhotoReq=%d ModeEco=%d Stop=%d",
                 static_cast<int>(parsed.CmdPhotoReq),
                 static_cast<int>(parsed.CmdModeEco),
                 static_cast<int>(parsed.CmdStop));
    } else {
        ESP_LOGW(TASKS_CPP_COMMS_TAG,
                 "JSON ricevuto ma non valido per CmdMessage: %s",
                 message.c_str());
    }
}

static bool ensureWifiConnected() {
    if (WiFi.status() == WL_CONNECTED) {
        return true;
    }

    ESP_LOGI(TASKS_CPP_COMMS_TAG, "Connessione WiFi...");
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    const unsigned long start = millis();
    const unsigned long timeoutMs = 15000; // 15s max
    while (WiFi.status() != WL_CONNECTED && (millis() - start) < timeoutMs) {
        delay(500);
    }

    if (WiFi.status() == WL_CONNECTED) {
        ESP_LOGI(TASKS_CPP_COMMS_TAG, "WiFi connesso.");
        return true;
    }

    ESP_LOGE(TASKS_CPP_COMMS_TAG,
             "ERRORE: impossibile connettersi al WiFi.");
    return false;
}

static bool ensureMqttConnected(WiFiClientSecure &espClient, PubSubClient &mqttClient) {
    espClient.setInsecure(); // per ora nessun controllo certificato server
    mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
    mqttClient.setCallback(mqttCallback);

    if (mqttClient.connected()) {
        return true;
    }

    const int maxAttempts = 3;
    int attempt = 0;
    while (!mqttClient.connected() && attempt < maxAttempts) {
        ++attempt;
        ESP_LOGI(TASKS_CPP_COMMS_TAG,
                 "Tentativo connessione MQTT #%d",
                 attempt);

        String clientId = String("ESP32-SERRAPP-") + String(random(0xffff), HEX);

        if (mqttClient.connect(clientId.c_str(), MQTT_USER, MQTT_PASS)) {
            ESP_LOGI(TASKS_CPP_COMMS_TAG, "MQTT connesso.");
            if (TOPIC_CMD[0] != '\0') {
                mqttClient.subscribe(TOPIC_CMD);
                ESP_LOGI(TASKS_CPP_COMMS_TAG,
                         "Iscritto al topic comandi: %s",
                         TOPIC_CMD);
            }
            return true;
        } else {
            ESP_LOGE(TASKS_CPP_COMMS_TAG,
                     "Connessione MQTT fallita, rc=%d",
                     mqttClient.state());
            delay(2000);
        }
    }

    ESP_LOGE(TASKS_CPP_COMMS_TAG,
             "ERRORE: impossibile connettersi a MQTT.");
    return false;
}

// --- TASK B: COMMUNICATIONS (Core 0) ---
void taskComms(void *pvParameters) {
    WiFiClientSecure espClient;
    PubSubClient mqttClient(espClient);

    static MqttCmdContext cmdContext{};
    cmdContext.hasLatestCmd = false;
    g_cmdContext = &cmdContext;

    GardenMessage rcv{};

    // 1. Connessione WiFi
    if (!ensureWifiConnected()) {
        ESP_LOGE(TASKS_CPP_COMMS_TAG,
                 "Abort: WiFi non disponibile.");
        xEventGroupSetBits(syncBits, BIT_COMMS_DONE);
        vTaskDelete(nullptr);
        return;
    }

    // 2. Connessione MQTT e subscribe al topic comandi (TOPIC_CMD)
    if (!ensureMqttConnected(espClient, mqttClient)) {
        ESP_LOGE(TASKS_CPP_COMMS_TAG,
                 "Abort: MQTT non disponibile.");
        xEventGroupSetBits(syncBits, BIT_COMMS_DONE);
        vTaskDelete(nullptr);
        return;
    }

    // 3. (Opzionale) Attesa dati da task Mango per eventuale publish
    if (dataQueue != nullptr &&
        xQueueReceive(dataQueue, &rcv, pdMS_TO_TICKS(1000)) == pdPASS) {
        ESP_LOGI(TASKS_CPP_COMMS_TAG,
                 "GardenMessage ricevuto dalla coda (publish non ancora implementato).");
        // Qui in futuro potrai serializzare rcv in JSON e pubblicarlo su TOPIC_PUB.
    } else {
        ESP_LOGI(TASKS_CPP_COMMS_TAG,
                 "Nessun GardenMessage ricevuto (timeout coda).");
    }

    // 4. Mantieni la connessione MQTT viva per un po' per ricevere i comandi JSON
    const unsigned long listenWindowMs = 10000; // ascolta per 10 secondi
    unsigned long start = millis();
    while (millis() - start < listenWindowMs) {
        if (!mqttClient.connected()) {
            // Prova a riconnettere rapidamente; se fallisce, esci dal loop
            if (!ensureMqttConnected(espClient, mqttClient)) {
                break;
            }
        }
        mqttClient.loop();
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    if (cmdContext.hasLatestCmd) {
        ESP_LOGI(TASKS_CPP_COMMS_TAG,
                 "Ultimo CmdMessage ricevuto: PhotoReq=%d ModeEco=%d Stop=%d",
                 static_cast<int>(cmdContext.latestCmd.CmdPhotoReq),
                 static_cast<int>(cmdContext.latestCmd.CmdModeEco),
                 static_cast<int>(cmdContext.latestCmd.CmdStop));
    } else {
        ESP_LOGI(TASKS_CPP_COMMS_TAG,
                 "Nessun CmdMessage JSON ricevuto nella finestra di ascolto.");
    }

    // 5. Segnala al setup() che le comunicazioni sono finite (anche se senza comandi)
    xEventGroupSetBits(syncBits, BIT_COMMS_DONE);
    vTaskDelete(nullptr);
}