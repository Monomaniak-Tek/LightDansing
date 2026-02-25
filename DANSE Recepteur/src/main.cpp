#include <FastLED.h>
#include <WiFi.h>
#include <esp_now.h>
#include "modele.h"  // effets locaux APA102

// ==============================
// Variables globales
// ==============================
uint8_t monGroupe = 0;           // 0=Bleu, 1=Rouge, etc.
volatile uint8_t effetActuel = 0xFF;  // 0xFF = aucun effet actif

// Modèle externe
bool modeleExterneActif = false;
volatile bool commandeEnAttente = false;
volatile uint8_t effetEnAttente = 0xFF;
volatile bool couleurEnAttente = false;
volatile uint8_t rEnAttente = 0, gEnAttente = 0, bEnAttente = 0, luminositeEnAttente = 0;
unsigned long lastModeleFrameMs = 0;
// Doit etre superieur a la plus longue duree entre 2 frames cote emetteur.
const unsigned long MODELE_TIMEOUT_MS = 25000;
const uint8_t LOCAL_BRIGHTNESS = 255;

// ==============================
// Callback ESP-NOW
// ==============================
void onDataRecv(const uint8_t *mac_addr, const uint8_t *data, int len) {
    (void)mac_addr;

    // COMMANDE (2 octets): masque + effet
    if (len == 2) {
        uint8_t groupeMask = data[0];
        if (groupeMask & (1 << monGroupe)) {
            effetEnAttente = data[1];
            commandeEnAttente = true;
        }
    }

    // COULEUR (5 octets): masque + r + g + b + luminosite
    if (len == 5) {
        uint8_t groupeMask = data[0];
        if (groupeMask & (1 << monGroupe)) {
            rEnAttente = data[1];
            gEnAttente = data[2];
            bEnAttente = data[3];
            luminositeEnAttente = data[4];
            couleurEnAttente = true;
        }
    }
}




// ==============================
// Setup
// ==============================
void setup() {
    Serial.begin(115200);
    delay(150);

    Serial.println("=== Récepteur LED APA102 ===");

    initLED();
    effetDemarrage(); // effet de boot

    // ESP-NOW
   WiFi.mode(WIFI_STA);
   WiFi.disconnect();
   WiFi.channel(1); // fixe le canal Wi-Fi sur 1
    if(esp_now_init() != ESP_OK){
        Serial.println("Erreur init ESP-NOW");
        return;
    }

    esp_now_register_recv_cb(onDataRecv);

    Serial.printf("Module configuré pour le groupe : %d\n", monGroupe);
    Serial.println("Récepteur prêt et attente de commandes ESP-NOW");
}

// ==============================
// Loop
// ==============================
void loop() {
    if (commandeEnAttente) {
        uint8_t effet = effetEnAttente;
        commandeEnAttente = false;

        if (effet == 200) {
            modeleExterneActif = true;
            lastModeleFrameMs = millis();
            Serial.println("Modele externe ON");
        } else if (effet == 201) {
            modeleExterneActif = false;
            effetActuel = 0; // blackout apres fin du modele externe
            FastLED.setBrightness(LOCAL_BRIGHTNESS);
            Serial.println("Modele externe OFF (fin modele)");
        } else {
            effetActuel = effet;
            modeleExterneActif = false;
            FastLED.setBrightness(LOCAL_BRIGHTNESS);
            Serial.println("Modele externe OFF");
        }
    }

    if (couleurEnAttente) {
        uint8_t r = rEnAttente;
        uint8_t g = gEnAttente;
        uint8_t b = bEnAttente;
        uint8_t luminosite = luminositeEnAttente;
        couleurEnAttente = false;

        // Une trame couleur valide indique que le modele externe est en cours.
        modeleExterneActif = true;
        FastLED.setBrightness(luminosite);
        fill_solid(leds, NUM_LEDS, CRGB(r, g, b));
        FastLED.show();
        lastModeleFrameMs = millis();
    }

    if (modeleExterneActif && (millis() - lastModeleFrameMs > MODELE_TIMEOUT_MS)) {
        modeleExterneActif = false;
        effetActuel = 0; // blackout de securite en cas de perte du modele externe
        FastLED.setBrightness(LOCAL_BRIGHTNESS);
        Serial.println("Timeout modele externe -> retour effets locaux");
    }

    // Si modèle externe actif, les couleurs sont appliquées plus haut dans loop()
    if(modeleExterneActif) {
        // rien à faire ici
    } else {
        // effets locaux
        switch(effetActuel) {
            case 0:  effetBlackout();      break;
            case 1:  effetLEDOn();         break;
            case 2:  effetVague();         break;
            case 3:  effetPulsation();     break;
            case 4:  effetStroboscope();   break;
            case 5:  effetArcEnCiel();     break;
            case 6:  effetExplosion();     break;
            case 7:  effetScanner();       break;
            case 8:  effetEtincelles();    break;
            case 9:  effetCouleurFixe();   break;
            case 10: effetAcceleration();  break;
            default: break;
        }
    }

    delay(10); // petite pause pour ne pas bloquer l'ESP
}
