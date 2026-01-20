constexpr static const char* TASKS_CPP_MANGO_TAG = "TASK_MANGO.CPP";
#include <Arduino.h>

#include "common.h"

extern QueueHandle_t dataQueue;

// --- TASK A: GARDEN LOGIC (Core 1) ---
void taskMango(void *pvParameters) {
    GardenMessage msg{};
    // TODO: Implement sensor reading logic
    
    // TODO: Implement actuator control logic
    
    // Send data to Comms task
    xQueueSend(dataQueue, &msg, pdMS_TO_TICKS(100));
    vTaskDelete(NULL);
}