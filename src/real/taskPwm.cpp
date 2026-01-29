static const char* TAG_PWM = "TASK_PWM.CPP";
#include <Arduino.h>
#include <freertos/portmacro.h>
#include "taskPwm.h"
#include "pins.h"

void pwmSetup() {
    pinMode(HEATER_PWM_PIN, OUTPUT);
    pinMode(HEATER_FAN_PIN, OUTPUT);
    pinMode(HUMIF_PWM_PIN, OUTPUT);
    
    digitalWrite(HEATER_PWM_PIN, RELAY_OFF);
    digitalWrite(HEATER_FAN_PIN, RELAY_OFF);
    digitalWrite(HUMIF_PWM_PIN, RELAY_OFF);
    ESP_LOGI(TAG_PWM, "pwmSetup: pin riscaldatore, ventola riscaldatore e umidificatore DISATTIVATI (ALTI)");
}

void taskSSR(void *pvParameters) {
    // Estrai il pin dai parametri
    int pin = (int)(intptr_t)pvParameters;
    
    digitalWrite(pin, RELAY_OFF);
    
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(10);

    for(;;) {
        unsigned long msInWindow = millis() % 1000;
        int onTime = constrain(targetPower, 0, 100) * 10; 

        bool shouldBeOn = (targetPower > 0) && (msInWindow < onTime);
        digitalWrite(pin, shouldBeOn ? RELAY_ON : RELAY_OFF);

        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}