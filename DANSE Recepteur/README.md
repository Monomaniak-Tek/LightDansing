# DANSE Recepteur (ESP32 + APA102)

## Role
Cet ESP32 recoit les commandes de l'emetteur en ESP-NOW et affiche soit:
- des effets locaux,
- soit un modele personnalise envoye en trames couleur.

## Pinmap actuelle (recepteur)
- `GPIO22` : DATA APA102 (`DATA_PIN`)
- `GPIO23` : CLOCK APA102 (`CLOCK_PIN`)
- `NUM_LEDS` : `30`

## Parametres LED actuels
- Type : `APA102`
- Ordre couleur : `BGR`
- Limite puissance FastLED : `5V / 1000mA`
- Luminosite locale par defaut : `255`

## Fonctionnement radio
- Wi-Fi : `WIFI_STA`
- Canal force : `1`
- ESP-NOW reception active

## Protocole recu
- Trame commande (2 octets) : `mask, effet`
- Trame couleur modele perso (5 octets) : `mask, r, g, b, brightness`
- Effet `200` : active le modele externe
- Effet `201` : fin de modele externe

## Comportement actuel fin de modele perso
- A la fin du modele (`201`), le recepteur passe en `blackout` (`effet 0`).
- Les LED restent eteintes jusqu'a une nouvelle commande d'effet.
- Timeout securite modele externe : `25000 ms`.

## Fichiers importants
- `src/main.cpp` : reception ESP-NOW + gestion etats local/externe
- `src/modele.h` : config LEDs et effets locaux

## Build / Upload (PlatformIO)
```bash
platformio run
platformio run -t upload
platformio device monitor -b 115200
```
