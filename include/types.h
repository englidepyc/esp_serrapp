#pragma once
#include <Arduino.h>

struct GardenMessage {
    float temp;        // "t"
    float hum;
    float vpd;         // "h"
    int moisture;
    float waterCalcLevel;       // "m"
    bool isWaterOver;      // "w"  
    int nextMessageTime; // in seconds
    bool isActuated;    
    
    int errCode;       // "err"
    bool isFire;

    // Commands received from MQTT (Optional feedback loop)
    bool isCmdPhoto;
    bool isCmdModeEco;
};

struct CmdMessage {
    bool CmdPhotoReq;
    bool CmdModePrecision;
    bool CmdStop;
};