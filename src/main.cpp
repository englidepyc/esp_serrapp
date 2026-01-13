#include <Arduino.h>
#include "shift_register.h"

const int INPUT_PIN = 10;

void setup() {
    Serial.begin(115200);
    initShiftRegister();
    pinMode(INPUT_PIN, INPUT_PULLUP);

    Serial.println("\n--- Sistema Pronto ---");
    Serial.println("Comandi disponibili:");
    Serial.println("- 'XY' : Imposta Device X (0-7) a Stato Y (0-1). Es: '01'");
    Serial.println("- 'r'  : Leggi stato GPIO 10");
    Serial.println("- 'H'  : Metti TUTTI i pin a HIGH");
    Serial.println("- 'L'  : Metti TUTTI i pin a LOW");
}

void loop() {
    if (Serial.available() > 0) {
        char firstChar = Serial.read();

        // 1. COMANDO 'r' - Lettura Digitale
        if (firstChar == 'r' || firstChar == 'R') {
            int val = digitalRead(INPUT_PIN);
            Serial.print(">>> Stato GPIO 10: ");
            Serial.println(val == HIGH ? "HIGH" : "LOW");
        } 
        
        // 2. COMANDO 'H' - Tutti i pin HIGH
        else if (firstChar == 'H' || firstChar == 'h') {
            Serial.println(">>> TUTTI i pin impostati a HIGH");
            current_sr_bits = 0xFF; // 11111111
            updateShiftRegister();
        }

        // 3. COMANDO 'L' - Tutti i pin LOW (Master Reset Software)
        else if (firstChar == 'L' || firstChar == 'l') {
            Serial.println(">>> TUTTI i pin impostati a LOW");
            current_sr_bits = 0x00; // 00000000
            updateShiftRegister();
        }

        // 4. COMANDO 'XY' - Controllo Singolo Bit (es. '01')
        else if (isDigit(firstChar)) {
            delay(10); // Piccolo delay per attendere il secondo carattere
            if (Serial.available() > 0) {
                char secondChar = Serial.read();
                
                int deviceIndex = firstChar - '0';
                bool state = (secondChar == '1');

                if (deviceIndex >= 0 && deviceIndex <= 7) {
                    Serial.print(">>> Device Q");
                    Serial.print(deviceIndex);
                    Serial.println(state ? " -> ON" : " -> OFF");
                    setDevice((Device)deviceIndex, state);
                } else {
                    Serial.println("Errore: Indice non valido (0-7)");
                }
            }
        }

        // Pulizia finale del buffer
        while(Serial.available() > 0 && (Serial.peek() == '\n' || Serial.peek() == '\r')) {
            Serial.read();
        }
    }
}