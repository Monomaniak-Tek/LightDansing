#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include "webpage.h"
#include "modelePersonalise.h" // contient Frame modele[] et NUM_FRAMES

#define LED_EMETTEUR 2

WebServer server(80);

// Variables
bool modeleActif = false;        // mode externe actif ou non
int frameIndex = 0;
unsigned long nextFrameAtMs = 0;
uint8_t modeleMaskActif = 0xFF;

// Broadcast
uint8_t broadcastMAC[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};

// =====================
// playModele : envoie les frames du modèle sans bloquer loop()
// =====================
void playModele() {
    if (!modeleActif) return;

    unsigned long now = millis();
    if ((long)(now - nextFrameAtMs) < 0) return;

    if (frameIndex >= NUM_FRAMES) {
        modeleActif = false;
        uint8_t finMsg[2] = {modeleMaskActif, 201}; // 201 = fin modele externe
        esp_now_send(broadcastMAC, finMsg, 2);
        Serial.println("FIN MODELE");
        return;
    }

    Serial.print("Frame ");
    Serial.print(frameIndex);
    Serial.print(" duree=");
    Serial.println(modele[frameIndex].durationMs);

    uint8_t msg[5] = {
        modeleMaskActif,
        modele[frameIndex].r,
        modele[frameIndex].g,
        modele[frameIndex].b,
        modele[frameIndex].brightness
    };

    esp_now_send(broadcastMAC, msg, 5);
    Serial.println("ENVOI COULEUR");

    nextFrameAtMs = now + modele[frameIndex].durationMs;
    frameIndex++;
}

// =====================
// sendESPNow : envoie un masque et un effet
// =====================

void sendESPNow(uint8_t masque, uint8_t effet) {

    Serial.print("Envoi masque=");
    Serial.print(masque);
    Serial.print(" effet=");
    Serial.println(effet);

    // Toujours envoyer le masque + effet
    uint8_t msg[2] = {masque, effet};
    esp_now_send(broadcastMAC, msg, 2);

    if(effet == 200) {
        Serial.println("Activation modele externe");
        modeleActif = true;
        modeleMaskActif = masque;
        frameIndex = 0;
        nextFrameAtMs = millis() + 50; // laisse le temps aux récepteurs sans bloquer
        Serial.println("Lecture modele...");
    }
    else {
        modeleActif = false;
    }
}

// =====================
// handleSend : gestion HTTP GET
// =====================
void handleSend() {
    if (!server.hasArg("g") || !server.hasArg("e")) {
        server.send(400, "text/plain", "Paramètres manquants");
        return;
    }
    uint8_t masque = server.arg("g").toInt();
    uint8_t effet  = server.arg("e").toInt();
    sendESPNow(masque, effet);

    server.send(200, "text/plain", "OK");
}

// =====================
// setup : initialisation
// =====================
void setup() {
    Serial.begin(115200);
    delay(500);

    // LED émetteur
    pinMode(LED_EMETTEUR, OUTPUT);
    digitalWrite(LED_EMETTEUR, HIGH);

    Serial.println("=== Émetteur LED APA102 ===");

    // Wi-Fi AP + STA
    WiFi.mode(WIFI_AP_STA);            
    WiFi.softAP("DANSE", NULL, 1);     
    WiFi.disconnect();                  

    IPAddress ip = WiFi.softAPIP();
    Serial.print("IP AP: "); Serial.println(ip);

    // mDNS
    if (MDNS.begin("danse")) {
        Serial.println("mDNS actif: http://danse.local");
    } else {
        Serial.println("Erreur mDNS");
    }

    // ESP-NOW
    if (esp_now_init() != ESP_OK) {
        Serial.println("Erreur init ESP-NOW");
        return;
    }

    // Peer broadcast
    esp_now_peer_info_t peer = {};
    memcpy(peer.peer_addr, broadcastMAC, 6);
    peer.channel = 1;
    peer.encrypt = false;

    if (esp_now_add_peer(&peer) != ESP_OK) {
        Serial.println("Erreur ajout peer broadcast");
    } else {
        Serial.println("Peer broadcast ajouté");
    }

    // Serveur web
    server.on("/", []() {
        server.send(200, "text/html", webPage);
    });
    server.on("/send", HTTP_GET, handleSend);
    server.begin();
    Serial.println("Serveur web démarré");
}

// =====================
// loop : traitement web uniquement
// =====================
void loop() {
    server.handleClient();  // web toujours actif
    playModele();
}
