constexpr static const char* M_SHIFT_REGISTER_H_TAG = "MOCK/SHIFT_REGISTER.CPP";
static const char* deviceNames[] = {"PUMP", "HEATER", "HEAT_FAN", "HUMIDIFIER", "EXT_FAN", "AMUX0", "AMUX1", "AMUX2"};
#include <Arduino.h>
#include "pins.h"
#include "shift_register.h"


// Internal mock state of the shift register outputs
static byte current_sr_bits = 0xFF;

// In the mock, this is a pure no-op for hardware, used only for logging if needed
void updateShiftRegister() {
   // No real hardware interaction here to keep tests safe
   ESP_LOGD(M_SHIFT_REGISTER_H_TAG, "Mock updateShiftRegister called, bits=0b%08b", current_sr_bits);
}

void setAllOutputs(bool state){
   current_sr_bits = (state ? 0b11111111 : 0);
   updateShiftRegister();
}

void setDevice(Device dev, bool state) {
   state ? bitSet(current_sr_bits, dev) : bitClear(current_sr_bits, dev);
   updateShiftRegister();

   // Maintain the same logging semantics as the real implementation
   if (state == ON_R) {
       ESP_LOGI(M_SHIFT_REGISTER_H_TAG, "%s activated! (mock)", deviceNames[dev]);
   } else {
       ESP_LOGI(M_SHIFT_REGISTER_H_TAG, "%s deactivated (mock)", deviceNames[dev]);
   }
}

void setAmuxChannel(byte channel) {
   if (channel > 7){
      ESP_LOGW(M_SHIFT_REGISTER_H_TAG, "Mock: invalid analog mux channel specified! (%d)", channel);
      return;
   }

   // Clear bits 5, 6 and 7
   current_sr_bits &= 0b00011111;

   // Shift the channel value into the upper 3 bits
   byte shiftedChannel = channel << 5;

   // Set the new channel bits
   current_sr_bits |= shiftedChannel;

   updateShiftRegister();
   ESP_LOGD(M_SHIFT_REGISTER_H_TAG, "Mock: analog mux set to channel %d", channel);
}

// Mock init: do NOT touch real pins, just reset internal state
void initShiftRegister() {
   current_sr_bits = 0b11111111;
   updateShiftRegister();
   ESP_LOGI(M_SHIFT_REGISTER_H_TAG, "Mock shift register setup: all outputs DISABLED (high in mock state)");
}

// for debugging purposes
byte readCurrentSrBits() {
   ESP_LOGD(SHIFT_REGISTER_H_TAG, "current_sr_bits: 0x%02X", current_sr_bits);
   return current_sr_bits;
}
