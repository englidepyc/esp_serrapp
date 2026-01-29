#pragma once

extern volatile int PWM_HEATER_POW;
extern volatile int PWM_HUMIF_POW;

void pwmSetup();
void taskPWM(void *pvParameters);