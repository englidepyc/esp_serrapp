constexpr static const char* TASKS_CPP_MANGO_TAG = "PROPORTIONAL.CPP";
#include <Arduino.h>
#include "pins.h"
#include "common_hw.h"
#include "control/proportional.h"


#include "taskMango.h"
#include "main.h"

void proportional_actuation(GardenMessage* msg) {
    bool is_heating = false;

    // --- PHASE 1: TEMPERATURE CONTROL (Priority #1) ---
    if (msg->temp < TARGET_TEMP_DAY) {
        float error_temp = TARGET_TEMP_DAY - msg->temp; 
        long heat_duration = (long)(K_P_HEATER * error_temp);

        // Clamp to safety limit
        if (heat_duration > MAX_HEAT_TIME_MS) heat_duration = MAX_HEAT_TIME_MS;

        ESP_LOGI(TASKS_CPP_MANGO_TAG, "Heating: Error %.2fC -> %ld ms", error_temp, heat_duration);

        digitalWrite(FAN_HEATER_PIN, RELAY_ON); 
        PWM_HEATER_POW = 100;
        vTaskDelay(pdMS_TO_TICKS(heat_duration)); 
        PWM_HEATER_POW = 0;
        digitalWrite(FAN_HEATER_PIN, RELAY_OFF);
        

        is_heating = true; // Mark this so we know we just dried the air further
    } 
    else if (msg->temp > (TARGET_TEMP_DAY + HYSTERESIS_TMP)) {
        // Venting logic (Bang-Bang)
        ESP_LOGW(TASKS_CPP_MANGO_TAG, "Temp High. Venting.");
        setDevice(FAN_EXT_P1, RELAY_ON);
        vTaskDelay(pdMS_TO_TICKS(FIXED_VENTING_TIME_MS)); 
        setDevice(FAN_EXT_P1, RELAY_OFF);

    }

    // --- PHASE 2: VPD / HUMIDITY CONTROL ---
    
    // Check if air is too dry (VPD > Target + Hysteresis)
    if (msg->vpd > (TARGET_VPD + HYSTERESIS_VPD)) {
        
        float error_vpd = msg->vpd - TARGET_VPD;
        
        // Proportional Calculation
        long mist_duration = (long)(K_P_MIST * error_vpd);
        
        // Safety Clamp
        if (mist_duration > MAX_MIST_TIME_MS) mist_duration = MAX_MIST_TIME_MS;

        ESP_LOGI(TASKS_CPP_MANGO_TAG, "Air Dry (VPD %.2f). Misting for %ld ms", msg->vpd, mist_duration);

        digitalWrite(HUMIF_PIN,RELAY_ON);
        
        // If we are misting, it's good to run the internal fan to disperse it
        // (Optional, depends on your wiring)
        setDevice(FAN_EXT_P1, RELAY_ON); 
        
        vTaskDelay(pdMS_TO_TICKS(mist_duration));
        
        digitalWrite(HUMIF_PIN,RELAY_OFF);
        setDevice(FAN_EXT_P1, RELAY_OFF);
        

    }
    // High Humidity Check (Low VPD)
    else if (msg->vpd < 0.4) {
        // 0.4 kPa is "Swampy". Risk of Mold.
        ESP_LOGW(TASKS_CPP_MANGO_TAG, "VPD Low (Swampy). Pulse Venting.");
        setDevice(FAN_EXT_P1, RELAY_ON);
        vTaskDelay(pdMS_TO_TICKS(FIXED_HUM_EXT_TIME_MS)); // Short burst to exchange air
        setDevice(FAN_EXT_P1, RELAY_OFF);

    }

    // --- PHASE 3: WATERING ---
    if (!msg->isWaterOver && msg->moisture < THRESH_SOIL_DRY) {
        ESP_LOGI(TASKS_CPP_MANGO_TAG, "Watering Pulse.");
        setDevice(PUMP_P0, RELAY_ON);
        vTaskDelay(pdMS_TO_TICKS(PUMP_DURATION_MS));
        setDevice(PUMP_P0, RELAY_OFF);
        updateCalcWaterLevel(msg->waterCalcLevel, PUMP_DURATION_MS);
    }else if(msg->isWaterOver){
        ESP_LOGE(TASKS_CPP_MANGO_TAG, "Water tank depleted! Impossible to water plants!");
    }
}