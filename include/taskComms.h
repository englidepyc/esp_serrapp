#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

#include "types.h"
#include "camera.h"

// Code di comunicazione tra Mango e Comms
extern QueueHandle_t dataQueue;   // GardenMessage
extern QueueHandle_t frameQueue;  // CameraFrame
