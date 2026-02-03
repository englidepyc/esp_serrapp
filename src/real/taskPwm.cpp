static const char* TAG_PWM = "TASK_PWM.CPP";
#include <Arduino.h>
#include <freertos/portmacro.h>
#include "taskPwm.h"
#include "pins.h"

void pwmSetup() {
    pinMode(HEATER_PWM_PIN, OUTPUT);
    pinMode(FAN_HEATER_PIN, OUTPUT);
    pinMode(HUMIF_PWM_PIN, OUTPUT);
    
    digitalWrite(HEATER_PWM_PIN, RELAY_OFF);
    digitalWrite(FAN_HEATER_PIN, RELAY_OFF);
    digitalWrite(HUMIF_PWM_PIN, RELAY_OFF);
    ESP_LOGI(TAG_PWM, "pwmSetup: pin riscaldatore, ventola riscaldatore e umidificatore DISATTIVATI (ALTI)");
}

//pwm con risoluzione e finestra temporale in ms specificata nell'header, che applica pwm lento a un pin 
void taskPWM(void *pvParameters) {
    // Estrai il pin dai parametri
    int pin = (int)(intptr_t)pvParameters;
    
    digitalWrite(pin, RELAY_OFF);
    
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(PWM_TIME_RESOLUTION);

    for(;;) {
        unsigned long msInWindow = millis() % PWM_TIME_WINDOW;
        int onTime = constrain(PWM_HEATER_POW, 0, 100) * PWM_TIME_RESOLUTION; 

        bool shouldBeOn = msInWindow < onTime;
        digitalWrite(pin, shouldBeOn ? RELAY_ON : RELAY_OFF);

        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}