//TODO: Testare!
constexpr static const char* TAG = "ANALOG_MOISTURE_READS.CPP";
#include <Arduino.h>
#include "pins.h"
#include "analog_moisture_reads.h"
#include "shift_register.h"
#include "i2c_io_expander.h"

int readMoisture_single(uint8_t analog_channel) {
    //scegli il canale dal mux analogico
    shift_register::setAmuxChannel(analog_channel);

    // 1. Leggi il valore analogico grezzo (0-4095 su ESP32)
    int rawValue = analogRead(MOIST_AOUT_PIN);

    // 2. Mappa il valore in una percentuale
    // Poiché AIR_VALUE è più alto di WATER_VALUE, mappiamo al contrario
    int percentage = map(rawValue, AIR_VALUE, WATER_VALUE, 0, 100);

    // 3. Vincola il valore tra 0 e 100 per evitare errori di calibrazione
    percentage = constrain(percentage, 0, 100);

    return percentage;
}

int readMoisture_scan() {
    int sum = 0;
    
    // Leggi tutti i sensori definiti in ANALOG_SENSORS_QTY
    for (uint8_t i = 0; i < ANALOG_SENSORS_QTY; i++) {
        int reading = readMoisture_single(i);
        sum += reading;
        
        // Piccolo delay per stabilizzazione tra letture successive
        delayMicroseconds(500);
    }
    
    // Calcola e restituisci la media
    return sum / ANALOG_SENSORS_QTY;
}
