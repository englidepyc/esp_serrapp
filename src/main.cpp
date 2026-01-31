constexpr static const char* MAIN_CPP_TAG = "MAIN.CPP/SETUP";
#include "main.h"
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/event_groups.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

#include "config.h"
#include "common_hw.h"
#include "camera.h"
#include "deep_sleep.h"

// Globals for Task Coordination
QueueHandle_t dataQueue;
EventGroupHandle_t syncBits;

//ISR, flag per fuoco
volatile RTC_NOINIT_ATTR bool fire_reboot_triggered;
void IRAM_ATTR fire_isr() {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    
    // Set the FIRE bit from the ISR
    if (syncBits != NULL) {
        xEventGroupSetBitsFromISR(syncBits, BIT_FIRE, &xHigherPriorityTaskWoken);
    }

    // Force context switch if setup() was waiting for this
    if (xHigherPriorityTaskWoken) {
        portYIELD_FROM_ISR();
    }
}

//PWM
volatile int PWM_HEATER_POW = 0;
volatile int PWM_HUMIF_POW = 0;

void setup() {
    Serial.begin(115200);
    esp_log_level_set("*", ESP_LOG_DEBUG);

    pwmSetup(); //sempre impostiamo i pin del riscaldatore spenti (RELAY_OFF) per sicurezza come prima cosa e inizializziamo i pin

    // Inizializzazioni variabili RTC al primo avvio
    if (esp_reset_reason() == ESP_RST_POWERON) {
        fire_reboot_triggered = false;
        ESP_LOGI(MAIN_CPP_TAG, "Cold Boot. System Clean.");
    }
    
    // Se necessario, entra in modalità fuoco (rilevato durante lo sleep o reboot da sveglio)
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
    print_wakeup_reason(wakeup_reason);
    if (wakeup_reason == FIRE_EXT1_WAKEUP || fire_reboot_triggered) {
        // Ensure flag is set for the NEXT loop if we crash
        fire_reboot_triggered = true; 
        execute_fire_protocol();
    }
    
    
    //impostiamo pin acuqa, fuoco e ingresso analog (moisture) e mettiamo l'interrupt che setta flag reboot fuoco durante la fase "da sveglio"
    rtc_gpio_hold_dis((gpio_num_t)WAKE_PIN); //sblocca il pin che era stato attaccato all'rtc deep sleep per accesso durante la veglia
    pinMode(WAKE_PIN, INPUT_PULLDOWN);
    attachInterrupt(digitalPinToInterrupt(WAKE_PIN), fire_isr, RISING);
    pinMode(FLOAT_SWITCH_PIN,INPUT_PULLUP);
    pinMode(MOIST_AOUT_PIN,INPUT_PULLUP);    

    i2c_setup(true);
    camera_init();

    
    // 2. Create RTOS Objects
    dataQueue = xQueueCreate(5, sizeof(GardenMessage));
    syncBits = xEventGroupCreate();

    // 3. Launch Workers
    xTaskCreatePinnedToCore(taskMango,  "Mango",  8192, NULL, 1, NULL, 1);
    xTaskCreatePinnedToCore(taskComms, "Comms", 8192, NULL, 1, NULL, 0);
    
    xTaskCreatePinnedToCore(taskPWM, "PWM_Heater", 1024, (void*)(intptr_t)HEATER_PWM_PIN, 1, NULL, 1);
    //DOPO: xTaskCreatePinnedToCore(taskPWM, "PWM_Humif", 1024, (void*)(intptr_t)HUMIF_PWM_PIN, 1, NULL, 1);

    ESP_LOGI(MAIN_CPP_TAG,"Task creation called!");


    //Aspetto che finiscono le comunicazioni oppure il fuoco
    EventBits_t bits = xEventGroupWaitBits(
        syncBits, 
        BIT_COMMS_DONE | BIT_FIRE, // Watch for these bits
        pdFALSE,                   // Clear bits on exit? (False: we want to read them)
        pdFALSE,                   // WaitForAllBits? FALSE = "OR" Logic (Wake on ANY)
        portMAX_DELAY              // Block forever until one happens
    );

    if ((bits & BIT_FIRE) != 0) {
        // CASE A: FIRE DETECTED
        ESP_LOGE(MAIN_CPP_TAG, "FIRE BIT detected WHILE WORKING! Rebooting...");
        fire_reboot_triggered = true;
        esp_restart();
    } 
    else if ((bits & BIT_COMMS_DONE) != 0) {
        // CASE B: NORMAL COMPLETION
        ESP_LOGI(MAIN_CPP_TAG, "Tasks finished normally. Sleeping.");
    }

    //5. Hold fire pin and Sleep again
    deep_sleep_setup(SLEEP_FIXED_DURATION);
    esp_deep_sleep_start();

}

void loop() {}

void execute_fire_protocol() {
    //TODO: Testare e mettere se necessario un topic a parte
    // --- 1. LOCAL EMERGENCY OBJECTS ---
    WiFiClientSecure fireEspClient;
    PubSubClient fireMqttClient(fireEspClient);

    ESP_LOGE("FIRE", "!!! FIRE PROTOCOL INITIATED !!!");

    // Safety: Kill relays immediately
    digitalWrite(HEATER_PWM_PIN,RELAY_OFF);
    digitalWrite(HUMIF_PWM_PIN,RELAY_OFF);
    digitalWrite(FAN_HEATER_PIN,RELAY_OFF);
    i2c_setup(false); //non abbiamo bisogno dei sensori!
    resetIOExpander();

    ESP_LOGE("FIRE", "IO EXPANDER AND GPIO RELAYS DEACTIVATED!");

    WiFi.mode(WIFI_STA);
    fireEspClient.setInsecure(); // Skip certs for speed
    fireMqttClient.setServer(MQTT_BROKER, MQTT_PORT);

    String clientId = "ESP32-FIRE-" + String(random(0xffff), HEX);
    long lastPub = 0;

    // --- END GAME LOOP (Infinite) ---
    while(1) {
        // A. WIFI RECONNECT LOOP
        if (WiFi.status() != WL_CONNECTED) {
            ESP_LOGE("FIRE", "WiFi Lost. Reconnecting...");
            WiFi.disconnect();
            WiFi.begin(WIFI_SSID, WIFI_PASS);
            
            // Wait up to 5 seconds for WiFi, then loop around
            int retries = 0;
            while (WiFi.status() != WL_CONNECTED && retries < 10) {
                delay(500);
                Serial.print("!"); 
                retries++;
            }
        }

        // B. MQTT RECONNECT LOOP (Only if WiFi is good)
        if (WiFi.status() == WL_CONNECTED && !fireMqttClient.connected()) {
            ESP_LOGE("FIRE", "Connecting to MQTT Broker...");
            if (fireMqttClient.connect(clientId.c_str(), MQTT_USER, MQTT_PASS)) {
                ESP_LOGE("FIRE", "MQTT Connected!");
            } else {
                ESP_LOGE("FIRE", "MQTT Fail rc=%d. Wait 2s.", fireMqttClient.state());
                delay(2000);
            }
        }

        // C. THE BEACON (Continuous Publishing)
        if (fireMqttClient.connected()) {
            fireMqttClient.loop(); // Keep socket alive

            long now = millis();
            // Publish every 2 seconds
            if (now - lastPub > 2000) {
                lastPub = now;
                
                // Retained = true. 
                // We keep sending it so "Last Seen" timestamp on dashboard stays fresh.
                fireMqttClient.publish(TOPIC_PUB, "!!! FIRE DETECTED !!! SYSTEM HALTED.", true);
                ESP_LOGE("FIRE", "SOS BROADCAST SENT.");
                
                // Visual Panic (Fast blink)
                digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
            }
        }
        
        delay(10); // Yield to prevent Watchdog Timer crash
    }
}