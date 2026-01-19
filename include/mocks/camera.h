#pragma once
#include <Arduino.h>

static const char* M_CAMERA_H_TAG = "MOCK/CAMERA.H";

void init_camera_system(){
    ESP_LOGD(M_CAMERA_H_TAG,"Mock Camera Initialized!");
};