constexpr static const char* TAG = "ANALOG_MOISTURE_READS_MOCK.CPP";
#include <Arduino.h>
#include "pins.h"
#include "analog_moisture_reads.h"
/*VERSIONE MOCK*/


// Valori di calibrazione (variano da sensore a sensore)
static const int AIR_VALUE = 3200;   // Valore letto all'aria (completamente asciutto)
static const int WATER_VALUE = 1500; // Valore letto in un bicchiere d'acqua (massima umidità)


int readMoisture() {
    // 1. Leggi il valore analogico grezzo (0-4095 su ESP32)
    // int rawValue = analogRead(MOIST_AOUT_PIN); // TODO: riabilitare quando si usa il sensore reale

    // Valore mockato casuale per i test
    // Genera un valore grezzo compreso tra WATER_VALUE e AIR_VALUE
    int rawValue = random(WATER_VALUE, AIR_VALUE + 1);

    // 2. Mappa il valore in una percentuale
    // Poiché AIR_VALUE è più alto di WATER_VALUE, mappiamo al contrario
    int percentage = map(rawValue, AIR_VALUE, WATER_VALUE, 0, 100);

    // 3. Vincola il valore tra 0 e 100 per evitare errori di calibrazione
    percentage = constrain(percentage, 0, 100);

    return percentage;
}

void analogMoistReadSetup(){};
