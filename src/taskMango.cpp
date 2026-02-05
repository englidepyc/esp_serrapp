constexpr static const char* TASKS_CPP_MANGO_TAG = "TASK_MANGO.CPP";
#include <Arduino.h>

#include "config.h"
#include "common_hw.h"
#include "control/proportional.h"
#include "taskComms.h"
#include "taskMango.h"
#include "main.h"

RTC_DATA_ATTR float currentWaterCalcLevel = 100.0;

// --- TASK A: GARDEN LOGIC (Core 1) ---
void taskMango(void *pvParameters){   
    GardenMessage msg {};
    
    // pvParameters viene usato solo come flag: non serve heap
    bool isEcoMode = (pvParameters != nullptr);
    msg.errCode = 0;


    
    if (isEcoMode) {
    //******* MODALITA' ECO TPC ********

        //**PRIMA**
        msg.isActuated = false;
        
        msg.isWaterOver = !(digitalRead(FLOAT_SWITCH_PIN)); //logica negata
        if (msg.isWaterOver) {
            currentWaterCalcLevel = 0.0f;
            ESP_LOGE(TASKS_CPP_MANGO_TAG, "Acqua finita dal float switch!");
        }
        // Sincronizza il livello calcolato corrente con il messaggio "prima" irrigazione
        msg.waterCalcLevel = currentWaterCalcLevel;
        temp_hum_scan_vpd(msg.hum, msg.temp, msg.vpd);
        msg.moisture = readMoisture_scan();

        // Calcola il tempo totale di attuazione (ms) e traduci in secondi per nextMessageTime
        long totalMs = proportional_calculate_total_time(&msg);
        int totalS = (totalMs + 999) / 1000; // arrotonda per eccesso
        msg.nextMessageTime = totalS;

        xQueueSend(dataQueue, &msg, pdMS_TO_TICKS(100));  
 

            //***ATTUAZIONE***/


            ESP_LOGI(TASKS_CPP_MANGO_TAG, "Scan Result BEFORE actuation -> T:%.1f H:%.1f VPD:%.2f Soil:%d%% (fixed) Tank:%.1f", 
                msg.temp, msg.hum, msg.vpd, msg.moisture, msg.waterCalcLevel);
            
            proportional_actuation(&msg);

            ESP_LOGI(TASKS_CPP_MANGO_TAG, "Scan Result AFTER actuation -> T:%.1f H:%.1f VPD:%.2f Soil:%d%% (fixed) Tank:%.1f", 
                msg.temp, msg.hum, msg.vpd, msg.moisture, msg.waterCalcLevel);


        //**DOPO**
        msg.isActuated = true;

        msg.isWaterOver = !(digitalRead(FLOAT_SWITCH_PIN)); //logica negata
        if (msg.isWaterOver) {
            currentWaterCalcLevel = 0.0f;
        }
        // Allinea nuovamente il livello acqua per il messaggio "dopo" irrigazione
        msg.waterCalcLevel = currentWaterCalcLevel;
        temp_hum_scan_vpd(msg.hum, msg.temp, msg.vpd);
        msg.moisture = readMoisture_scan();
        msg.nextMessageTime = SLEEP_FIXED_DURATION;
        xQueueSend(dataQueue, &msg, pdMS_TO_TICKS(100));

    
    
    
    } else {
        //*** PRECISION MODE ***/
        ESP_LOGD(TASKS_CPP_MANGO_TAG, "Modalita' PRECISION attivata in taskMango");
    
    /*
        //setup tempo
        while(1){
            //aggiorno tempo
            //PID_TEMPERATURA



            if(/*sono passati 10 minuti*/            /*){
                //Logica misura moisture e irrigazione, ricorda di controllare se l'acqua è finita con logica negata
                //MANDA messaggio a taskComms che lo invierà MQTT, setta il feedback isModePrecision = true
            }

            if(/*live_mode*/                          /*){
                //Manda GardenMessage con misurazioni a taskComms magari con un delay di 10 secondi (inferiore al ciclo del pid)
            }
        }
    }
    
    */

    /*
        // Parametri PID - Da calibrare in base al sistema
        double Kp = 2.0; 
        double Ki = 5.0; 
        double Kd = 1.0;

        unsigned long lastTime;
        double input, output, setpoint;
        double errSum, lastErr;

        void setup() {
        Serial.begin(9600);
        
        setpoint = 100; // Il valore che vogliamo raggiungere
        lastTime = millis();
        }

        void loop() {
        unsigned long now = millis();
        double timeChange = (double)(now - lastTime);

            // Calcoliamo solo se è passato del tempo (es. ogni 100ms)
            if(timeChange >= 100) {
                input = analogRead(A0); // Lettura sensore
                
                // 1. Calcolo dell'errore
                double error = setpoint - input;
                
                // 2. Calcolo Integrale (somma degli errori nel tempo)
                errSum += (error * timeChange);
                
                // 3. Calcolo Derivativa (variazione dell'errore)
                double dErr = (error - lastErr) / timeChange;

                // 4. Calcolo Output PID
                output = Kp * error + Ki * errSum + Kd * dErr;

                // Limiti di sicurezza per l'uscita (PWM 0-255)
                if(output > 255) output = 255;
                else if(output < 0) output = 0;

                analogWrite(9, output); // Applica l'azione di controllo

                // Salvataggio variabili per il prossimo ciclo
                lastErr = error;
                lastTime = now;

                Serial.print("Input: "); Serial.print(input);
                Serial.print(" -> Output: "); Serial.println(output);
            
            } */
        }  
   

    

    
    
    vTaskDelete(NULL);
}

//updates the currentWaterCalcLevel to the GardenMsg
void updateCalcWaterLevel(float& waterLevel_percentage, int duration_s){
    currentWaterCalcLevel = currentWaterCalcLevel - (K_WATER * duration_s);
    waterLevel_percentage = currentWaterCalcLevel;
}