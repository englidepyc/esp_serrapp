#pragma once

extern volatile int targetPower;

void pwmSetup();
void taskSSR(void *pvParameters);