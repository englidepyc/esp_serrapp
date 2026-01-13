#include <Arduino.h>
#include "shift_register.h"
#include "setup.h"

void setup() {
    Serial.begin(115200);
    deep_sleep_setup();
}

void loop() {
    
    Serial.println("Waiting 5 seconds before deep sleep...");
    delay(5000);
    Serial.println("Gonna sleep now\n");
    esp_deep_sleep_start();
    
}

