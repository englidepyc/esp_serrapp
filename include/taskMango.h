#pragma once
#include <esp_attr.h>

constexpr float K_WATER = 10;
extern RTC_DATA_ATTR float waterLevel;

void updateCalcWaterLevel(float& waterLevel_percentage, int duration);