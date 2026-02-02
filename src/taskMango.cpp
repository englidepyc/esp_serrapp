extern const char* TASKS_CPP_MANGO_TAG = "TASK_MANGO.CPP";
#include <Arduino.h>

#include "config.h"
#include "common_hw.h"
#include "control/proportional.h"

extern QueueHandle_t dataQueue;

// --- TASK A: GARDEN LOGIC (Core 1) ---
void taskMango(void *pvParameters) {
    GardenMessage msg{};
    msg.errCode = 0;
    msg.isActuated = false;
    msg.nextMessageTime = 60;
    
    //TODO: IMPLEMENTARE LOGICA ACQUA FINITA e CALCOLO LIVELLO DELL'ACQUA
    msg.isWaterOver = !(digitalRead(FLOAT_SWITCH_PIN)); //logica negata    
    // TODO: Implement sensor reading logic
    temp_hum_scan_vpd(msg.hum, msg.temp, msg.vpd);
    msg.moisture = readMoisture_scan();
    msg.waterCalcLevel = 50; //%

    ESP_LOGI(TASKS_CPP_MANGO_TAG, "Scan Result -> T:%.1f H:%.1f VPD:%.2f Soil:%d%% (fixed) Tank:%.1f", 
             msg.temp, msg.hum, msg.vpd, msg.moisture, msg.waterCalcLevel);

    

    proportional_actuation(&msg);

    
    // Send data to Comms task
    xQueueSend(dataQueue, &msg, pdMS_TO_TICKS(100));
    vTaskDelete(NULL);
}