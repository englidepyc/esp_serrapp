#pragma once
#include <Arduino.h>
#include "pins.h"

static const char* TAG = "MOCK/SHIFT_REGISTER.H";

// Keep the same device names array for logging and compatibility
const char* deviceNames[] = {"PUMP", "HEATER", "HEAT_FAN", "HUMIDIFIER", "EXT_FAN", "AMUX0", "AMUX1", "AMUX2"};

// Same macros as the real implementation
#define  ON_R LOW
#define  OFF_R HIGH

// Same enum for devices so existing code compiles unchanged
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

// Internal mock state of the shift register outputs
byte current_sr_bits = 0;

// In the mock, this is a pure no-op for hardware, used only for logging if needed
inline void updateShiftRegister() {
   // No real hardware interaction here to keep tests safe
   ESP_LOGD(TAG, "Mock updateShiftRegister called, bits=0b%08b", current_sr_bits);
}

inline void setAllOutputs(bool state){
   current_sr_bits = (state ? 0b11111111 : 0);
   updateShiftRegister();
}

inline void setDevice(Device dev, bool state) {
   state ? bitSet(current_sr_bits, dev) : bitClear(current_sr_bits, dev);
   updateShiftRegister();

   // Maintain the same logging semantics as the real implementation
   if (state == ON_R) {
     ESP_LOGI(TAG, "%s activated! (mock)", deviceNames[dev]);
   } else {
     ESP_LOGI(TAG, "%s deactivated (mock)", deviceNames[dev]);
   }
}

inline void setAmuxChannel(byte channel) {
   if (channel > 7){
      ESP_LOGW(TAG, "Mock: invalid analog mux channel specified! (%d)", channel);
      return;
   }

   // Clear bits 5, 6 and 7
   current_sr_bits &= 0b00011111;

   // Shift the channel value into the upper 3 bits
   byte shiftedChannel = channel << 5;

   // Set the new channel bits
   current_sr_bits |= shiftedChannel;

   updateShiftRegister();
   ESP_LOGD(TAG, "Mock: analog mux set to channel %d", channel);
}

// Mock init: do NOT touch real pins, just reset internal state
inline void initShiftRegister() {
   current_sr_bits = 0b11111111;
   updateShiftRegister();
   ESP_LOGI(TAG, "Mock shift register setup: all outputs DISABLED (high in mock state)");
}
