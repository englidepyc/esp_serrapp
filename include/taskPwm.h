#pragma once

extern volatile int targetPower;
#define RELAY_ON LOW
#define RELAY_OFF HIGH

void pwmSetup();
void taskSSR(void *pvParameters);