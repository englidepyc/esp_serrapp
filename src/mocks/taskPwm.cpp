#include <Arduino.h>
#include <freertos/portmacro.h>
#include <esp_log.h>
#include "taskPwm.h"

static constexpr const char* TAG_MOCK_PWM = "MOCK_PWM";

void pwmSetup() {
    ESP_LOGI(TAG_MOCK_PWM, "pwmSetup: nessuna inizializzazione hardware");
}

void taskPWM(void *pvParameters) {
    // Estrai il pin dai parametri (non usato nel mock)
    int pin = (int)(intptr_t)pvParameters;

    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(PWM_TIME_RESOLUTION);

    ESP_LOGI(TAG_MOCK_PWM, "taskPWM mock avviata sul pin %d (no GPIO)", pin);

    for(;;) {
        // Stessa logica temporale della versione reale, ma senza toccare i pin
        unsigned long msInWindow = millis() % PWM_TIME_WINDOW;
        int onTime = constrain(PWM_HEATER_POW, 0, 100) * PWM_TIME_RESOLUTION;

        bool shouldBeOn = msInWindow < onTime;

        // Opzionale: log molto sporadico per debug, non ogni 10 ms
        // if (msInWindow == 0) {
        //     ESP_LOGD(TAG_MOCK_PWM, "PWM mock: duty=%d%%", PWM_HEATER_POW);
        // }

        (void)shouldBeOn; // usato solo per simulare calcolo, nessun I/O

        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}
