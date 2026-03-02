#include <FastLED.h>
#include <WiFi.h>
#include <esp_now.h>
#include <Preferences.h>
#include "modele.h"  // effets locaux APA102

// ==============================
// Variables globales
// ==============================
uint8_t monGroupe = 0;           // 0=Bleu, 1=Rouge, etc.
volatile uint8_t effetActuel = 0xFF;  // 0xFF = aucun effet actif
const uint8_t EFFET_MODELE_START = 200;
const uint8_t EFFET_MODELE_END = 201;
const uint8_t FLAG_COULEUR_FIXE = 0x80;
const uint8_t GROUPE_MAX = 5;
const uint8_t GROUPE_DEFAUT = 0;
const uint8_t BTN_PROVISION_PIN = 0;         // bouton BOOT integre (GPIO0)
const unsigned long PROVISION_HOLD_MS = 800; // appui long requis
const unsigned long PROVISION_BOOT_WINDOW_MS = 5000; // fenetre d'entree apres boot
const unsigned long PROVISION_STEP_MS = 1000; // defilement couleurs
const unsigned long BTN_DEBOUNCE_MS = 25;
const char* NVS_NAMESPACE = "danse";
const char* NVS_KEY_GROUPE = "groupe";
Preferences prefs;

// Modèle externe
bool modeleExterneActif = false;
volatile bool commandeEnAttente = false;
volatile uint8_t effetEnAttente = 0xFF;
volatile bool couleurEnAttente = false;
volatile uint8_t rEnAttente = 0, gEnAttente = 0, bEnAttente = 0, luminositeEnAttente = 0;
volatile bool couleurFixePacketEnAttente = false;
unsigned long lastModeleFrameMs = 0;
// Doit etre superieur a la plus longue duree entre 2 frames cote emetteur.
const unsigned long MODELE_TIMEOUT_MS = 25000;
const uint8_t LOCAL_BRIGHTNESS = 255;
CRGB couleurFixeActuelle = CRGB::Blue;
uint8_t luminositeFixeActuelle = 255;
portMUX_TYPE espNowMux = portMUX_INITIALIZER_UNLOCKED;

CRGB couleurPourGroupe(uint8_t groupe) {
    switch (groupe) {
        case 0: return CRGB::Blue;
        case 1: return CRGB::Red;
        case 2: return CRGB::Green;
        case 3: return CRGB::Purple;
        case 4: return CRGB::White;
        case 5: return CRGB::Yellow;
        default: return CRGB::White;
    }
}

uint8_t chargeGroupeDepuisNVS() {
    if (!prefs.begin(NVS_NAMESPACE, false)) {
        Serial.println("NVS indisponible, groupe par defaut");
        return GROUPE_DEFAUT;
    }
    uint8_t groupe = prefs.getUChar(NVS_KEY_GROUPE, GROUPE_DEFAUT);
    prefs.end();
    if (groupe > GROUPE_MAX) {
        groupe = GROUPE_DEFAUT;
    }
    return groupe;
}

void sauvegardeGroupeNVS(uint8_t groupe) {
    if (groupe > GROUPE_MAX) {
        groupe = GROUPE_DEFAUT;
    }
    if (!prefs.begin(NVS_NAMESPACE, false)) {
        Serial.println("Erreur ouverture NVS pour sauvegarde groupe");
        return;
    }
    prefs.putUChar(NVS_KEY_GROUPE, groupe);
    prefs.end();
}

bool provisioningDemandeAuBoot() {
    // Sur ESP32 DevKit, GPIO0 est une pin de boot:
    // ne pas exiger "maintenu a l'allumage" pour eviter le mode flash.
    // On offre une fenetre juste apres boot pour appui long.
    unsigned long windowStart = millis();
    while (millis() - windowStart < PROVISION_BOOT_WINDOW_MS) {
        if (digitalRead(BTN_PROVISION_PIN) == LOW) {
            unsigned long pressStart = millis();
            while (digitalRead(BTN_PROVISION_PIN) == LOW) {
                if (millis() - pressStart >= PROVISION_HOLD_MS) {
                    while (digitalRead(BTN_PROVISION_PIN) == LOW) {
                        delay(5);
                    }
                    return true;
                }
                delay(5);
            }
        }
        delay(5);
    }
    return false;
}

void clignoteValidation(CRGB couleur, uint8_t repeats = 3) {
    for (uint8_t i = 0; i < repeats; i++) {
        fill_solid(leds, NUM_LEDS, couleur);
        FastLED.show();
        delay(140);
        FastLED.clear();
        FastLED.show();
        delay(140);
    }
}

void modeProvisioningGroupe() {
    Serial.println("=== Mode provisioning groupe ===");
    Serial.println("Defilement 1s/couleur. Clique pour valider.");

    uint8_t groupeSelection = monGroupe;
    unsigned long lastStepMs = millis();

    fill_solid(leds, NUM_LEDS, couleurPourGroupe(groupeSelection));
    FastLED.show();

    bool lastRead = HIGH;
    bool stableState = HIGH;
    unsigned long lastChangeMs = 0;

    while (true) {
        unsigned long now = millis();

        if (now - lastStepMs >= PROVISION_STEP_MS) {
            lastStepMs = now;
            groupeSelection = (groupeSelection + 1) % (GROUPE_MAX + 1);
            fill_solid(leds, NUM_LEDS, couleurPourGroupe(groupeSelection));
            FastLED.show();
            Serial.printf("Groupe candidat: %u\n", groupeSelection);
        }

        bool reading = digitalRead(BTN_PROVISION_PIN);
        if (reading != lastRead) {
            lastRead = reading;
            lastChangeMs = now;
        }

        if ((now - lastChangeMs) >= BTN_DEBOUNCE_MS && reading != stableState) {
            stableState = reading;
            if (stableState == LOW) {
                while (digitalRead(BTN_PROVISION_PIN) == LOW) {
                    delay(5);
                }
                monGroupe = groupeSelection;
                sauvegardeGroupeNVS(monGroupe);
                clignoteValidation(couleurPourGroupe(monGroupe));
                Serial.printf("Groupe %u valide et sauvegarde\n", monGroupe);
                return;
            }
        }

        delay(5);
    }
}

// ==============================
// Callback ESP-NOW
// ==============================
void onDataRecv(const uint8_t *mac_addr, const uint8_t *data, int len) {
    (void)mac_addr;

    // COMMANDE (2 octets): masque + effet
    if (len == 2) {
        uint8_t groupeMask = data[0];
        if (groupeMask & (1 << monGroupe)) {
            portENTER_CRITICAL_ISR(&espNowMux);
            effetEnAttente = data[1];
            commandeEnAttente = true;
            portEXIT_CRITICAL_ISR(&espNowMux);
        }
    }

    // COULEUR (5 octets): meta + r + g + b + luminosite
    // meta bit7=1 -> couleur fixe (effet 9), bit7=0 -> frame modele externe
    if (len == 5) {
        uint8_t meta = data[0];
        uint8_t groupeMask = meta & 0x7F;
        bool isCouleurFixe = (meta & FLAG_COULEUR_FIXE) != 0;
        if (groupeMask & (1 << monGroupe)) {
            portENTER_CRITICAL_ISR(&espNowMux);
            rEnAttente = data[1];
            gEnAttente = data[2];
            bEnAttente = data[3];
            luminositeEnAttente = data[4];
            couleurFixePacketEnAttente = isCouleurFixe;
            couleurEnAttente = true;
            portEXIT_CRITICAL_ISR(&espNowMux);
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

    pinMode(BTN_PROVISION_PIN, INPUT_PULLUP);
    monGroupe = chargeGroupeDepuisNVS();
    Serial.printf("Groupe charge depuis NVS: %u\n", monGroupe);

    initLED();

    if (provisioningDemandeAuBoot()) {
        modeProvisioningGroupe();
    }

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
    bool hasCommande = false;
    uint8_t effet = 0;
    portENTER_CRITICAL(&espNowMux);
    if (commandeEnAttente) {
        effet = effetEnAttente;
        commandeEnAttente = false;
        hasCommande = true;
    }
    portEXIT_CRITICAL(&espNowMux);

    if (hasCommande) {

        if (effet == EFFET_MODELE_START) {
            modeleExterneActif = true;
            lastModeleFrameMs = millis();
            Serial.println("Modele externe ON");
        } else if (effet == EFFET_MODELE_END) {
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

    bool hasCouleur = false;
    uint8_t r = 0, g = 0, b = 0, luminosite = 0;
    bool isCouleurFixe = false;
    portENTER_CRITICAL(&espNowMux);
    if (couleurEnAttente) {
        r = rEnAttente;
        g = gEnAttente;
        b = bEnAttente;
        luminosite = luminositeEnAttente;
        isCouleurFixe = couleurFixePacketEnAttente;
        couleurEnAttente = false;
        hasCouleur = true;
    }
    portEXIT_CRITICAL(&espNowMux);

    if (hasCouleur) {

        if (isCouleurFixe) {
            // Couleur de l'effet local 9
            couleurFixeActuelle = CRGB(r, g, b);
            luminositeFixeActuelle = luminosite;
            modeleExterneActif = false;
            if (effetActuel == 9) {
                FastLED.setBrightness(luminositeFixeActuelle);
                effetCouleurFixe(couleurFixeActuelle);
            }
        } else {
            // Une trame couleur modele valide indique que le modele externe est en cours.
            modeleExterneActif = true;
            FastLED.setBrightness(luminosite);
            fill_solid(leds, NUM_LEDS, CRGB(r, g, b));
            FastLED.show();
            lastModeleFrameMs = millis();
        }
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
            case 9:
                FastLED.setBrightness(luminositeFixeActuelle);
                effetCouleurFixe(couleurFixeActuelle);
                break;
            case 10: effetAcceleration();  break;
            default: break;
        }
    }

    delay(10); // petite pause pour ne pas bloquer l'ESP
}
