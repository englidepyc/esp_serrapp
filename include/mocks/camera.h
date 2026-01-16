#pragma once
#include <Arduino.h>

static const char* TAG = "MOCK/CAMERA.H";

void init_camera_system(){
    ESP_LOGD(TAG,"Mock Camera Initialized!");
};