#include <PubSubClient.h>
#include <WiFiClientSecure.h>
#include <WiFi.h>
#include "config.h"
//https://www.hivemq.com/demos/websocket-client/

/*config.h file example (fill with your passowords):
// --- CONFIGURAZIONE WIFI ---
const char* ssid = "your-wifi-ssid"
const char* password = "your-wifi-password"

// --- CONFIGURAZIONE MQTT (Broker Pubblico) ---
const char* mqtt_server = "your-mqtt-server"
int mqtt_port = 8883; //1883 unencrypted
const char* userId = "your-user-id"
const char* userPassword = "your-user-password"
const char* topic_sub = "topic/path"; 
const char* topic_pub = "topic/path";
*/

WiFiClientSecure espClient; //ESP32

PubSubClient client(espClient);
const int ledPin = 14; // LED integrato
// --- GESTIONE MESSAGGI IN ARRIVO (CALLBACK) ---
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Messaggio arrivato [");
  Serial.print(topic);
  Serial.print("]: ");

  // Trasformiamo il payload in stringa per leggerlo meglio
  String message;
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.println(message);

  // LOGICA SEMPLICE DI COMANDO
  if (message == "ON") {
    digitalWrite(ledPin, HIGH); // Accendi
    Serial.println("LED ACCESO");
  } else if (message == "OFF") {
    digitalWrite(ledPin, LOW);  // Spegni
    Serial.println("LED SPENTO");
  }
}

void setup() {
  pinMode(ledPin, OUTPUT);
  Serial.begin(115200);

  //per adesso non chiediamo il certificato al server
  espClient.setInsecure();

  // Connessione WiFi
  Serial.print("Connessione WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" Connesso!");

  // Configura MQTT
  client.setServer(mqtt_server, mqtt_port); // Porta standard non criptata
  client.setCallback(callback);        // Chi chiamare quando arriva un messaggio
}

void reconnect() {
  // Loop finché non siamo connessi
  while (!client.connected()) {
    Serial.print("Tentativo connessione MQTT...");
    // Crea un ID client random
    String clientId = "ESP32Client-Min414";
    //clientId += String(random(0xffff), HEX);
    

    if (client.connect(clientId.c_str(), userId, userPassword)) {
      Serial.print("Connesso con id:");
      Serial.println(clientId);
      // Appena connesso, mi iscrivo al topic dei comandi
      client.subscribe(topic_sub);
    } else {
      Serial.print("Fallito, rc=");
      Serial.print(client.state());
      Serial.println(" riprovo tra 5 secondi");
      delay(5000);
    }
  }
}

long lastMsg = 0;

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop(); // Fondamentale: controlla se ci sono messaggi in arrivo!

  // Invia temperatura ogni 5 secondi (senza bloccare con delay lunghi)
  long now = millis();
  if (now - lastMsg > 5000) {
    lastMsg = now;
    
    // Simula temperatura tra 20 e 30 gradi
    float temp = 20.0 + (random(100) / 10.0);
    
    // Converti float in stringa
    char tempString[8];
    dtostrf(temp, 1, 2, tempString);
    
    Serial.print("Invio temperatura: ");
    Serial.println(tempString);
    
    // PUBBLICA
    client.publish(topic_pub, tempString);
  }
}