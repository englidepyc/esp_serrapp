constexpr static const char* TAG_PROPORTIONAL_CPP = "PROPORTIONAL.CPP";
#include <Arduino.h>
#include "pins.h"
#include "common_hw.h"
#include "control/proportional.h"


#include "taskMango.h"
#include "main.h"

long proportional_calculate_total_time(GardenMessage* msg) {
    long heat_duration_ms      = 0;
    long temp_vent_duration_ms = 0;
    long mist_duration_ms      = 0;
    long hum_vent_duration_ms  = 0;
    long pump_duration_ms      = 0;

    // --- PHASE 1: TEMPERATURE CONTROL (Priority #1) ---
    if (msg->temp < TARGET_TEMP_DAY) {
        float error_temp = TARGET_TEMP_DAY - msg->temp;
        long d = (long)(K_P_HEATER * error_temp);
        if (d > MAX_HEAT_TIME_MS) d = MAX_HEAT_TIME_MS;
        heat_duration_ms = d;
    }
    else if (msg->temp > (TARGET_TEMP_DAY + HYSTERESIS_TMP)) {
        temp_vent_duration_ms = FIXED_VENTING_TIME_MS;
    }

    // --- PHASE 2: VPD / HUMIDITY CONTROL ---
    if (msg->vpd > (TARGET_VPD + HYSTERESIS_VPD)) {
        float error_vpd = msg->vpd - TARGET_VPD;
        long d = (long)(K_P_MIST * error_vpd);
        if (d > MAX_MIST_TIME_MS) d = MAX_MIST_TIME_MS;
        mist_duration_ms = d;
    }
    else if (msg->vpd < 0.4) {
        hum_vent_duration_ms = FIXED_HUM_EXT_TIME_MS;
    }

    // --- PHASE 3: WATERING ---
    if (!msg->isWaterOver && msg->moisture < THRESH_SOIL_DRY) {
        pump_duration_ms = PUMP_DURATION_MS;
    }

    long total_actuation_ms = heat_duration_ms + temp_vent_duration_ms +
                              mist_duration_ms + hum_vent_duration_ms +
                              pump_duration_ms;

    ESP_LOGI(TAG_PROPORTIONAL_CPP,
             "Total actuation time (estimate): %ld ms",
             total_actuation_ms);

    return total_actuation_ms;
}

void proportional_actuation(GardenMessage* msg) {
    // Ricalcola le durate per applicare le attuazioni
    long heat_duration_ms      = 0;
    long temp_vent_duration_ms = 0;
    long mist_duration_ms      = 0;
    long hum_vent_duration_ms  = 0;
    long pump_duration_ms      = 0;

    // --- PHASE 1: TEMPERATURE CONTROL (Priority #1) ---
    if (msg->temp < TARGET_TEMP_DAY) {
        float error_temp = TARGET_TEMP_DAY - msg->temp;
        long d = (long)(K_P_HEATER * error_temp);
        if (d > MAX_HEAT_TIME_MS) d = MAX_HEAT_TIME_MS;
        heat_duration_ms = d;
    }
    else if (msg->temp > (TARGET_TEMP_DAY + HYSTERESIS_TMP)) {
        temp_vent_duration_ms = FIXED_VENTING_TIME_MS;
    }

    // --- PHASE 2: VPD / HUMIDITY CONTROL ---
    if (msg->vpd > (TARGET_VPD + HYSTERESIS_VPD)) {
        float error_vpd = msg->vpd - TARGET_VPD;
        long d = (long)(K_P_MIST * error_vpd);
        if (d > MAX_MIST_TIME_MS) d = MAX_MIST_TIME_MS;
        mist_duration_ms = d;
    }
    else if (msg->vpd < 0.4) {
        hum_vent_duration_ms = FIXED_HUM_EXT_TIME_MS;
    }

    // --- PHASE 3: WATERING ---
    if (!msg->isWaterOver && msg->moisture < THRESH_SOIL_DRY) {
        pump_duration_ms = PUMP_DURATION_MS;
    }

    // --- APPLY ACTUATIONS USING PRE-CALCULATED DURATIONS ---

    // Temperature control
    if (heat_duration_ms > 0) {
        float error_temp = TARGET_TEMP_DAY - msg->temp;
        ESP_LOGI(TAG_PROPORTIONAL_CPP, "Heating: Error %.2fC -> %ld s", error_temp, heat_duration_ms / 1000);

        digitalWrite(FAN_HEATER_PIN, RELAY_ON);
        PWM_HEATER_POW = 100;
        vTaskDelay(pdMS_TO_TICKS(heat_duration_ms));
        PWM_HEATER_POW = 0;
        digitalWrite(FAN_HEATER_PIN, RELAY_OFF);

    } else if (temp_vent_duration_ms > 0) {
        ESP_LOGW(TAG_PROPORTIONAL_CPP, "Temp High. Venting.");
        setDevice(FAN_EXT_P1, RELAY_ON);
        vTaskDelay(pdMS_TO_TICKS(temp_vent_duration_ms));
        setDevice(FAN_EXT_P1, RELAY_OFF);
    }

    // VPD / Humidity control
    if (mist_duration_ms > 0) {
        ESP_LOGI(TAG_PROPORTIONAL_CPP, "Air Dry (VPD %.2f). Misting for %ld ms", msg->vpd, mist_duration_ms);

        digitalWrite(HUMIF_PIN, RELAY_ON);
        setDevice(FAN_EXT_P1, RELAY_ON);
        vTaskDelay(pdMS_TO_TICKS(mist_duration_ms));
        digitalWrite(HUMIF_PIN, RELAY_OFF);
        setDevice(FAN_EXT_P1, RELAY_OFF);

    } else if (hum_vent_duration_ms > 0) {
        ESP_LOGW(TAG_PROPORTIONAL_CPP, "VPD Low (Swampy). Pulse Venting.");
        setDevice(FAN_EXT_P1, RELAY_ON);
        vTaskDelay(pdMS_TO_TICKS(hum_vent_duration_ms)); // Short burst to exchange air
        setDevice(FAN_EXT_P1, RELAY_OFF);
    }

    // Watering
    if (pump_duration_ms > 0) {
        ESP_LOGI(TAG_PROPORTIONAL_CPP, "Watering Pulse.");
        setDevice(PUMP_P0, RELAY_ON);
        vTaskDelay(pdMS_TO_TICKS(pump_duration_ms));
        setDevice(PUMP_P0, RELAY_OFF);
        updateCalcWaterLevel(msg->waterCalcLevel, pump_duration_ms / 1000);
    } else if (msg->isWaterOver) {
        ESP_LOGE(TAG_PROPORTIONAL_CPP, "Water tank depleted! Impossible to water plants!");
    }
}