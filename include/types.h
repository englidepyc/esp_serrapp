#pragma once
#include <Arduino.h>

struct GardenMessage {
    float temp;        // "t"
    float hum;
    float vpd;         // "h"
    int moisture;
    float waterCalcLevel;       // "m"
    bool isWaterOver;      // "w"  
    int nextMessageTime; //in minutes
    bool isActuated;    
    
    int errCode;       // "err"
    bool isFire;

    // Commands received from MQTT (Optional feedback loop)
    bool isCmdPhoto;
    bool isCmdModeEco;
};

struct CmdMessage {
    bool CmdPhotoReq;
    bool CmdModeEco;
    bool CmdStop;
};

struct MangoTaskConfig {
    bool doPhoto;   // se fare la foto
    bool isEcoMode; // true = eco, false = precision
};