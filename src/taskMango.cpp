constexpr static const char* TASKS_CPP_MANGO_TAG = "TASK_MANGO.CPP";
#include <Arduino.h>

#include "config.h"
#include "common_hw.h"
#include "control/proportional.h"
#include "taskComms.h"
#include "taskMango.h"
#include "main.h"

RTC_DATA_ATTR float currentWaterCalcLevel = 100.0;

// --- TASK A: GARDEN LOGIC (Core 1) ---
void taskMango(void *pvParameters) {   
    GardenMessage msg {};
    
    // pvParameters viene usato solo come flag: non serve heap
    bool isEcoMode = (pvParameters != nullptr);
    msg.errCode = 0;


    
    if (isEcoMode) {
    //******* MODALITA' ECO TPC ********

        //**PRIMA**
        msg.isActuated = false;
        
        msg.isWaterOver = !(digitalRead(FLOAT_SWITCH_PIN)); //logica negata
        if (msg.isWaterOver) {
            currentWaterCalcLevel = 0.0f;
            ESP_LOGE(TASKS_CPP_MANGO_TAG, "Acqua finita dal float switch!");
        }
        // Sincronizza il livello calcolato corrente con il messaggio "prima" irrigazione
        msg.waterCalcLevel = currentWaterCalcLevel;
        temp_hum_scan_vpd(msg.hum, msg.temp, msg.vpd);
        msg.moisture = readMoisture_scan();

        // Calcola il tempo totale di attuazione (ms) e traduci in secondi per nextMessageTime
        long totalMs = proportional_calculate_total_time(&msg);
        int totalS = (totalMs + 999) / 1000; // arrotonda per eccesso
        msg.nextMessageTime = totalS;

        xQueueSend(dataQueue, &msg, pdMS_TO_TICKS(100));  
 

            //***ATTUAZIONE***/


            ESP_LOGI(TASKS_CPP_MANGO_TAG, "Scan Result BEFORE actuation -> T:%.1f H:%.1f VPD:%.2f Soil:%d%% (fixed) Tank:%.1f", 
                msg.temp, msg.hum, msg.vpd, msg.moisture, msg.waterCalcLevel);
            
            proportional_actuation(&msg);

            ESP_LOGI(TASKS_CPP_MANGO_TAG, "Scan Result AFTER actuation -> T:%.1f H:%.1f VPD:%.2f Soil:%d%% (fixed) Tank:%.1f", 
                msg.temp, msg.hum, msg.vpd, msg.moisture, msg.waterCalcLevel);


        //**DOPO**
        msg.isActuated = true;

        msg.isWaterOver = !(digitalRead(FLOAT_SWITCH_PIN)); //logica negata
        if (msg.isWaterOver) {
            currentWaterCalcLevel = 0.0f;
        }
        // Allinea nuovamente il livello acqua per il messaggio "dopo" irrigazione
        msg.waterCalcLevel = currentWaterCalcLevel;
        temp_hum_scan_vpd(msg.hum, msg.temp, msg.vpd);
        msg.moisture = readMoisture_scan();
        msg.nextMessageTime = SLEEP_FIXED_DURATION;
        xQueueSend(dataQueue, &msg, pdMS_TO_TICKS(100));

    
    
    
    } else {
        ESP_LOGD(TASKS_CPP_MANGO_TAG, "Modalita' PRECISION attivata in taskMango");
    }

    

    
    
    vTaskDelete(NULL);
}

//updates the currentWaterCalcLevel to the GardenMsg
void updateCalcWaterLevel(float& waterLevel_percentage, int duration_s){
    currentWaterCalcLevel = currentWaterCalcLevel - (K_WATER * duration_s);
    waterLevel_percentage = currentWaterCalcLevel;
}