constexpr static const char* TASKS_CPP_COMMS_TAG = "TASK_COMMS.CPP";
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

#include "config.h"
#include "types.h"
#include "main.h"
#include "taskComms.h"

extern EventGroupHandle_t syncBits;
QueueHandle_t dataQueue = nullptr;
QueueHandle_t frameQueue = nullptr;

// --- MQTT command context (owned by taskComms) ---
struct MqttCmdContext {
    CmdMessage latestCmd;
    volatile bool hasLatestCmd;
};

static MqttCmdContext* g_cmdContext = nullptr; // set by taskComms when running
// --- Prototipi funzioni locali ---
static bool parseBoolValue(const String &src, int valueStart, bool &out);
static bool parseBoolField(const String &json, const char *key, bool &out);
static bool parseCmdMessageJson(const String &json, CmdMessage &cmdOut);
static void mqttCallback(char *topic, byte *payload, unsigned int length);
static bool ensureWifiConnected();
static bool ensureMqttConnected(WiFiClientSecure &espClient, PubSubClient &mqttClient);

// --- TASK B: COMMUNICATIONS (Core 0) ---
void taskComms(void *pvParameters) {
    WiFiClientSecure espClient;
    PubSubClient mqttClient(espClient);

    static MqttCmdContext cmdContext{};
    cmdContext.hasLatestCmd = false;
    g_cmdContext = &cmdContext;

    // Crea le code al primo avvio (se non ancora create)
    if (dataQueue == nullptr) {
        dataQueue = xQueueCreate(2, sizeof(GardenMessage));
    }
    if (frameQueue == nullptr) {
        frameQueue = xQueueCreate(2, sizeof(CameraFrame));
    }

        if (!ensureWifiConnected()) {
            ESP_LOGE(TASKS_CPP_COMMS_TAG,
                     "Abort: WiFi non disponibile. Proseguo a default");
            xEventGroupSetBits(syncBits, BIT_COMMS_DONE);
            vTaskDelete(nullptr);
        }

        if (!ensureMqttConnected(espClient, mqttClient)) {
            ESP_LOGE(TASKS_CPP_COMMS_TAG,
                     "Abort: MQTT non disponibile. Proseguo a default");
            xEventGroupSetBits(syncBits, BIT_COMMS_DONE);
            vTaskDelete(nullptr);
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


        // 4b. Scegli il comando effettivo: nuovo da MQTT o ultimo salvato in RTC
        CmdMessage effectiveCmd = g_rtcCmdMessage; // default: ultimo comando noto (o default iniziale)

        if (cmdContext.hasLatestCmd) {
            // Nuovo comando ricevuto: aggiorna anche la copia persistente in RTC
            g_rtcCmdMessage = cmdContext.latestCmd;
            effectiveCmd    = cmdContext.latestCmd;

            ESP_LOGI(TASKS_CPP_COMMS_TAG,
                     "Nuovo comando applicato e salvato in RTC -> PhotoReq=%d ModeEco=%d Stop=%d",
                     static_cast<int>(effectiveCmd.CmdPhotoReq),
                     static_cast<int>(effectiveCmd.CmdModeEco),
                     static_cast<int>(effectiveCmd.CmdStop));
        } else {
            ESP_LOGI(TASKS_CPP_COMMS_TAG,
                     "Nessun nuovo comando MQTT: uso il comando in RTC -> PhotoReq=%d ModeEco=%d Stop=%d",
                     static_cast<int>(effectiveCmd.CmdPhotoReq),
                     static_cast<int>(effectiveCmd.CmdModeEco),
                     static_cast<int>(effectiveCmd.CmdStop));
        }

        //**** Gestione del comando effettivo
        
        if (effectiveCmd.CmdStop) {
          //*** STOP DETECTED */
            while (1) {
                ESP_LOGI(TASKS_CPP_COMMS_TAG,
                         "Comando di stop ricevuto, serra sospesa in loop...");
                vTaskDelay(pdMS_TO_TICKS(1000));
            }
        } else if (effectiveCmd.CmdModeEco == false) {
            while (1) {
                //TODO: *** PRECISION
                ESP_LOGI(TASKS_CPP_COMMS_TAG,
                         "Modalità precision attivata vado in loop, TODO IMPLEMENTARE...");
                vTaskDelay(pdMS_TO_TICKS(1000));
            }
        } else if (effectiveCmd.CmdModeEco == true){
            //*****MODE ECO ******/
            ESP_LOGI(TASKS_CPP_COMMS_TAG,
                     "Modalità eco attivata, avvio taskMango.");

            MangoTaskConfig *mangoCfg = new MangoTaskConfig;
            mangoCfg->doPhoto = effectiveCmd.CmdPhotoReq;
            mangoCfg->isEcoMode = true;

            BaseType_t created = xTaskCreatePinnedToCore(
                taskMango,
                "Mango",
                8192,
                mangoCfg,
                1,
                &handleMango,
                1
            );

            if (created != pdPASS) {
                ESP_LOGE(TASKS_CPP_COMMS_TAG,
                         "Errore nella creazione di taskMango (Eco mode)");
                delete mangoCfg;
            
            }else{
                
                // Due cicli prima e dopo (GardenMessage + eventuale CameraFrame)
                for (int i = 0; i < 2; ++i) {
                        GardenMessage dataMsg{};
                        if (xQueueReceive(dataQueue, &dataMsg, pdMS_TO_TICKS(5000)) != pdTRUE) {
                            ESP_LOGE(TASKS_CPP_COMMS_TAG,
                                    "Timeout in attesa del GardenMessage %d da Mango", i);
                            break;
                        }
                        
                        if(effectiveCmd.CmdPhotoReq){
                            CameraFrame  frame{};
                            if (xQueueReceive(frameQueue, &frame, pdMS_TO_TICKS(5000)) != pdTRUE) {
                            ESP_LOGE(TASKS_CPP_COMMS_TAG,
                                    "Timeout in attesa del frame %d da Mango", i);
                            break;
                            }
                        }
                        
                        //TODO: Costruire mqtt in base a isActuated
                        if (!dataMsg.isActuated) {
                            ESP_LOGI(TASKS_CPP_COMMS_TAG,
                                    "Ricevuta foto da mango PRIMA irrigazione (TODO: upload Firebase)");
                        } else {
                            ESP_LOGI(TASKS_CPP_COMMS_TAG,
                                    "Ricevuta foto da mango DOPO irrigazione (TODO: upload Firebase)");
                        }

                        // TODO: Inviare MQTT ed (eventuale) foto
                }

                
            }/*CREAZIONE MANGO TASK SUCCESSFUL */
        
        
        }//*****MODE ECO ****/

    // 5. Segnala al setup() che le comunicazioni sono finite (anche se senza comandi)
    xEventGroupSetBits(syncBits, BIT_COMMS_DONE);
    vTaskDelete(nullptr);
}

// --- Implementazione funzioni ausiliarie ---

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

        String clientId = String("ESP32-SERRAPP");

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