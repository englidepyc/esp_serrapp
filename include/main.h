#pragma once
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/event_groups.h>

#include "types.h"
constexpr static int SLEEP_FIXED_DURATION = 10;

#define BIT_FIRE       (1 << 0)
#define BIT_MANGO_DONE (1 << 1)
#define BIT_COMMS_DONE (1 << 2)

//prototipi e task
void execute_fire_protocol();
void taskMango(void *pvParameters);
void taskComms(void *pvParameters);
void taskPWM(void* pvParameters);

extern TaskHandle_t handleMango;
extern TaskHandle_t handleComms;
extern TaskHandle_t handlePWM;

