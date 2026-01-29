constexpr static const char* SHIFT_REGISTER_H_TAG = "SHIFT_REGISTER.CPP";
static const char* deviceNames[] = {"PUMP", "HEATER", "HEAT_FAN", "HUMIDIFIER", "EXT_FAN", "AMUX0", "AMUX1", "AMUX2"};
#include <Arduino.h>
#include "shift_register.h"
#include "pins.h"

static byte current_sr_bits = 0xFF; 

void updateShiftRegister() {
   digitalWrite(LATCH_PIN, LOW);
   shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, current_sr_bits);
   digitalWrite(LATCH_PIN, HIGH);
}

void setAllOutputs(bool state){
   current_sr_bits = (state ? 0b11111111 : 0);
   updateShiftRegister(); 
}

void setDevice(Device dev, bool state) {
   state ? bitSet(current_sr_bits, dev) : bitClear(current_sr_bits, dev);
   updateShiftRegister();

   // Using your ON_R (LOW) define for clarity
   if (state == ON_R) {
       ESP_LOGI(SHIFT_REGISTER_H_TAG, "%s activated!", deviceNames[dev]);
   } else {
       ESP_LOGI(SHIFT_REGISTER_H_TAG, "%s deactivated", deviceNames[dev]);
   }
}

void setAmuxChannel(byte channel) {
    
   if (channel > 7){
      ESP_LOGW(SHIFT_REGISTER_H_TAG, "Canale mux analogico specificato non valido!\n");
      return;
    } 
    
    // 1. Puliamo i bit 5, 6 e 7 della variabile current_sr_bits (mettiamoli a 0)
    // Usiamo una maschera: 0b00011111 (0x1F) mantiene i primi 5 bit e azzera gli ultimi 3
    current_sr_bits &= 0b00011111;

    // 2. Prendiamo il numero del canale e lo spostiamo a sinistra di 5 posizioni
    // Esempio: canale 3 (binario 011) diventa 01100000
    byte shiftedChannel = channel << 5;

    // 3. Uniamo il nuovo canale alla variabile corrente con l'operatore OR (|)
    current_sr_bits |= shiftedChannel;

    // 4. Inviamo allo Shift Register
    updateShiftRegister();
   ESP_LOGD(SHIFT_REGISTER_H_TAG, "Mux analogico settato sul canale %d",channel);
}


//setup e reset uscite a 1 (relè spento)
void initShiftRegister() {
   pinMode(DATA_PIN, OUTPUT);
   pinMode(LATCH_PIN, OUTPUT);
   pinMode(CLOCK_PIN, OUTPUT);

   current_sr_bits = 0b11111111;
   updateShiftRegister();
   ESP_LOGI(SHIFT_REGISTER_H_TAG, "Shift register setup: tutto DISATTIVATO (alto)");
}
