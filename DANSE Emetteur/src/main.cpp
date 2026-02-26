#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include "webpage.generated.h"
#include "modelePersonalise.h" // contient Frame modele[] et NUM_FRAMES

#define LED_EMETTEUR 2
#define BTN_MODELE 0

WebServer server(80);

// Variables
bool modeleActif = false;        // mode externe actif ou non
int frameIndex = 0;
unsigned long nextFrameAtMs = 0;
uint8_t modeleMaskActif = 0xFF;
const uint8_t MODELE_BOUTON_MASK = 0x3F; // groupes 0..5
const uint8_t FLAG_COULEUR_FIXE = 0x80;  // trame couleur fixe (effet 9)
const int8_t WIFI_TX_POWER = 68; // 68 => ~17 dBm (valeur en 0.25 dBm)

// Bouton: INPUT_PULLUP (HIGH relache, LOW appuye)
bool btnStateStable = HIGH;
bool btnStateLastRead = HIGH;
unsigned long btnLastChangeMs = 0;
const unsigned long BTN_DEBOUNCE_MS = 30;

// Broadcast
uint8_t broadcastMAC[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};

void sendCommandeESPNow(uint8_t masque, uint8_t effet);
void sendCommandeESPNowRepete(uint8_t masque, uint8_t effet, uint8_t repeats = 3, uint16_t spacingMs = 25);

// =====================
// playModele : envoie les frames du modèle sans bloquer loop()
// =====================
void playModele() {
    if (!modeleActif) return;

    unsigned long now = millis();
    if ((long)(now - nextFrameAtMs) < 0) return;

    if (frameIndex >= NUM_FRAMES) {
        modeleActif = false;
        // 201 = fin modele externe
        sendCommandeESPNowRepete(modeleMaskActif, 201);
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
void sendCommandeESPNow(uint8_t masque, uint8_t effet) {
    uint8_t msg[2] = {masque, effet};
    esp_now_send(broadcastMAC, msg, 2);
}

void sendCommandeESPNowRepete(uint8_t masque, uint8_t effet, uint8_t repeats, uint16_t spacingMs) {
    if (repeats == 0) return;
    for (uint8_t i = 0; i < repeats; i++) {
        sendCommandeESPNow(masque, effet);
        if (i + 1 < repeats) delay(spacingMs);
    }
}

void sendESPNow(uint8_t masque, uint8_t effet) {

    Serial.print("Envoi masque=");
    Serial.print(masque);
    Serial.print(" effet=");
    Serial.println(effet);

    // Toujours envoyer le masque + effet (repete pour fiabilite)
    sendCommandeESPNowRepete(masque, effet);

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

void sendCouleurFixe(uint8_t masque, uint8_t r, uint8_t g, uint8_t b, uint8_t brightness) {
    uint8_t meta = (masque & 0x7F) | FLAG_COULEUR_FIXE;
const int8_t WIFI_TX_POWER = 68; // 68 => ~17 dBm (valeur en 0.25 dBm)
    uint8_t msg[5] = {meta, r, g, b, brightness};
    esp_now_send(broadcastMAC, msg, 5);

    Serial.print("Couleur fixe r=");
    Serial.print(r);
    Serial.print(" g=");
    Serial.print(g);
    Serial.print(" b=");
    Serial.print(b);
    Serial.print(" bri=");
    Serial.println(brightness);
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

    // Effet 9: couleur fixe optionnelle via cr/cg/cb (0..255)
    if (effet == 9 && server.hasArg("cr") && server.hasArg("cg") && server.hasArg("cb")) {
        uint8_t r = (uint8_t)constrain(server.arg("cr").toInt(), 0, 255);
        uint8_t g = (uint8_t)constrain(server.arg("cg").toInt(), 0, 255);
        uint8_t b = (uint8_t)constrain(server.arg("cb").toInt(), 0, 255);
        sendCouleurFixe(masque, r, g, b, 255);
    }

    server.send(200, "text/plain", "OK");
}

void handleBoutonModele() {
    bool reading = digitalRead(BTN_MODELE);
    unsigned long now = millis();

    if (reading != btnStateLastRead) {
        btnLastChangeMs = now;
        btnStateLastRead = reading;
    }

    if (now - btnLastChangeMs >= BTN_DEBOUNCE_MS && reading != btnStateStable) {
        btnStateStable = reading;
        if (btnStateStable == LOW) {
            Serial.println("Bouton modele: lancement");
            sendESPNow(MODELE_BOUTON_MASK, 200);
        }
    }
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
    pinMode(BTN_MODELE, INPUT_PULLUP);

    Serial.println("=== Émetteur LED APA102 ===");

    // Wi-Fi AP + STA
    WiFi.mode(WIFI_AP_STA);            
    WiFi.softAP("DANSE", NULL, 1);     
    WiFi.disconnect();                  
    esp_wifi_set_max_tx_power(WIFI_TX_POWER); // Puissance radio fixee pour stabilite

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
    handleBoutonModele();
    playModele();
}
