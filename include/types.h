#pragma once
#include <Arduino.h>

struct GardenMessage {
    float temp;        // "t"
    float hum;         // "h"
    int moisture;      // "m"
    bool waterOk;      // "w"
    float battery;     // "v"
    uint8_t actState;  // "act" (Shift Register State)
    int errCode;       // "err"
    const char* rst;   // "rst"
    
    // Commands received from MQTT (Optional feedback loop)
    bool cmdPhotoReq; 
};