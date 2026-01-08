#include <WiFi.h>
#include <WiFiClientSecure.h> // Using Secure Client as per your request
#include <PubSubClient.h>
#include "config.h"
#include "types.h"

extern QueueHandle_t dataQueue;
extern EventGroupHandle_t syncBits;
#define BIT_COMMS_DONE (1 << 0)

// Callback for incoming messages (e.g., Photo Request)
void mqttCallback(char* topic, byte* payload, unsigned int length) {
    Serial.print("[MQTT] Message on: ");
    Serial.println(topic);
    // In Step 4, we will check if payload == "1" to trigger photo
}

void taskComms(void *pvParameters) {
    GardenMessage rcv;
    
    // 1. CONNECT WIFI
    Serial.print("[COMMS] WiFi Connecting");
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    
    int retries = 0;
    while (WiFi.status() != WL_CONNECTED && retries < 20) {
        vTaskDelay(pdMS_TO_TICKS(500));
        Serial.print(".");
        retries++;
    }
    
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("\n[COMMS] WiFi Failed! Aborting.");
        xEventGroupSetBits(syncBits, BIT_COMMS_DONE); // Release Setup to sleep
        vTaskDelete(NULL);
        return;
    }
    Serial.println("\n[COMMS] WiFi Connected.");

    // 2. CONNECT MQTT (SECURE)
    WiFiClientSecure espClient;
    espClient.setInsecure(); // Skip cert validation for now (easier for testing)
    
    PubSubClient client(espClient);
    client.setServer(MQTT_BROKER, MQTT_PORT);
    client.setCallback(mqttCallback);

    // Wait for Data from Main Task *BEFORE* connecting MQTT
    // This ensures we have something to say before we waste radio time.
    if (xQueueReceive(dataQueue, &rcv, pdMS_TO_TICKS(5000))) {
        
        Serial.println("[COMMS] Data Received. Connecting MQTT...");
        
        String clientId = "MangoBot-" + String(random(0xffff), HEX);
        if (client.connect(clientId.c_str(), MQTT_USER, MQTT_PASS)) {
            Serial.println("[COMMS] MQTT Connected.");
            
            // A. Check for incoming commands (Subscribe)
            client.subscribe(TOPIC_CMD);
            client.loop(); // Process any retained messages immediately

            // B. Format JSON Payload (The New Structure)
            // keys: t=temp, h=hum, m=moisture, w=water, v=volt, act=actuators, err=error, rst=reset
            char payload[128];
            snprintf(payload, sizeof(payload), 
                "{\"t\":%.1f,\"h\":%.1f,\"m\":%d,\"w\":%d,\"v\":%.2f,\"act\":%d,\"err\":%d,\"rst\":\"%s\"}",
                rcv.temp, rcv.hum, rcv.moisture, rcv.waterOk, rcv.battery, rcv.actState, rcv.errCode, rcv.rst
            );

            // C. Publish
            client.publish(TOPIC_PUB, payload);
            Serial.printf("[COMMS] Published: %s\n", payload);
            
            // Small delay to ensure packet transmission
            vTaskDelay(pdMS_TO_TICKS(500));
            client.disconnect();
            
        } else {
            Serial.printf("[COMMS] MQTT Connect Failed. State=%d\n", client.state());
        }
    } else {
        Serial.println("[COMMS] Timeout: No data from Main Task.");
    }

    // 3. DONE
    xEventGroupSetBits(syncBits, BIT_COMMS_DONE);
    vTaskDelete(NULL);
}