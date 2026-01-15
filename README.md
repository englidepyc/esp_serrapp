# Per fare funzionare il tutto
## Per prima cosa 
Crea un file nella cartella `include/` e chiamalo `config.h`.<br>
Copia e incolla questo codice e inserisci le credenziali opportune, dopo potrai compilare ed eseguire il codice 
```cpp
#pragma once
// --- CREDENTIALS (FILL THESE) ---
#define WIFI_SSID       ""
#define WIFI_PASS       ""

// MQTT Configuration
#define MQTT_BROKER     "" 
#define MQTT_PORT       8883             // SSL Port
#define MQTT_USER       ""   // Keep empty if public broker
#define MQTT_PASS       ""  // Keep empty if public broker

// Topics
#define TOPIC_PUB       ""
#define TOPIC_CMD       ""
```
# Altri branch
I branch `test/` sono fatti a partire dal develop e implementano piccole funzionalità, ad esempio lo shift register, il deep sleep etc e hanno dei semplici esempi che li mostrano in azione in `src/main`.
<br>
Quando sei soddisfatto di avere sviluppato la tua piccola funzionalità nel tuo branch `test/`, caricalo con `git push --set-upstream origin test/tua_funzionalita`.
<br> Poi disctuiamo e proviamo e lo integriamo nel branch `develop`