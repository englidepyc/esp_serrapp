#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

#include "types.h"
#include "camera.h"

//Comunicazione tra Mango e Comms
extern QueueHandle_t dataQueue;   // GardenMessage
extern volatile bool live_mode;
