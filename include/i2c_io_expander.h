#pragma once
#include <Arduino.h>

// Indirizzo I2C del PCF8574
// 0x20 è standard per il chip PCF8574 (Texas Instruments/NXP)
// 0x3F è standard per il chip PCF8574A (o molti moduli cinesi con display LCD)
// Controlla con un I2C scanner se non funziona!
#define PCF_ADDR 0x20
#define PCF_MUX_CHANNEL 4 

namespace io_expander {

enum Device : byte {
   PUMP_P0     = 0,
   FAN_EXT_P1  = 1,
   // I bit dell'AMUX sono consecutivi per facilitare lo shift
   AMUX_0_P2   = 2, 
   AMUX_1_P3   = 3,
   AMUX_2_P4   = 4,
};

// Prototipi
void initIOExpander();
void updateIOExpander(); // Funzione interna, ma accessibile se serve forzare l'update
void setAllOutputs(bool state);
void setDevice(Device dev, bool state);
void setAmuxChannel(byte channel);
void resetIOExpander();

}