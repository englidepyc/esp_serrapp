constexpr static const char* TASKS_CPP_COMMS_TAG = "TASK_COMMS.CPP";
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#include "config.h"
#include "types.h"
#include "main.h"
#include "taskComms.h"

extern EventGroupHandle_t syncBits;

QueueHandle_t dataQueue = nullptr;
volatile bool live_mode = false;

RTC_DATA_ATTR CmdMessage g_rtcCmdMessage; //comandi persistenti fra deep sleep anche se non si connette

// --- MQTT command context (owned by taskComms) ---
struct MqttCmdContext {
    CmdMessage latestCmd;
    volatile bool hasLatestCmd;
};

static MqttCmdContext* g_cmdContext = nullptr; // set by taskComms when running, condiviso da taskComms e callback
// --- Prototipi funzioni locali ---
static bool parseCmdMessageJson(const String &json, CmdMessage &cmdOut);
static String buildGardenMessageJson(const GardenMessage &dataMsg);

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
            cmdContext.hasLatestCmd = false;

            ESP_LOGI(TASKS_CPP_COMMS_TAG,
                     "Nuovo comando applicato e salvato in RTC -> PhotoReq=%d ModePrecision=%d Stop=%d",
                     static_cast<int>(effectiveCmd.CmdPhotoReq),
                     static_cast<int>(effectiveCmd.CmdModePrecision),
                     static_cast<int>(effectiveCmd.CmdStop));
        } else {
            ESP_LOGI(TASKS_CPP_COMMS_TAG,
                     "Nessun nuovo comando MQTT: uso il comando in RTC -> PhotoReq=%d ModePrecision=%d Stop=%d",
                     static_cast<int>(effectiveCmd.CmdPhotoReq),
                     static_cast<int>(effectiveCmd.CmdModePrecision),
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
        } else if (effectiveCmd.CmdModePrecision == true) {
            
            
            BaseType_t created = xTaskCreatePinnedToCore(
                taskMango,
                "Mango",
                8192,
                (void*)0, // usa pvParameters come semplice flag eco/non-eco
                1,
                &handleMango,
                1
            );

            if (created != pdPASS) {
                ESP_LOGE(TASKS_CPP_COMMS_TAG,
                         "Errore nella creazione di taskMango (Precision mode)");
            }else{
            //*** PRECISION MODE ***
            GardenMessage dataMsg{};
            while (1) {    
                ESP_LOGI(TASKS_CPP_COMMS_TAG,
                         "Modalità precision attivata vado in loop, TODO IMPLEMENTARE...");

                
                if (/*Task mango ha mandato (prodotto) il GardenMessage perché sono passati 10 minuti*/
                    xQueueReceive(dataQueue, &dataMsg, pdMS_TO_TICKS(100)) == pdTRUE) {

                    // Costruzione JSON dei dati letti da Mango (riuso di buildGardenMessageJson)
                    String payload = buildGardenMessageJson(dataMsg);

                    if (!mqttClient.connected()) {
                        // Prova a riconnettere rapidamente; se fallisce, salta questo giro
                        if (!ensureMqttConnected(espClient, mqttClient)) {
                            ESP_LOGE(TASKS_CPP_COMMS_TAG,
                                     "MQTT disconnesso durante publish dati Mango (precision), skip.");
                            continue;
                        }
                    }

                    bool pubOk = mqttClient.publish(TOPIC_PUB, payload.c_str(), true);

                    if (!pubOk) {
                        ESP_LOGE(TASKS_CPP_COMMS_TAG,
                                 "Errore publish GardenMessage (precision, isActuated=%d)",
                                 static_cast<int>(dataMsg.isActuated));
                    } else {
                        if (!dataMsg.isActuated) {
                            ESP_LOGI(TASKS_CPP_COMMS_TAG,
                                     "[PRECISION] Dati PRIMA irrigazione pubblicati su MQTT");
                        } else {
                            ESP_LOGI(TASKS_CPP_COMMS_TAG,
                                     "[PRECISION] Dati DOPO irrigazione pubblicati su MQTT");
                        }
                    }
                }
                
                
                // Mantieni la connessione MQTT viva per un po' per ricevere i comandi JSON
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

                if(cmdContext.hasLatestCmd == true){
                    
                    /*il messaggio è CmdPhotoReq = 1, CmdModePrecision = 1 CmdStop = 0*/
                    if (cmdContext.latestCmd.CmdModePrecision && !cmdContext.latestCmd.CmdStop && cmdContext.latestCmd.CmdPhotoReq){
                        live_mode = true;
                        
                while(cmdContext.hasLatestCmd == true){
                    /***LIVE MODE***/
                    /*//TODO Regolare il tempo per essere compatibile col PID*/
                    cmdContext.hasLatestCmd = false;

                    xQueueReceive(dataQueue, &dataMsg, pdMS_TO_TICKS(100));

                    // Costruzione JSON dei dati letti da Mango (riuso di buildGardenMessageJson)
                    String payload = buildGardenMessageJson(dataMsg);

                    if (!mqttClient.connected()) {
                        // Prova a riconnettere rapidamente; se fallisce, salta questo giro
                        if (!ensureMqttConnected(espClient, mqttClient)) {
                            ESP_LOGE(TASKS_CPP_COMMS_TAG,
                                     "MQTT disconnesso durante publish dati Mango (precision), skip.");
                            continue;
                        }
                    }

                    bool pubOk = mqttClient.publish(TOPIC_PUB, payload.c_str(), true);

                    // Mantieni la connessione MQTT viva per un po' per ricevere i comandi JSON
                    const unsigned long listenWindowMs = 5000; // ascolta per 5 secondi, calibrare, potrebbe esserci lag dal telefono
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
                
                
                
                }// FINE LIVE MODE

                       
            }

                    }
                    
                    //non è else così funziona anche se siamo usciti dal precedente if perché è cambiato il messaggio
                    if(/*C'è un NUOVO messaggio che ha CmdModePrecision = 0 e CmdStop = 0*/){

                        //LOGGA "Passo a modalità eco e vado in loop", vai in loop con TaskDelay e implementeremo dopo

                    }

                    
                    if(/*C'è un NUOVO messaggio che ha e CmdStop = 1*/){

                        //LOGGA "Stop e vado in loop", vai in loop con TaskDelay e implementeremo dopo

                    }


                    //Arrivati qui significa che non sono ricevuti ulteriori nuovi messaggi, possiamo settare live_mode falsa e ritornare al while principale

                    


                }

                
                         








                    vTaskDelay(pdMS_TO_TICKS(1000));
                }
            }
            
            
            
            
        } else if (effectiveCmd.CmdModePrecision == false){
            //*****MODE ECO ******/
            ESP_LOGI(TASKS_CPP_COMMS_TAG,
                     "Modalità eco attivata, avvio taskMango.");

            BaseType_t created = xTaskCreatePinnedToCore(
                taskMango,
                "Mango",
                8192,
                (void*)1, // usa pvParameters come semplice flag eco/non-eco
                1,
                &handleMango,
                1
            );

            if (created != pdPASS) {
                ESP_LOGE(TASKS_CPP_COMMS_TAG,
                         "Errore nella creazione di taskMango (Eco mode)");
            
            }else{
                
                // Due cicli prima e dopo (GardenMessage + eventuale CameraFrame)
                for (int i = 0; i < 2; ++i) {
                        GardenMessage dataMsg{};
                        if (xQueueReceive(dataQueue, &dataMsg, pdMS_TO_TICKS(180000)) != pdTRUE) {
                            ESP_LOGE(TASKS_CPP_COMMS_TAG,
                                    "Timeout in attesa del GardenMessage %d da Mango", i);
                            break;
                        }

                        // Per ora tralasciamo la foto, ma lasciamo l'hook
                        if (effectiveCmd.CmdPhotoReq) {
                            // CameraFrame frame{}; // TODO: gestire cattura e upload foto
                        }

                        // Costruzione JSON dei dati letti da Mango
                        String payload = buildGardenMessageJson(dataMsg);

                        if (!mqttClient.connected()) {
                            // Prova a riconnettere rapidamente; se fallisce, esci dal loop
                            if (!ensureMqttConnected(espClient, mqttClient)) {
                                ESP_LOGE(TASKS_CPP_COMMS_TAG,
                                         "MQTT disconnesso durante publish dati Mango, abort.");
                                break;
                            }
                        }

                        bool pubOk = mqttClient.publish(TOPIC_PUB, payload.c_str(), true);

                        if (!pubOk) {
                            ESP_LOGE(TASKS_CPP_COMMS_TAG,
                                     "Errore publish GardenMessage (isActuated=%d)",
                                     static_cast<int>(dataMsg.isActuated));
                        } else {
                            if (!dataMsg.isActuated) {
                                ESP_LOGI(TASKS_CPP_COMMS_TAG,
                                         "Dati PRIMA irrigazione pubblicati su MQTT");
                            } else {
                                ESP_LOGI(TASKS_CPP_COMMS_TAG,
                                         "Dati DOPO irrigazione pubblicati su MQTT");
                            }
                        }
                }

                
            }/*CREAZIONE MANGO TASK SUCCESSFUL */
        
        
        }//*****MODE ECO ****/

    // 5. Segnala al setup() che le comunicazioni sono finite (anche se senza comandi)
    xEventGroupSetBits(syncBits, BIT_COMMS_DONE);
    vTaskDelete(nullptr);
}

static bool parseCmdMessageJson(const String &json, CmdMessage &cmdOut) {
    CmdMessage tmp{};
    bool anyField = false;

    StaticJsonDocument<256> doc;
    DeserializationError err = deserializeJson(doc, json);
    if (err) {
        ESP_LOGW(TASKS_CPP_COMMS_TAG,
                 "deserializeJson fallita: %s",
                 err.c_str());
        return false;
    }

    JsonObject obj = doc.as<JsonObject>();

    if (obj.containsKey("CmdPhotoReq")) {
        tmp.CmdPhotoReq = obj["CmdPhotoReq"].as<bool>();
        anyField = true;
    }
    if (obj.containsKey("CmdModePrecision")) {
        tmp.CmdModePrecision = obj["CmdModePrecision"].as<bool>();
        anyField = true;
    }
    if (obj.containsKey("CmdStop")) {
        tmp.CmdStop = obj["CmdStop"].as<bool>();
        anyField = true;
    }

    if (!anyField) {
        return false;
    }

    cmdOut = tmp;
    return true;
}

// Costruisce il JSON MQTT a partire da un GardenMessage
static String buildGardenMessageJson(const GardenMessage &dataMsg) {
    StaticJsonDocument<256> doc;
    JsonObject obj = doc.to<JsonObject>();

    obj["temp"] = dataMsg.temp;
    obj["hum"] = dataMsg.hum;
    obj["vpd"] = dataMsg.vpd;
    obj["moisture"] = dataMsg.moisture;
    obj["waterCalcLevel"] = dataMsg.waterCalcLevel;
    obj["isWaterOver"] = dataMsg.isWaterOver;
    obj["nextMessageTime"] = dataMsg.nextMessageTime;
    obj["isActuated"] = dataMsg.isActuated;
    obj["errCode"] = dataMsg.errCode;
    obj["isFire"] = dataMsg.isFire;

    String payload;
    serializeJson(doc, payload);
    return payload;
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

/*     if (g_cmdContext == nullptr) {
        ESP_LOGW(TASKS_CPP_COMMS_TAG,
                 "CmdContext non inizializzato, messaggio scartato");
        return;
    } */

    CmdMessage parsed{};
    if (parseCmdMessageJson(message, parsed)) {
        g_cmdContext->latestCmd = parsed;
        g_cmdContext->hasLatestCmd = true;

        ESP_LOGI(TASKS_CPP_COMMS_TAG,
             "CmdMessage aggiornato -> PhotoReq=%d ModePrecision=%d Stop=%d",
             static_cast<int>(parsed.CmdPhotoReq),
             static_cast<int>(parsed.CmdModePrecision),
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