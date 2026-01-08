#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/event_groups.h>
#include "config.h"
#include "types.h"
//https://www.hivemq.com/demos/websocket-client/

/*config.h file example (fill with passowords and configurations):
// --- CREDENTIALS---
#define WIFI_SSID       "YOUR_WIFI_SSID"
#define WIFI_PASS       "YOUR_WIFI_PASSWORD"

// MQTT Configuration
#define MQTT_BROKER     "YOUR_MQTT_SERVER" 
#define MQTT_PORT       8883             // SSL Port
#define MQTT_USER       "YOUR_USER_ID"   // Keep empty if public broker
#define MQTT_PASS       "YOUR_PASSWORD"  // Keep empty if public broker

// Topics
#define TOPIC_PUB       "garden/mangos/status"
#define TOPIC_CMD       "garden/cmd/capture"

// --- PINS ---
#define PIN_SR_DATA     47
#define PIN_SR_CLOCK    48
#define PIN_SR_LATCH    14
#define PIN_I2C_SDA     41
#define PIN_I2C_SCL     42
#define PIN_ANALOG_IN   1 

// --- SYSTEM ---
#define DEEP_SLEEP_SEC  10

*/

// --- GLOBALS ---
QueueHandle_t dataQueue;
EventGroupHandle_t syncBits;
#define BIT_COMMS_DONE (1 << 0)

// --- SIMULATED HARDWARE DRIVER ---
// In real life, this shifts bits. Here, it just prints.
void updateSR(uint8_t data) {
    // Simulation: Visualizing the byte
    Serial.print("[HW SIM] Shift Register Update: 0b");
    for (int i = 7; i >= 0; i--) {
        Serial.print((data >> i) & 1);
    }
    Serial.println();
    
    // Physical Write (Safe to leave even without hardware connected)
    digitalWrite(PIN_SR_LATCH, LOW);
    shiftOut(PIN_SR_DATA, PIN_SR_CLOCK, MSBFIRST, data);
    digitalWrite(PIN_SR_LATCH, HIGH);
}

// Forward Declaration
void taskComms(void *pvParameters);

// --- TASK A: MAIN LOGIC (The Brain) ---
void taskMain(void *pvParameters) {
    Serial.println("[MAIN] Logic Started. Reading Sensors...");
    GardenMessage msg;

    // 1. SIMULATE SENSORS (Replace with AHT20/ADC later)
    msg.temp = 20.0 + (random(0, 150) / 10.0);   // 20.0 - 35.0 C
    msg.hum = 40.0 + (random(0, 400) / 10.0);    // 40% - 80%
    msg.moisture = random(30, 90);               // 30% - 90%
    msg.waterOk = true;
    msg.battery = 4.15;
    msg.rst = "pwr"; // Hardcoded for now
    msg.errCode = 0;
    msg.cmdPhotoReq = false; // Default

    // 2. LOGIC DECISION (The Overlord Logic)
    // "If Temp < 22C, Turn ON Heater (Bit 2)"
    uint8_t actuatorState = 0;

    if (msg.temp < 22.0) {
        Serial.printf("[MAIN] Temp %.1f is Low! HEATER ON.\n", msg.temp);
        actuatorState |= (1 << 2); // Set Bit 2 (Heater)
    } else {
        Serial.printf("[MAIN] Temp %.1f is OK.\n", msg.temp);
    }

    // "If Moisture < 40%, Turn ON Pump (Bit 0)"
    if (msg.moisture < 40) {
        Serial.printf("[MAIN] Moisture %d%% is Low! PUMP ON.\n", msg.moisture);
        actuatorState |= (1 << 0); // Set Bit 0 (Pump)
    }

    // 3. ACTUATE
    msg.actState = actuatorState;
    updateSR(actuatorState);

    // 4. WAIT (Simulate working time)
    // If heaters were on, we'd stay awake here.
    vTaskDelay(pdMS_TO_TICKS(1000));

    // 5. TURN OFF ACTUATORS (Before Sleep)
    updateSR(0x00);

    // 6. HANDOFF TO COMMS
    Serial.println("[MAIN] Sending data to Queue...");
    xQueueSend(dataQueue, &msg, portMAX_DELAY);
    
    Serial.println("[MAIN] Retire.");
    vTaskDelete(NULL);
}

// --- SETUP (The Orchestrator) ---
void setup() {
    Serial.begin(115200);
    delay(1000); 
    Serial.println("\n--- SYSTEM WAKE ---");

    // Init Pins
    pinMode(PIN_SR_DATA, OUTPUT);
    pinMode(PIN_SR_CLOCK, OUTPUT);
    pinMode(PIN_SR_LATCH, OUTPUT);
    
    // Create RTOS Objects
    dataQueue = xQueueCreate(5, sizeof(GardenMessage));
    syncBits = xEventGroupCreate();

    // Spawn Workers
    xTaskCreatePinnedToCore(taskMain,  "Main",  4096, NULL, 1, NULL, 1);
    xTaskCreatePinnedToCore(taskComms, "Comms", 4096, NULL, 1, NULL, 0);

    // Wait for "Job Done" signal
    Serial.println("[SETUP] Waiting for Comms...");
    xEventGroupWaitBits(syncBits, BIT_COMMS_DONE, pdFALSE, pdTRUE, pdMS_TO_TICKS(15000));

    Serial.println("[SETUP] Sleep time.");
    Serial.flush();
    esp_sleep_enable_timer_wakeup(DEEP_SLEEP_SEC * 1000000ULL);
    esp_deep_sleep_start();
}

void loop() { /* Dead */ }