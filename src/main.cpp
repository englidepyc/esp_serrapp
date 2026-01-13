#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/event_groups.h>
#include "config.h"
#include "types.h"

// Globals for Task Coordination
QueueHandle_t dataQueue;
EventGroupHandle_t syncBits;
#define BIT_COMMS_DONE (1 << 0)

// Forward declarations
void taskMain(void *pvParameters);
void taskComms(void *pvParameters);

void setup() {
    Serial.begin(115200);
    
    // 1. Initialize Hardware (I2C, Pins, SR)
    // pinMode(PIN_SR_LATCH, OUTPUT); ...
    
    // 2. Create RTOS Objects
    dataQueue = xQueueCreate(5, sizeof(GardenMessage));
    syncBits = xEventGroupCreate();

    // 3. Launch Workers
    // Main Logic on Core 1, Comms on Core 0
    xTaskCreatePinnedToCore(taskMain,  "Main",  8192, NULL, 1, NULL, 1);
    xTaskCreatePinnedToCore(taskComms, "Comms", 8192, NULL, 1, NULL, 0);

    // 4. Wait for Join (The Boss blocks here)
    xEventGroupWaitBits(syncBits, BIT_COMMS_DONE, pdFALSE, pdTRUE, portMAX_DELAY);

    // 5. Enter Deep Sleep
    Serial.println("System going to sleep...");
    esp_sleep_enable_timer_wakeup(DEEP_SLEEP_SEC * 1000000ULL);
    // esp_sleep_enable_ext0_wakeup...
    esp_deep_sleep_start();
}

void loop() {} // loop() is never reached