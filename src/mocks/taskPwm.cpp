#include <Arduino.h>
#include <freertos/portmacro.h>
#include <esp_log.h>
#include "taskPwm.h"

static constexpr char* TAG_MOCK_PWM = "MOCK_PWM";

void pwmSetup() {
    ESP_LOGI(TAG_MOCK_PWM, "pwmSetup: nessuna inizializzazione hardware");
}

void taskPWM(void *pvParameters) {
    // Estrai il pin dai parametri (anche se non utilizzato)
    int pin = (int)(intptr_t)pvParameters;
    
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(100); // Delay più lungo nel mock

    ESP_LOGI(TAG_MOCK_PWM, "taskPWM avviata sul pin %d (nessuna azione)", pin);

    for(;;) {
        // Mock: non fa nulla, solo delay
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}
