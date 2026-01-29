/*Contains deep sleep and other types of setup*/
#pragma once
#include <Arduino.h>
#include "driver/rtc_io.h"
#include "pins.h"

#define uS_TO_S_FACTOR 1000000ULL
#define FIRE_EXT1_WAKEUP ESP_SLEEP_WAKEUP_EXT1

void print_wakeup_reason(esp_sleep_wakeup_cause_t wr);
void deep_sleep_setup(int secondi);