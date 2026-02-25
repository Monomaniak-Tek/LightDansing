#ifndef MODELE_H
#define MODELE_H

#include <Arduino.h>
#include <FastLED.h>

// ==============================
// CONFIGURATION DES LED
// ==============================
#define NUM_LEDS 30         // nombre de LEDs sur ton bandeau
#define DATA_PIN 22          // pin DATA du bandeau APA102
#define CLOCK_PIN 23        // pin CLOCK du bandeau APA102

CRGB leds[NUM_LEDS];

// ==============================
// INITIALISATION
// ==============================
void initLED() {
    FastLED.addLeds<APA102, DATA_PIN, CLOCK_PIN, BGR>(leds, NUM_LEDS);
    FastLED.setMaxPowerInVoltsAndMilliamps(5, 1000); // limite à 500 mA
    FastLED.setBrightness(255); // IMPORTANT
    FastLED.clear();
    FastLED.show();

}

// ==============================
// EFFET DE DEMARRAGE
// ==============================

void effetDemarrage() {

    for (int b = 0; b <= 255; b += 5) {
        fill_solid(leds, NUM_LEDS, CRGB(b, b, b));
        FastLED.show();
        delay(8);
    }

    for (int b = 255; b >= 0; b -= 5) {
        fill_solid(leds, NUM_LEDS, CRGB(b, b, b));
        FastLED.show();
        delay(8);
    }
}

// ==============================
// EFFETS
// ==============================

// 0 - LED OFF (Blackout)
void effetBlackout() {
    FastLED.clear();
    FastLED.show();
}

// 1 - LED ON (Blanc fixe)
void effetLEDOn() {
    fill_solid(leds, NUM_LEDS, CRGB::White);
    FastLED.show();
}

// 2 - Vague lumineuse
void effetVague() {
    static uint8_t phase = 0;

    for (int i = 0; i < NUM_LEDS; i++) {
        uint8_t brightness = sin8(i * 16 + phase);
        leds[i] = CHSV(160, 255, brightness); // bleu avec vague
    }

    phase += 4;
    FastLED.show();
    delay(30);
}

// 3 - Pulsation
void effetPulsation() {
    static int brightness = 0;
    static int delta = 5;

    fill_solid(leds, NUM_LEDS, CRGB::Red);
    FastLED.setBrightness(brightness);
    FastLED.show();

    brightness += delta;
    if (brightness <= 0 || brightness >= 255) delta = -delta;
    delay(30);

    FastLED.setBrightness(255); // remet normal
}


// 4 - Stroboscope
void effetStroboscope() {
    static bool on = false;
    fill_solid(leds, NUM_LEDS, on ? CRGB::White : CRGB::Black);
    FastLED.show();
    on = !on;
    delay(50);
}

// 5 - Arc-en-ciel fluide
void effetArcEnCiel() {
    static uint8_t startIndex = 0;
    fill_rainbow(leds, NUM_LEDS, startIndex++, 7);
    FastLED.show();
    delay(50);
}

// 6 - Explosion centrale
void effetExplosion() {
    static int center = NUM_LEDS / 2;
    static int radius = 0;
    FastLED.clear();
    for (int i = 0; i < NUM_LEDS; i++) {
        if (abs(i - center) <= radius) leds[i] = CRGB::Yellow;
    }
    radius++;
    if (radius > NUM_LEDS / 2) radius = 0;
    FastLED.show();
    delay(50);
}

// 7 - Scanner gauche-droite
void effetScanner() {
    static int pos = 0;
    static int dir = 1;
    FastLED.clear();
    leds[pos] = CRGB::Red;
    FastLED.show();
    pos += dir;
    if (pos <= 0 || pos >= NUM_LEDS - 1) dir = -dir;
    delay(50);
}

// 8 - Étincelles aléatoires
void effetEtincelles() {
    FastLED.clear();
    for (int i = 0; i < NUM_LEDS / 5; i++) {
        leds[random(NUM_LEDS)] = CHSV(random(0, 255), 255, 255);
    }
    FastLED.show();
    delay(50);
}

// 9 - Couleur fixe (paramétrable, par défaut bleu)
void effetCouleurFixe(CRGB couleur = CRGB::Blue) {
    fill_solid(leds, NUM_LEDS, couleur);
    FastLED.show();
}

// 10 - Accélération lumineuse (chenillard qui accélère)
void effetAcceleration() {
    static unsigned long previousMillis = 0;
    static int pos = 0;
    static int speed = 200;       // temps entre les LED en ms
    static int deltaSpeed = -5;   // variation de vitesse

    unsigned long currentMillis = millis();

    // Vérifie si le temps est écoulé pour passer à la LED suivante
    if (currentMillis - previousMillis >= speed) {
        previousMillis = currentMillis;

        // Efface les LED et allume la LED courante
        FastLED.clear();
        leds[pos] = CRGB::Green;
        FastLED.show();

        // Passe à la LED suivante
        pos++;
        if (pos >= NUM_LEDS) {
            speed = 200; 
            pos = 0;
        }
        // Ajuste la vitesse progressivement
        speed += deltaSpeed;
        if (speed <= 20) {
            speed = 20;
            deltaSpeed = 5;   // on ralentit maintenant
        } 
        else if (speed >= 200) {
            speed = 200;
            deltaSpeed = -5;  // on accélère maintenant
        }
    }
}



#endif // MODELE_H
