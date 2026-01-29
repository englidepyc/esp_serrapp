#pragma once
#include <Arduino.h>

#define ON_R LOW
#define OFF_R HIGH

enum Device : byte {
   PUMP_Q0     = 0,
   HEATER_Q1   = 1,
   FAN_HEAT_Q2 = 2,
   HUMIF_Q3    = 3,
   FAN_EXT_Q4  = 4,
   AMUX_0_Q5   = 5,
   AMUX_1_Q6   = 6,
   AMUX_2_Q7   = 7,
};

void initShiftRegister();
void updateShiftRegister();
void setAllOutputs(bool state);
void setDevice(Device dev, bool state);
void setAmuxChannel(byte channel);