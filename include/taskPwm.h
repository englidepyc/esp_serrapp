#pragma once

extern volatile int PWM_HEATER_POW;
extern volatile int PWM_HUMIF_POW;

constexpr int PWM_TIME_RESOLUTION = 20; //ms
constexpr int PWM_TIME_WINDOW = PWM_TIME_RESOLUTION * 100;

void pwmSetup();
void taskPWM(void *pvParameters);