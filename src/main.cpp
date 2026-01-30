/*
 * ESP32-S3 Camera Web Server
 * Scatta una foto e la visualizza tramite un semplice web server HTTP
 */

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include "esp_camera.h"
#include "config.h"

// Configurazione camera (basata su .copilot-settings.md)
#define CAM_PIN_PWDN    -1  // Non usato
#define CAM_PIN_RESET   -1  // Non usato
#define CAM_PIN_XCLK    15
#define CAM_PIN_SIOD    4   // SDA
#define CAM_PIN_SIOC    5   // SCL
#define CAM_PIN_D7      18  // Y9
#define CAM_PIN_D6      17  // Y8
#define CAM_PIN_D5      16  // Y7
#define CAM_PIN_D4      12  // Y6
#define CAM_PIN_D3      10  // Y5
#define CAM_PIN_D2      8   // Y4
#define CAM_PIN_D1      9   // Y3
#define CAM_PIN_D0      11  // Y2
#define CAM_PIN_VSYNC   6
#define CAM_PIN_HREF    7
#define CAM_PIN_PCLK    13

WebServer server(80);
bool cameraInitialized = false;

// Inizializza la camera
bool initCamera() {
    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = CAM_PIN_D0;
    config.pin_d1 = CAM_PIN_D1;
    config.pin_d2 = CAM_PIN_D2;
    config.pin_d3 = CAM_PIN_D3;
    config.pin_d4 = CAM_PIN_D4;
    config.pin_d5 = CAM_PIN_D5;
    config.pin_d6 = CAM_PIN_D6;
    config.pin_d7 = CAM_PIN_D7;
    config.pin_xclk = CAM_PIN_XCLK;
    config.pin_pclk = CAM_PIN_PCLK;
    config.pin_vsync = CAM_PIN_VSYNC;
    config.pin_href = CAM_PIN_HREF;
    config.pin_sccb_sda = CAM_PIN_SIOD;
    config.pin_sccb_scl = CAM_PIN_SIOC;
    config.pin_pwdn = CAM_PIN_PWDN;
    config.pin_reset = CAM_PIN_RESET;
    config.xclk_freq_hz = 20000000;
    config.pixel_format = PIXFORMAT_JPEG;
    
    // Configurazione qualità immagine
    if(psramFound()) {
        config.frame_size = FRAMESIZE_UXGA;  // 1600x1200
        config.jpeg_quality = 10;            // 0-63, più basso = migliore qualità
        config.fb_count = 2;
        config.fb_location = CAMERA_FB_IN_PSRAM;
        config.grab_mode = CAMERA_GRAB_LATEST;
        Serial.println("PSRAM trovata - Configurazione alta qualità");
    } else {
        config.frame_size = FRAMESIZE_SVGA;  // 800x600
        config.jpeg_quality = 12;
        config.fb_count = 1;
        config.fb_location = CAMERA_FB_IN_DRAM;
        Serial.println("PSRAM non trovata - Configurazione base");
    }
    
    // Inizializza camera
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        Serial.printf("Errore inizializzazione camera: 0x%x\n", err);
        return false;
    }
    
    Serial.println("Camera inizializzata con successo!");
    return true;
}

// Handler per la pagina principale
void handleRoot() {
    String html = R"HTML(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP32-S3 Camera</title>
    <style>
        body { 
            font-family: Arial, sans-serif; 
            text-align: center; 
            margin: 20px;
            background-color: #f0f0f0;
        }
        h1 { color: #333; }
        button {
            background-color: #4CAF50;
            color: white;
            padding: 15px 32px;
            font-size: 16px;
            border: none;
            border-radius: 4px;
            cursor: pointer;
            margin: 10px;
        }
        button:hover { background-color: #45a049; }
        #photo {
            max-width: 100%;
            height: auto;
            margin-top: 20px;
            border: 2px solid #333;
        }
        .info {
            background-color: #fff;
            padding: 15px;
            border-radius: 5px;
            margin: 20px auto;
            max-width: 600px;
        }
    </style>
</head>
<body>
    <h1>ESP32-S3 Camera Web Server</h1>
    <div class="info">
        <p><strong>Stato:</strong> Camera attiva</p>
        <p>Clicca su "Scatta Foto" per catturare un'immagine</p>
    </div>
    <button onclick="capturePhoto()">Scatta Foto</button>
    <button onclick="location.reload()">Ricarica Pagina</button>
    <br>
    <img id="photo" src="" alt="Nessuna foto catturata" style="display:none;">
    
    <script>
        function capturePhoto() {
            document.getElementById('photo').style.display = 'none';
            fetch('/capture')
                .then(response => response.blob())
                .then(blob => {
                    const url = URL.createObjectURL(blob);
                    const img = document.getElementById('photo');
                    img.src = url;
                    img.style.display = 'block';
                })
                .catch(err => alert('Errore: ' + err));
        }
    </script>
</body>
</html>
)HTML";
    server.send(200, "text/html", html);
}

// Handler per catturare la foto
void handleCapture() {
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
        Serial.println("Errore: impossibile catturare foto");
        server.send(500, "text/plain", "Errore cattura foto");
        return;
    }
    
    Serial.printf("Foto catturata: %d bytes, %dx%d\n", fb->len, fb->width, fb->height);
    
    server.sendHeader("Content-Disposition", "inline; filename=capture.jpg");
    server.send_P(200, "image/jpeg", (const char *)fb->buf, fb->len);
    
    esp_camera_fb_return(fb);
}

void setup() {
    Serial.begin(115200);
    Serial.println("\n\n=== ESP32-S3 Camera Web Server ===");
    
    // Connessione WiFi
    Serial.printf("Connessione a %s...\n", WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("\nERRORE: Impossibile connettersi al WiFi!");
        Serial.println("Verifica le credenziali in config.h");
        return;
    }
    
    Serial.println("\nWiFi connesso!");
    Serial.print("Indirizzo IP: ");
    Serial.println(WiFi.localIP());
    
    // Inizializza camera
    cameraInitialized = initCamera();
    if (!cameraInitialized) {
        Serial.println("ERRORE: Inizializzazione camera fallita!");
        return;
    }
    
    // Configura web server
    server.on("/", handleRoot);
    server.on("/capture", handleCapture);
    server.begin();
    
    Serial.println("\n=== Server avviato! ===");
    Serial.printf("Apri il browser e vai a: http://%s\n", WiFi.localIP().toString().c_str());
}

void loop() {
    if (cameraInitialized && WiFi.status() == WL_CONNECTED) {
        server.handleClient();
    }
}

/*
 * ============================================================================
 * COME ACCEDERE AL WEB SERVER:
 * ============================================================================
 * 
 * 1. CONFIGURAZIONE INIZIALE:
 *    - Apri include/config.h
 *    - Imposta WIFI_SSID con il nome della tua rete WiFi
 *    - Imposta WIFI_PASS con la password della tua rete WiFi
 * 
 * 2. CARICA IL PROGRAMMA:
 *    - Compila e carica il codice sull'ESP32-S3
 *    - Apri il Serial Monitor (115200 baud)
 *    - Attendi la connessione WiFi
 * 
 * 3. OTTIENI L'INDIRIZZO IP:
 *    - Nel Serial Monitor vedrai: "Indirizzo IP: 192.168.x.x"
 *    - Annota questo indirizzo IP
 * 
 * 4. ACCEDI AL WEB SERVER:
 *    - Assicurati che il tuo computer/smartphone sia sulla STESSA rete WiFi
 *    - Apri un browser web (Chrome, Firefox, Safari, ecc.)
 *    - Nella barra degli indirizzi digita: http://192.168.x.x
 *      (sostituisci con l'IP che hai visto nel Serial Monitor)
 * 
 * 5. USA L'INTERFACCIA:
 *    - Clicca sul pulsante "📷 Scatta Foto" per catturare un'immagine
 *    - L'immagine apparirà nella pagina web
 *    - Puoi scattare nuove foto cliccando di nuovo il pulsante
 * 
 * ============================================================================
 * COSA FA QUESTO CODICE:
 * ============================================================================
 * 
 * 1. INIZIALIZZAZIONE:
 *    - Si connette alla rete WiFi configurata in config.h
 *    - Inizializza la camera OV2640 con i pin corretti
 *    - Configura il frame buffer in PSRAM (8MB) per immagini ad alta risoluzione
 *    - Avvia un web server HTTP sulla porta 80
 * 
 * 2. WEB SERVER:
 *    - Route "/" (pagina principale): mostra l'interfaccia HTML con pulsanti
 *    - Route "/capture": cattura una foto JPEG e la invia al browser
 * 
 * 3. CATTURA FOTO:
 *    - Quando premi il pulsante, il browser fa una richiesta a /capture
 *    - L'ESP32 usa esp_camera_fb_get() per catturare un frame dalla camera
 *    - Il frame è in formato JPEG (già compresso)
 *    - Viene inviato direttamente al browser come immagine
 *    - Il buffer viene liberato dopo l'invio
 * 
 * 4. CONFIGURAZIONE CAMERA:
 *    - Formato: JPEG (compresso, adatto per web)
 *    - Risoluzione: UXGA (1600x1200) se PSRAM disponibile
 *    - Qualità JPEG: 10 (0=migliore, 63=peggiore)
 *    - Double buffering per acquisizione più fluida
 *    - Frame buffer allocato in PSRAM per non saturare la SRAM interna
 * 
 * 5. VANTAGGI DI QUESTA IMPLEMENTAZIONE:
 *    - Codice minimalista: ~220 righe totali
 *    - Non usa librerie complesse o async web server
 *    - Gestione memoria ottimizzata con PSRAM
 *    - Interfaccia web responsive e moderna
 *    - Facile da debuggare e modificare
 * 
 * ============================================================================
 * TROUBLESHOOTING:
 * ============================================================================
 * 
 * PROBLEMA: "Impossibile connettersi al WiFi"
 * SOLUZIONE: Verifica WIFI_SSID e WIFI_PASS in config.h
 * 
 * PROBLEMA: "Inizializzazione camera fallita"
 * SOLUZIONE: Controlla i collegamenti hardware della camera
 * 
 * PROBLEMA: "Errore cattura foto"
 * SOLUZIONE: Verifica che la PSRAM sia abilitata in platformio.ini:
 *            board_build.arduino.memory_type = qio_opi
 * 
 * PROBLEMA: Non riesco ad accedere al server dal browser
 * SOLUZIONE: Assicurati di essere sulla stessa rete WiFi dell'ESP32
 * 
 * ============================================================================
 */