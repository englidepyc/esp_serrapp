#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

#include "types.h"
#include "camera.h"

constexpr int MQTT_WINDOW_MS = 10000;
constexpr int WAIT_MANGO_MSG_LIVE_MS = 2000; 

//Comunicazione tra Mango e Comms
extern QueueHandle_t dataQueue;   // GardenMessage
extern volatile bool live_mode;
