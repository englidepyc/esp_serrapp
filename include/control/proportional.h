#pragma once
#include "types.h"
//L'acqua è fissa
// --- CLIMATE CONSTANTS ---
const float TARGET_TEMP_DAY    = 24.0; 
const float TARGET_VPD         = 1.0;   // kPa (Ideal "Goldilocks" zone)

// --- TUNING FACTORS ---
const long  K_P_HEATER         = 30000; // ms per 1°C error
const long  MAX_HEAT_TIME_MS   = 120000;// 2 mins max heat

// K_p Mist: How many ms to mist for every 1.0 kPa of dryness?
// Example: If VPD is 2.0 (very dry), error is 1.0. Run mist for 15s.
const long  K_P_MIST           = 15000; 
const long  MAX_MIST_TIME_MS   = 45000; // 45s max (Prevent flooding)

const int   THRESH_SOIL_DRY       = 60;
const int   PUMP_DURATION_MS      = 5000; // 5 Seconds shot glass

// Deadbands (Don't react to tiny changes)
const float HYSTERESIS_VPD     = 0.2;   // Only mist if VPD > Target + 0.2
const float HYSTERESIS_TMP     = 2;

void proportional_actuation(GardenMessage* msg);