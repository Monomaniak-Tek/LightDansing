# DANSE Emetteur (ESP32)

## Role
Cet ESP32 heberge une interface web et envoie des commandes/evenements LED aux recepteurs via ESP-NOW.

## Pinmap actuelle (emetteur)
- `GPIO2` : LED temoin emetteur (`LED_EMETTEUR`)

## Fonctionnement radio
- Mode Wi-Fi : `WIFI_AP_STA`
- AP cree : `DANSE`
- Canal radio : `1`
- ESP-NOW : envoi en broadcast

## Protocole utilise
- Trame commande (2 octets) : `mask, effet`
- Trame couleur modele perso (5 octets) : `mask, r, g, b, brightness`
- Effet `200` : debut modele externe
- Effet `201` : fin modele externe

## Fichiers importants
- `src/main.cpp` : logique web + envoi ESP-NOW + lecture non bloquante du modele
- `src/modelePersonalise.cpp` : frames du modele personnalise
- `src/webpage.h` : page web de controle

## Build / Upload (PlatformIO)
```bash
platformio run
platformio run -t upload
platformio device monitor -b 115200
```
