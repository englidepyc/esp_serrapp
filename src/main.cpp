#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/event_groups.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "config.h"
#include "types.h"
#include "pins.h"

#include "mocks/i2c_temp_hum_reads.h"
#include "mocks/camera.h"
#include "mocks/shift_register.h"
#include "deep_sleep.h"

static const char* MAIN_CPP_TAG = "MAIN.CPP";

//ISR e flag per fuoco
volatile RTC_NOINIT_ATTR bool fire_reboot_triggered;
SemaphoreHandle_t fireSemaphore;
void IRAM_ATTR fire_isr() {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(fireSemaphore, &xHigherPriorityTaskWoken);
    
    // CRITICAL: This forces the CPU to jump to taskPanic IMMEDIATELY
    // We don't wait for the next tick. We switch context NOW.
    if (xHigherPriorityTaskWoken) {
        portYIELD_FROM_ISR();
    }
}

//prototipi
void execute_fire_protocol();
void taskPanic(void *pvParameters);
void taskMango(void *pvParameters);
void taskComms(void *pvParameters);

// Globals for Task Coordination
QueueHandle_t dataQueue;
EventGroupHandle_t syncBits;
#define BIT_COMMS_DONE (1 << 0)

void setup() {
    Serial.begin(115200);
    esp_log_level_set("*", ESP_LOG_DEBUG);
    
    initShiftRegister(); //prima cosa spegnere sempre il relé (sicurezza)

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
    
    
    //impostiamo pin acuqa e fuoco e mettiamo l'interrupt che setta flag reboot fuoco durante la fase "da sveglio"
    rtc_gpio_hold_dis((gpio_num_t)WAKE_PIN); //sblocca il pin che era stato attaccato all'rtc deep sleep per accesso durante la veglia
    pinMode(WAKE_PIN, INPUT_PULLDOWN);
    attachInterrupt(digitalPinToInterrupt(WAKE_PIN), fire_isr, RISING);
    pinMode(FLOAT_SWITCH_PIN,INPUT_PULLUP);    

    i2c_setup();
    init_camera_system();

    
    // 2. Create RTOS Objects
    dataQueue = xQueueCreate(5, sizeof(GardenMessage));
    syncBits = xEventGroupCreate();

    // 3. Launch Workers
    // Main Logic on Core 1, Comms on Core 0, task del reboot in caso fuoco con massima priorità
    fireSemaphore = xSemaphoreCreateBinary();
    xTaskCreatePinnedToCore(taskPanic, "Panic", 2048, NULL, configMAX_PRIORITIES - 1, NULL, 1);
    xTaskCreatePinnedToCore(taskMango,  "Mango",  8192, NULL, 1, NULL, 1);
    xTaskCreatePinnedToCore(taskComms, "Comms", 8192, NULL, 1, NULL, 0);

    ESP_LOGI(MAIN_CPP_TAG,"Task creation called!");


    EventBits_t bits = xEventGroupWaitBits(
        syncBits, 
        BIT_COMMS_DONE, 
        pdFALSE,        // Don't clear bits here
        pdTRUE,         // Wait for all bits
        portMAX_DELAY  // <--- The timeout allows us to loop back and check fire
    );

    //5. Hold fire pin and Sleep again
    deep_sleep_setup(10);
    esp_deep_sleep_start();

}

void loop() {}

// 2. The Guardian Task (The Executioner)
void taskPanic(void *pvParameters) {
    // This task sits here forever, blocked, using 0 CPU.
    xSemaphoreTake(fireSemaphore, portMAX_DELAY);
    
    // If we get here, the ISR fired.
    ESP_LOGE("PANIC", "Fire detected! Preempting everything and restarting...");
    
    // Set the persistent flag so we know to scream on reboot
    fire_reboot_triggered = true; 
    
    esp_restart();
}


void execute_fire_protocol() {
    //TODO: Testare e mettere se necessario un topic a parte
    // --- 1. LOCAL EMERGENCY OBJECTS ---
    WiFiClientSecure fireEspClient;
    PubSubClient fireMqttClient(fireEspClient);

    ESP_LOGE("FIRE", "!!! FIRE PROTOCOL INITIATED !!!");

    // Safety: Kill relays immediately
    // write_shift_register(0x00); 

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