#pragma once
int readMoisture_single();
int readMoisture_scan();

#define ANALOG_SENSORS_QTY 2 //Moisture

// Valori di calibrazione (variano da sensore a sensore)
constexpr int AIR_VALUE = 3200;   // Valore letto all'aria (completamente asciutto)
constexpr int WATER_VALUE = 1500; // Valore letto in un bicchiere d'acqua (massima umidità)