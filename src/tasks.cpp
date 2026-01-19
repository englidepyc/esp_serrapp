#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include "config.h"
#include "types.h"
static const char* TASKS_CPP_MANGO_TAG = "TASKS.CPP/TASK_MANGO";
static const char* TASKS_CPP_COMMS_TAG = "TASKS.CPP/TASK_COMMS";

extern QueueHandle_t dataQueue;
extern EventGroupHandle_t syncBits;
#define BIT_COMMS_DONE (1 << 0)

// --- TASK A: GARDEN LOGIC (Core 1) ---
void taskMango(void *pvParameters) {
    GardenMessage msg{};

    // Simulated "normal" sensing activity for ~10 seconds
    uint32_t startMs = millis();
    while (millis() - startMs < 10000UL) {
        // Populate message with pseudo-random test data
        msg.temp       = random(150, 300) / 10.0f;  // 15.0 - 29.9 C
        msg.hum        = random(300, 900) / 10.0f;  // 30.0 - 89.9 %
        msg.moisture   = random(300, 800);
        msg.waterOk    = random(0, 2) == 1;
        msg.battery    = random(120, 150) / 10.0f;  // 12.0 - 14.9 V
        msg.actState   = static_cast<uint8_t>(random(0, 256));
        msg.errCode    = 0;
        msg.rst        = "TEST";
        msg.cmdPhotoReq = false;

        ESP_LOGI(TASKS_CPP_MANGO_TAG, "Sim data: T=%.1f H=%.1f M=%d", msg.temp, msg.hum, msg.moisture);

        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    // Simulated actuator work: e.g., heater ON for 30 seconds (no real pins)
    ESP_LOGI(TASKS_CPP_MANGO_TAG, "[SIM] Heater ON for 30s (no GPIO)");
    uint32_t heaterStart = millis();
    while (millis() - heaterStart < 30000UL) {
        uint32_t elapsed = (millis() - heaterStart) / 1000UL;
        ESP_LOGI(TASKS_CPP_MANGO_TAG, "[SIM] Heater running... %lus / 30s", (unsigned long)elapsed);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    ESP_LOGI(TASKS_CPP_MANGO_TAG, "[SIM] Heater OFF, Mango task finishing.");
    
    // End of simulation: just finish the task, fire ISR/hardware will
    // independently set any reboot flags for testing.
    xQueueSend(dataQueue, &msg, pdMS_TO_TICKS(100));
    vTaskDelete(NULL); // Task completes and releases memory
    // Send data to Comms task (non-blocking if queue is full)
    
}

// --- TASK B: COMMUNICATIONS (Core 0) ---
void taskComms(void *pvParameters) {
    GardenMessage rcv;

    ESP_LOGI(TASKS_CPP_COMMS_TAG, "Comms task started, waiting for data from Mango...");

    // Wait for data from Task Main
    if (xQueueReceive(dataQueue, &rcv, pdMS_TO_TICKS(10000)) == pdPASS) {
        ESP_LOGI(TASKS_CPP_COMMS_TAG, "Received data from Mango. Starting WiFi/MQTT sequence...");

        // --- WIFI SETUP ---
        WiFi.mode(WIFI_STA);
        WiFi.begin(WIFI_SSID, WIFI_PASS);
        ESP_LOGI(TASKS_CPP_COMMS_TAG, "Connecting to WiFi SSID: %s", WIFI_SSID);

        const uint32_t wifiTimeoutMs = 10000; // 10s max
        uint32_t wifiStart = millis();
        while (WiFi.status() != WL_CONNECTED && (millis() - wifiStart) < wifiTimeoutMs) {
            ESP_LOGI(TASKS_CPP_COMMS_TAG, "WiFi status: %d, waiting...", WiFi.status());
            vTaskDelay(pdMS_TO_TICKS(500));
        }

        if (WiFi.status() != WL_CONNECTED) {
            ESP_LOGE(TASKS_CPP_COMMS_TAG, "WiFi connect FAILED after timeout.");
        } else {
            ESP_LOGI(TASKS_CPP_COMMS_TAG, "WiFi CONNECTED, IP: %s", WiFi.localIP().toString().c_str());

            // --- MQTT SETUP ---
            WiFiClientSecure espClient;
            espClient.setInsecure(); // skip certificate validation for quick testing
            PubSubClient mqttClient(espClient);
            mqttClient.setServer(MQTT_BROKER, MQTT_PORT);

            ESP_LOGI(TASKS_CPP_COMMS_TAG, "Connecting to MQTT broker %s:%d ...", MQTT_BROKER, MQTT_PORT);

            String clientId = String("ESP32-MAIN-") + String(random(0xffff), HEX);

            const uint8_t maxMqttRetries = 3;
            uint8_t attempt = 0;
            bool mqttOk = false;
            while (attempt < maxMqttRetries && !mqttOk) {
                if (mqttClient.connect(clientId.c_str(), MQTT_USER, MQTT_PASS)) {
                    mqttOk = true;
                    ESP_LOGI(TASKS_CPP_COMMS_TAG, "MQTT CONNECTED on attempt %d", attempt + 1);
                } else {
                    ESP_LOGE(TASKS_CPP_COMMS_TAG, "MQTT connect FAIL rc=%d (attempt %d)", mqttClient.state(), attempt + 1);
                    vTaskDelay(pdMS_TO_TICKS(2000));
                    attempt++;
                }
            }

            if (mqttOk) {
                // Simple payload based on received data
                String payload;
                payload.reserve(128);
                payload += "TEST MSG | T="; payload += String(rcv.temp, 1);
                payload += " H=";        payload += String(rcv.hum, 1);
                payload += " M=";        payload += String(rcv.moisture);
                payload += " W=";        payload += (rcv.waterOk ? "1" : "0");
                payload += " V=";        payload += String(rcv.battery, 1);

                ESP_LOGI(TASKS_CPP_COMMS_TAG, "Publishing to topic %s: %s", TOPIC_PUB, payload.c_str());

                if (mqttClient.publish(TOPIC_PUB, payload.c_str(), true)) {
                    ESP_LOGI(TASKS_CPP_COMMS_TAG, "MQTT publish SUCCESS");
                } else {
                    ESP_LOGE(TASKS_CPP_COMMS_TAG, "MQTT publish FAILED");
                }

                mqttClient.loop();
            }
        }
    } else {
        // --- SECURITY ELSE BLOCK ---
        // This triggers if taskMain didn't send data within 10 seconds
        ESP_LOGE(TASKS_CPP_COMMS_TAG,"taskMain timed out or failed to send data!");
        
        // Option 1: Send an 'Error' message to MQTT instead
        // Option 2: Log the error to RTC memory for the next boot
    }

    // Signal setup() that we are finished
    xEventGroupSetBits(syncBits, BIT_COMMS_DONE);
    vTaskDelete(NULL); 
}