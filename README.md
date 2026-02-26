# LightDansing

Systeme de pilotage lumineux pour salle de spectacle, base sur 2 projets ESP32:
- **DANSE Emetteur**: interface web + envoi ESP-NOW
- **DANSE Recepteur**: reception ESP-NOW + pilotage LED APA102

## Architecture
- 1 emetteur ESP32 cree un AP Wi-Fi `DANSE` (canal 1) et diffuse les commandes en ESP-NOW.
- N recepteurs ESP32 (jusqu'a plusieurs dizaines) recoivent les commandes en broadcast.
- Les recepteurs appliquent soit un effet local, soit un modele personnalise envoye frame par frame.

## Arborescence
- `DANSE Emetteur/` : firmware emetteur
- `DANSE Recepteur/` : firmware recepteur

## Protocole radio
- **Commande effet** (2 octets): `mask, effet`
- **Frame couleur** (5 octets): `mask, r, g, b, brightness`
- `effet=200`: debut modele externe
- `effet=201`: fin modele externe

## Fonctionnement principal
- L'emetteur peut repeter les commandes (fiabilite radio) avec un espacement court.
- Le modele personnalise est joue en non-bloquant cote emetteur.
- A la fin du modele personnalise, les recepteurs passent en blackout (LED eteintes) jusqu'a nouvelle commande.

## Build et flash
Prerequis:
- VS Code + extension PlatformIO
- Cartes ESP32 compatibles

Exemples de commandes:
```bash
cd "DANSE Emetteur"
platformio run
platformio run -t upload

cd "../DANSE Recepteur"
platformio run
platformio run -t upload
```

## Documentation detaillee
- Voir le README de chaque sous-projet:
  - `DANSE Emetteur/README.md`
  - `DANSE Recepteur/README.md`

## Depot GitHub
- Remote: `https://github.com/Monomaniak-Tek/LightDansing`
