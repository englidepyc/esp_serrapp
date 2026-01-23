extern const char* TASKS_CPP_MANGO_TAG;
#include "common_hw.h"
#include "control/proportional.h"

void proportional_actuation(GardenMessage* msg) {
    bool is_heating = false;

    // --- PHASE 1: TEMPERATURE CONTROL (Priority #1) ---
    if (msg->temp < TARGET_TEMP_DAY) {
        float error_temp = TARGET_TEMP_DAY - msg->temp; 
        long heat_duration = (long)(K_P_HEATER * error_temp);

        // Clamp to safety limit
        if (heat_duration > MAX_HEAT_TIME_MS) heat_duration = MAX_HEAT_TIME_MS;

        ESP_LOGI(TASKS_CPP_MANGO_TAG, "Heating: Error %.2fC -> %ld ms", error_temp, heat_duration);

        setDevice(FAN_HEAT_Q2, ON_R); 
        setDevice(HEATER_Q1, ON_R);
        vTaskDelay(pdMS_TO_TICKS(heat_duration)); 
        setDevice(HEATER_Q1, OFF_R);
        setDevice(FAN_HEAT_Q2, OFF_R);
        
        msg->isActuated = true;
        is_heating = true; // Mark this so we know we just dried the air further
    } 
    else if (msg->temp > (TARGET_TEMP_DAY + 2.0)) {
        // Venting logic (Bang-Bang)
        ESP_LOGW(TASKS_CPP_MANGO_TAG, "Temp High. Venting.");
        setDevice(FAN_EXT_Q4, ON_R);
        vTaskDelay(pdMS_TO_TICKS(30000)); 
        setDevice(FAN_EXT_Q4, OFF_R);
        msg->isActuated = true;
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

        setDevice(HUMIF_Q3, ON_R);
        
        // If we are misting, it's good to run the internal fan to disperse it
        // (Optional, depends on your wiring)
        setDevice(FAN_EXT_Q4, ON_R); 
        
        vTaskDelay(pdMS_TO_TICKS(mist_duration));
        
        setDevice(HUMIF_Q3, OFF_R);
        setDevice(FAN_EXT_Q4, OFF_R);
        
        msg->isActuated = true;
    }
    // High Humidity Check (Low VPD)
    else if (msg->vpd < 0.4) {
        // 0.4 kPa is "Swampy". Risk of Mold.
        ESP_LOGW(TASKS_CPP_MANGO_TAG, "VPD Low (Swampy). Pulse Venting.");
        setDevice(FAN_EXT_Q4, ON_R);
        vTaskDelay(pdMS_TO_TICKS(15000)); // Short burst to exchange air
        setDevice(FAN_EXT_Q4, OFF_R);
        msg->isActuated = true;
    }

    // --- PHASE 3: WATERING ---
    if (!msg->isWaterOver && msg->moisture < THRESH_SOIL_DRY) {
        ESP_LOGI(TASKS_CPP_MANGO_TAG, "Watering Pulse.");
        setDevice(PUMP_Q0, ON_R);
        vTaskDelay(pdMS_TO_TICKS(PUMP_DURATION_MS));
        setDevice(PUMP_Q0, OFF_R);
        msg->isActuated = true;
    }else if(msg->isWaterOver){
        ESP_LOGE(TASKS_CPP_MANGO_TAG, "Water tank depleted! Impossible to water plants!");
    }
}