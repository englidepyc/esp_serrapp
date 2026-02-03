#pragma once
#define RELAY_ON LOW
#define RELAY_OFF HIGH

//PWM E ATTUAZIONI (ex shift register)
#define HEATER_PWM_PIN 47
#define FAN_HEATER_PIN 21
#define HUMIF_PIN 19

/* //SHIFT REGISTER
#define DATA_PIN 47   // DS
#define LATCH_PIN 21  // ST_CP
#define CLOCK_PIN 19 // SH_CP */

//AOUT dal mux analogico per i sensori umidità terreno
#define MOIST_AOUT_PIN 1

#define FLOAT_SWITCH_PIN 20

//I2C (mux per sensori temperatura e umidità)
#define SDA_PIN 42
#define SCL_PIN 41


//DEEP SLEEP (GPIO FIRE SENSOR)
#define WAKE_PIN 2

//per attivare o disattivare l'alimentazione durante il deep sleep
#define NPN_PIN 14