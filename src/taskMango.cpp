extern const char* TASKS_CPP_MANGO_TAG = "TASK_MANGO.CPP";
#include <Arduino.h>

#include "config.h"
#include "common_hw.h"
#include "control/proportional.h"
#include "taskComms.h"
#include "taskMango.h"
#include "main.h"

RTC_DATA_ATTR float currentWaterCalcLevel = 100.0f;

// --- TASK A: GARDEN LOGIC (Core 1) ---
void taskMango(void *pvParameters) {   
    MangoTaskConfig config{};
    if (pvParameters != nullptr) {
        MangoTaskConfig* incoming = static_cast<MangoTaskConfig*>(pvParameters);
        config = *incoming;
        delete incoming;
    } else {
        config.doPhoto = false;
        config.isEcoMode = true;
    }

    GardenMessage msg{};
    msg.errCode = 0;


    
    if (config.isEcoMode) {
    //******* MODALITA' ECO TPC ********
        msg.nextMessageTime = 60;


        //**PRIMA**
        msg.isActuated = false;
        
        msg.isWaterOver = !(digitalRead(FLOAT_SWITCH_PIN)); //logica negata
        msg.isWaterOver ? currentWaterCalcLevel = 0.0 : 1;                
        temp_hum_scan_vpd(msg.hum, msg.temp, msg.vpd);
        msg.moisture = readMoisture_scan();

        xQueueSend(dataQueue, &msg, pdMS_TO_TICKS(100));  
        
        if(config.doPhoto){
            CameraFrame beforeFrame{};
            xQueueSend(frameQueue, &beforeFrame, pdMS_TO_TICKS(100));
        }

 

            //***ATTUAZIONE***/


            ESP_LOGI(TASKS_CPP_MANGO_TAG, "Scan Result -> T:%.1f H:%.1f VPD:%.2f Soil:%d%% (fixed) Tank:%.1f", 
                msg.temp, msg.hum, msg.vpd, msg.moisture, msg.waterCalcLevel);
            
            proportional_actuation(&msg);




        //**DOPO**
        msg.isActuated = true;
        
        msg.isWaterOver = !(digitalRead(FLOAT_SWITCH_PIN)); //logica negata
        msg.isWaterOver ? currentWaterCalcLevel = 0.0 : 1;                
        temp_hum_scan_vpd(msg.hum, msg.temp, msg.vpd);
        msg.moisture = readMoisture_scan();

        xQueueSend(dataQueue, &msg, pdMS_TO_TICKS(100));
        
        if(config.doPhoto){
            CameraFrame afterFrame{};
            xQueueSend(frameQueue, &afterFrame, pdMS_TO_TICKS(100));
        }

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    } else {
        ESP_LOGD(TASKS_CPP_MANGO_TAG, "Modalita' PRECISION attivata in taskMango");
    }

    

    
    
    vTaskDelete(NULL);
}

//updates the currentWaterCalcLevel to the GardenMsg
void updateCalcWaterLevel(float& waterLevel_percentage, int duration){
    currentWaterCalcLevel = currentWaterCalcLevel - (K_WATER * duration);
    waterLevel_percentage = currentWaterCalcLevel;
}