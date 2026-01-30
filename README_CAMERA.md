# 📸 Camera API - Documentazione

## Panoramica

L'interfaccia camera fornisce un'API unificata per la gestione della camera OV5640 con supporto per:

- **Versione REAL**: Controlla hardware reale (pin GPIO, sensore OV5640)
- **Versione MOCK**: Simulazione completa senza hardware

## 🔧 Come selezionare Real vs Mock

La selezione tra versione reale e mock avviene a **compile-time** tramite PlatformIO.

### Configurazione in `platformio.ini`

```ini
[env:release]
# Versione REAL - usa hardware reale
build_flags =
    -D USE_REAL_HW=1
build_src_filter =
    +<*>
    -<mocks/>
    +<real/>

[env:mock]
# Versione MOCK - simulazione senza hardware
build_flags =
    -D USE_REAL_HW=0
build_src_filter =
    +<*>
    -<real/>
    +<mocks/>
```

### Build e Upload

```bash
# Compila e carica versione REAL (hardware)
pio run -e release --target upload

# Compila e carica versione MOCK (simulazione)
pio run -e mock --target upload
```

## 📚 API Reference

### Inizializzazione

```cpp
#include "camera.h"

// Inizializzazione con configurazione default
bool camera_init();

// Inizializzazione con configurazione personalizzata
CameraConfig config;
config.frame_size = FRAMESIZE_SVGA;    // 800x600
config.jpeg_quality = 12;              // 0-63 (più basso = migliore)
config.fb_count = 1;                   // Numero di buffer
bool camera_init_custom(config);
```

### Cattura Frame

```cpp
CameraFrame frame;

// Cattura con timeout default (5 secondi)
if (camera_capture_frame(&frame)) {
    // Usa frame.buffer (dati JPEG)
    // Usa frame.length (dimensione bytes)
    // Usa frame.width e frame.height

    // IMPORTANTE: rilascia SEMPRE il frame
    camera_release_frame(&frame);
}

// Cattura con timeout custom (3 secondi)
if (camera_capture_frame(&frame, 3000)) {
    // ... usa frame ...
    camera_release_frame(&frame);
}
```

### Verifica Stato

```cpp
// Verifica se camera è pronta
if (camera_is_ready()) {
    // Camera inizializzata e funzionante
}

// Ottieni configurazione corrente
CameraConfig config;
if (camera_get_config(&config)) {
    Serial.printf("Frame size: %d\n", config.frame_size);
    Serial.printf("Quality: %d\n", config.jpeg_quality);
}
```

### Cleanup

```cpp
// Deinicializza camera e libera risorse
camera_deinit();
```

## 🎯 Pattern d'uso comuni

### 1. Uso Base

```cpp
void setup() {
    if (!camera_init()) {
        Serial.println("Errore inizializzazione camera");
        return;
    }

    CameraFrame frame;
    if (camera_capture_frame(&frame)) {
        Serial.printf("Foto: %d bytes\n", frame.length);
        camera_release_frame(&frame);
    }
}
```

### 2. Task RTOS

```cpp
void camera_task(void *param) {
    camera_init();

    while (true) {
        CameraFrame frame;
        if (camera_capture_frame(&frame, 3000)) {
            // Elabora frame
            camera_release_frame(&frame);
        }
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

void setup() {
    xTaskCreate(camera_task, "Camera", 4096, NULL, 5, NULL);
}
```

### 3. Web Server Integration

```cpp
#include <WebServer.h>

WebServer server(80);

void handleCapture() {
    if (!camera_is_ready()) {
        server.send(503, "text/plain", "Camera non disponibile");
        return;
    }

    CameraFrame frame;
    if (!camera_capture_frame(&frame)) {
        server.send(500, "text/plain", "Errore cattura");
        return;
    }

    server.send_P(200, "image/jpeg",
                  (const char*)frame.buffer, frame.length);

    camera_release_frame(&frame);
}

void setup() {
    camera_init();
    server.on("/capture", handleCapture);
    server.begin();
}
```

### 4. RAII Pattern (C++)

```cpp
class CameraFrameGuard {
    CameraFrame frame;
    bool valid;
public:
    CameraFrameGuard() : valid(camera_capture_frame(&frame)) {}
    ~CameraFrameGuard() { if (valid) camera_release_frame(&frame); }

    bool isValid() const { return valid; }
    const CameraFrame* get() const { return valid ? &frame : nullptr; }
};

// Uso:
void capture_photo() {
    CameraFrameGuard guard;
    if (guard.isValid()) {
        auto frame = guard.get();
        // Usa frame...
    } // Rilascio automatico
}
```

## 🔍 Differenze Real vs Mock

| Aspetto         | Real                                            | Mock                        |
| --------------- | ----------------------------------------------- | --------------------------- |
| **Hardware**    | Usa GPIO e sensore OV5640                       | Nessun pin toccato          |
| **Dati**        | Frame JPEG reali dalla camera                   | Frame JPEG fittizi generati |
| **Timing**      | Dipende dal sensore (~30-100ms)                 | Simulato (10-50ms)          |
| **PSRAM**       | Usata se disponibile                            | Simula uso PSRAM            |
| **Errori**      | Possibili errori hardware reali                 | Simula errori occasionali   |
| **Temperatura** | Camera può scaldarsi                            | Nessun effetto hardware     |
| **Sensore**     | `camera_get_sensor()` restituisce sensore reale | Restituisce NULL            |

## ⚠️ Note Importanti

### Thread Safety

- ✅ Tutte le funzioni sono thread-safe
- ✅ Usa mutex interno per proteggere accesso concorrente
- ✅ Sicuro chiamare da task RTOS multiple

### Memory Management

- ⚠️ **SEMPRE** rilasciare frame con `camera_release_frame()`
- ⚠️ Non rilasciare un frame più di una volta
- ⚠️ Non usare frame dopo averlo rilasciato

### Limitazioni Mock

- Non testa problemi hardware reali
- Frame generati hanno pattern riconoscibile
- Utile per sviluppo logica senza hardware

## 🐛 Debug

### Log Levels

```cpp
// In platformio.ini
build_flags =
    -D CORE_DEBUG_LEVEL=4  ; 0=None, 3=Info, 4=Debug, 5=Verbose
```

### Log Output

**Real:**

```
[CAMERA] Inizializzazione camera OV5640...
[CAMERA] Sensore rilevato: PID=0x5640
[CAMERA] PSRAM rilevata - buffer in PSRAM
[CAMERA] Sensore OV5640 configurato
[CAMERA] Warm-up sensore...
[CAMERA] ✓ Camera pronta [800x600, Q=12, FB=1]
[CAMERA] ✓ Cattura OK: 45234 bytes, 800x600
```

**Mock:**

```
[MOCK/CAMERA] Inizializzazione MOCK camera (nessun hardware)
[MOCK/CAMERA] ✓ Mock camera pronta [simulato: 800x600, Q=12, FB=1]
[MOCK/CAMERA] ⚠️  NESSUN PIN HARDWARE UTILIZZATO - SOLO SIMULAZIONE
[MOCK/CAMERA] ✓ Mock cattura #1: 48000 bytes, 800x600 (simulato in 23ms)
```

## 📝 Esempi Completi

Vedi [examples/camera_usage_example.cpp](examples/camera_usage_example.cpp) per 10 pattern d'uso completi.

## 🔗 Hardware Reference

### Pin OV5640 (ESP32-S3)

| Segnale    | GPIO | Note           |
| ---------- | ---- | -------------- |
| XCLK       | 15   | Clock          |
| SIOD (SDA) | 4    | I2C Data       |
| SIOC (SCL) | 5    | I2C Clock      |
| Y9 (D7)    | 18   | Data bit 7     |
| Y8 (D6)    | 17   | Data bit 6     |
| Y7 (D5)    | 16   | Data bit 5     |
| Y6 (D4)    | 12   | Data bit 4     |
| Y5 (D3)    | 10   | Data bit 3     |
| Y4 (D2)    | 8    | Data bit 2     |
| Y3 (D1)    | 9    | Data bit 1     |
| Y2 (D0)    | 11   | Data bit 0     |
| VSYNC      | 6    | Vertical Sync  |
| HREF       | 7    | Horizontal Ref |
| PCLK       | 13   | Pixel Clock    |

### Requisiti

- ESP32-S3 con PSRAM (consigliato)
- Alimentazione stabile 5V 2A
- Camera OV5640 compatibile

## 🚀 Quick Start

```cpp
#include "camera.h"

void setup() {
    Serial.begin(115200);

    // Inizializza camera (real o mock a seconda della compilazione)
    if (camera_init()) {
        Serial.println("Camera pronta!");
    } else {
        Serial.println("Errore camera!");
    }
}

void loop() {
    CameraFrame frame;

    if (camera_capture_frame(&frame)) {
        Serial.printf("Frame: %d bytes\n", frame.length);
        camera_release_frame(&frame);
    }

    delay(5000);
}
```

---

**Versione API:** 1.0  
**Compatibilità:** ESP32-S3 + OV5640  
**Licenza:** Progetto interno
