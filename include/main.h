#pragma once
constexpr static int SLEEP_FIXED_DURATION = 600;

#define BIT_FIRE       (1 << 1)
#define BIT_COMMS_DONE (1 << 0)

//prototipi e task
void execute_fire_protocol();
void taskMango(void *pvParameters);
void taskComms(void *pvParameters);
void taskSSR(void* pvParameters);