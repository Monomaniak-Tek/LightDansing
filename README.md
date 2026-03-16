# DANSE Emetteur (ESP32)

## Role
Cet ESP32 heberge une interface web et envoie des commandes/evenements LED aux recepteurs via ESP-NOW.

Version: V1.0

## Description globale
Le projet DANSE pilote des bandeaux LED APA102 via un emetteur ESP32 et des recepteurs ESP32.
L'emetteur cree un Wi-Fi local (AP ou Wi-Fi existant) et expose une interface web.
Depuis cette page, on selectionne des groupes, on lance des effets et on joue un modele personnalise.
Les commandes sont diffusees aux recepteurs via ESP-NOW, chaque recepteur n'appliquant que son groupe.
La carte SD sert a stocker les modeles LED et les fichiers audio.

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
- `web/webpage.html` : page web source (editable)
- `include/webpage.generated.h` : header auto-genere a la compilation

## Build / Upload (PlatformIO)
```bash
platformio run
platformio run -t upload
platformio device monitor -b 115200
```
