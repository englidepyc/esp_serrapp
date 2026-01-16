#include <WiFi.h>
#include <PubSubClient.h>
#include "config.h"
#include "types.h"

extern QueueHandle_t dataQueue;
extern EventGroupHandle_t syncBits;
#define BIT_COMMS_DONE (1 << 0)

// --- TASK A: GARDEN LOGIC (Core 1) ---
void taskMain(void *pvParameters) {
    GardenMessage msg;
    
    // TODO: Read Shift Register state from RTC if needed
    // TODO: Read Digital Inputs (Fire, Float Switch)
    // TODO: Select Mux Channels and Read Soil Moisture
    // TODO: Perform Climate Logic (Heater/Fans)
    // TODO: Update Shift Register (updateSR)

    // Send data to Comms
    xQueueSend(dataQueue, &msg, portMAX_DELAY);
    
    vTaskDelete(NULL); // Task completes and releases memory
}

// --- TASK B: COMMUNICATIONS (Core 0) ---
void taskComms(void *pvParameters) {
    GardenMessage rcv;

    // TODO: Connect WiFi
    // TODO: Setup MQTT (PubSubClient)

    // Wait for data from Task Main
    if (xQueueReceive(dataQueue, &rcv, pdMS_TO_TICKS(10000)) == pdPASS) {
        // TODO: Connect MQTT
        // TODO: Format JSON Payload
        // TODO: client.publish()
        // TODO: Check for incoming commands (Photo flag)
    }else {
        // --- SECURITY ELSE BLOCK ---
        // This triggers if taskMain didn't send data within 10 seconds
        Serial.println("Error: taskMain timed out or failed to send data!");
        
        // Option 1: Send an 'Error' message to MQTT instead
        // Option 2: Log the error to RTC memory for the next boot
    }

    // Signal setup() that we are finished
    xEventGroupSetBits(syncBits, BIT_COMMS_DONE);
    vTaskDelete(NULL); 
}