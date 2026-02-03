# Per fare funzionare il tutto

## Per prima cosa

Copia il file `include/config.example.h` in un file chiamato `config.h` e inserisci le credenziali opportune, dopo potrai compilare ed eseguire il codice

# Altri branch

I branch `feat/` sono fatti a partire dal develop e implementano piccole funzionalità, ad esempio lo shift register, il deep sleep etc e possono avere dei semplici esempi che li mostrano in azione in `src/main.cpp`.
<br>
Quando sei soddisfatto di avere sviluppato la tua piccola funzionalità nel tuo branch `feat/`, caricalo con `git push --set-upstream origin feat/tua_funzionalita`.
<br> Poi disctuiamo e proviamo e lo integriamo nel branch `develop`

# Documentazione

La board custom è presa dal link [https://github.com/handledexception/platform-espressif32/blob/esp32-s3-devkitc-1-n16r8v/boards/esp32-s3-devkitc-1-n16r8v.json]

## Interfaccia hardware

I sensori e gli attuatori sono gestiti da interfacce hardware implementate in `src/real`. E' possibile cambiare l'environment PlatformIO da `release` a `test` per passare dalle vere implementazioni a quelle nella cartella `src/mocks` per sviluppare e testare senza hardware effettivo, non toccando i pin della board per evitare corti e malfunzionamenti, specialmente quando si settano i pin in modalità OUTPUT.

### i2c_mux_and_sensor_reads

IMPOSTA TUTTA LA COMUNICAZIONE I2C e gestisce la lettura dei sensori di temperatura e umidità (AHT20) tramite il multiplexer I2C TCA9548A.

**Funzioni principali:**

- `i2c_setup(bool enable_sensors)`: Inizializza il bus I2C e i sensori. Chiamare nel setup con `enable_sensors=true` per inizializzare anche i sensori AHT20.
- `temp_hum_scan_vpd(float &hum, float &temp, float &vpd)`: Legge tutti i sensori connessi al mux, calcola la media dei valori e determina il VPD (Vapor Pressure Deficit). Gestisce automaticamente letture non valide e include delay tra letture per evitare self-heating.
- `temp_hum_single_sensor(float &hum, float &temp, uint8_t sensor_bus)`: Legge un singolo sensore su uno specifico canale del mux.
- `calculate_vpd(float temp, float hum)`: Calcola il VPD in kPa da temperatura (°C) e umidità relativa (%).

**Nota:** La quantità di sensori è definita da `I2C_SENSORS_QTY` e il delay tra letture da `I2C_SENSOR_READ_DELAY_MS` nell'header.

### i2c_io_expander

Gestisce l'I/O expander PCF8574 via I2C per controllare pompe, ventole e il multiplexer analogico.

**Funzioni principali:**

- `resetIOExpander()`: Inizializza l'expander con tutti i relè spenti (stato HIGH = OFF per logica active-low). Chiamare nel setup dopo `i2c_setup()`.
- `setDevice(Device dev, bool state)`: Attiva/disattiva un dispositivo specifico. Dispositivi disponibili: `PUMP_P0`, `FAN_EXT_P1`. Usare `RELAY_ON`/`RELAY_OFF` come stato.
- `setAmuxChannel(byte channel)`: Seleziona il canale del multiplexer analogico (0-7) per le letture di umidità del terreno.
- `setAllOutputs(bool state)`: Imposta tutti gli output contemporaneamente.

**Nota:** I relè hanno logica active-low (LOW=ON, HIGH=OFF). Le costanti `RELAY_ON`/`RELAY_OFF` in `pins.h` gestiscono automaticamente questa logica.

### analog_moisture_reads

Gestisce la lettura dei sensori di umidità del terreno via ADC attraverso il multiplexer analogico.

**Funzioni principali:**

- `readMoisture_single(uint8_t analog_channel)`: Legge un singolo sensore sul canale specificato, restituisce la percentuale di umidità (0-100%).
- `readMoisture_scan()`: Legge tutti i sensori definiti in `ANALOG_SENSORS_QTY` e restituisce la media delle letture.

**Nota:** I valori di calibrazione `AIR_VALUE` e `WATER_VALUE` nell'header vanno regolati per i sensori specifici. Include delay di stabilizzazione tra letture consecutive.

### Relè e stati

L'unico relè SSR con il quale possiamo fare **pwm**, comunque lento data la frequenza di rete, è quello del **riscaldatore**.
La funzione `taskPWM()` rende possibile controllare qualsiasi pin, per esempio quello dell'**umidificatore**, qualora avessimo a disposizione nuovo hardware.
Gli stati `RELAY_ON` e `RELAY_OFF` sono definiti in `pins.h` dato che vanno usati sia dalla task che si occupa di fare il pwm che da banali `digitalWrite()` per gli altri dispositivi on/off.

### Cosa fare in main.cpp e nei file che usano l'interfaccia

- In `src/main.cpp` E' NECESSARIO chiamare sempre come sicurezza dal fuoco (oltre che setup del pin pwm e on/off direttamente connessi ai GPIO dell'esp) `pwmSetup()`.

- in `src/main.cpp` E' NECESSARIO impostare correttamente come INPUT con `pinMode()` i pin di input del GALLEGGIANTE del serbatoio, del risveglio dal sesnore FUOCO e delle letture analogiche dell'acqua nel TERRENO, nell'apposita sezione di `setup()`.

- in `src/main.cpp` E' NECSSARIO chiamare anche i vari metodi di setup delle restanti interfacce hardware, che inizializzano e testano le comunicazioni e impostano correttamente i pin. Questi sono `i2c_setup()` e `init_camera_system()`. Senza l'i2c setup non potremo leggere i sensori nè lavorare con l'io expander

## Ciclo di funzionamento

### Task main
La task principale, nel setup di `src/main.cpp`.
- Inizializza e configura l'hardware e l'interfaccia hardware, inclusa la taskPWM lenta che sarà usata da mango semplicemente variando la potenza con `PWM_HEATER_POW`. Spawna la task comms e finisce con il deep sleep.
- Gestisce l'emergenza del fuoco tramite una flag persistente, rilevato sia durante il deep sleep sia durante la 'veglia' tramite isr, rebootando l'esp. in ciascun caso non crea nessun altra task.
- Vengono esposti a tutti per sicurezza e per sviluppo, tramite l'header, variabili importanti quali quelle di rtos, anche venissero usate/ inizializzate dalle altre task 

### Task comms
- La task si connette al broker Mqtt sopra WiFi e sovrascrive il CmdMessage di default salvato in RTC e definito e inizializzato in `src/main.cpp`. Questo permette al tutto di continuare a funzionare anche in caso di mancata connessione, poi la task decide come proseguire, creando la taskMango con gli opportuni parametri della richiesta foto e della modalità _eco_ con cicli di deep sleep oppure _precision_ costantemente attiva
- Si occupa di inviare tramite mqtt tutti i dettagli delle misurazioni e del lavoro di taskMango
- DA IMPLEMENTARE la task si occupa anche di mandare la foto scattata da taskMango mentre taskMango continua a lavorare alla serra

### Task mango
DA IMPLEMENTARE

### Deep sleep
La funzione `deep_sleep_setup()` viene chiamata prima di avviare lo sleep nel setup di `src/main.cpp` per impostare correttamente il tempo di sleep

## Controllo della serra

### Controllo proporzionale TPC (con deep sleep)

Nell'header `include/control/proportional.h` è possibile settare i vari parametri per il controllo tpc, che viene implementato nell'apposita sorgente e utilizzato da `src/taskMango.cpp`, tramite l'apposita proportional.cpp