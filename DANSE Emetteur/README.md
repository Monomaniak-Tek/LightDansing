# DANSE Emetteur (ESP32)

## Role
Cet ESP32 heberge une interface web locale et envoie des commandes aux recepteurs via ESP-NOW.

## Pinmap actuelle (emetteur)
- `GPIO2` : LED temoin emetteur (`LED_EMETTEUR`)
- `GPIO0` : bouton local "modele perso"

## Fonctionnement radio
- Mode Wi-Fi : `WIFI_AP_STA`
- AP cree : `DANSE`
- Mot de passe AP : `fiona123`
- Canal radio : `1`
- ESP-NOW : envoi en broadcast (peer FF:FF:FF:FF:FF:FF)

## Protocole utilise
- Trame commande (2 octets) : `mask, effet`
- Trame couleur modele perso (5 octets) : `mask, r, g, b, brightness`
- Effet `200` : debut modele externe
- Effet `201` : fin modele externe
- Effets `240..245` : configuration groupe recepteur en lot (`groupe 0..5`)

## API web
- `GET /` : page web de controle
- `GET /send?g=<mask>&e=<effet>` : commande d'effet
- `GET /send?g=<mask>&e=9&cr=<r>&cg=<g>&cb=<b>` : couleur fixe
- `GET /setgroup?g=<0..5>` : config groupe en lot

## Interface web
- Choix des groupes cibles (0..5)
- Effets lumineux `0..10`
- Couleur fixe via selecteur couleur
- Modele perso + musique (option MP3 local)
- Configuration groupe en lot

## Prerequis
- VS Code + PlatformIO
- Carte ESP32 type `esp32dev`
- Recepteur(s) sur canal Wi-Fi 1 (ESP-NOW)

## Fichiers importants
- `src/main.cpp` : logique web + envoi ESP-NOW
- `src/modelePersonalise.cpp` : frames du modele personnalise
- `web/webpage.html` : page web source
- `include/webpage.generated.h` : header auto-genere a la compilation
- `NOTICE_UTILISATEUR_MODULES.md` : notice terrain utilisateur

## Build / Upload (PlatformIO)
```bash
platformio run
platformio run -t upload
platformio device monitor -b 115200
```
