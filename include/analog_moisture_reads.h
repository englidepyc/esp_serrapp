#pragma once
int readMoisture_single(uint8_t analog_channel);
int readMoisture_scan();

static constexpr int ANALOG_SENSORS_QTY = 2; //Moisture

// Valori di calibrazione (variano da sensore a sensore)
static constexpr int AIR_VALUE = 3200;   // Valore letto all'aria (completamente asciutto)
static constexpr int WATER_VALUE = 1500; // Valore letto in un bicchiere d'acqua (massima umidità)