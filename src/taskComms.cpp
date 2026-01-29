constexpr static const char* TASKS_CPP_COMMS_TAG = "TASK_COMMS.CPP";
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

#include "config.h"
#include "types.h"

#define BIT_COMMS_DONE (1 << 0)

extern QueueHandle_t dataQueue;
extern EventGroupHandle_t syncBits;

// --- TASK B: COMMUNICATIONS (Core 0) ---
void taskComms(void *pvParameters) {
    GardenMessage rcv;

    // Wait for data from Task Mango
    if (xQueueReceive(dataQueue, &rcv, pdMS_TO_TICKS(10000)) == pdPASS) {
        // TODO: WiFi setup
        
        // TODO: MQTT setup
        
        // TODO: Publish data to MQTT
        
    } else {
        // TODO: Handle timeout - taskMango didn't send data
    }

    // Signal setup() that we are finished
    xEventGroupSetBits(syncBits, BIT_COMMS_DONE);
    vTaskDelete(NULL); 
}