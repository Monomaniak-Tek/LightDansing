# Notice Utilisateur - Modules DANSE

Version: 03/03/2026  
Public: utilisateurs non techniques

## 1. Objet
Ce systeme pilote des bandeaux LED depuis un emetteur ESP32.

Principe:
- l'emetteur cree un reseau Wi-Fi local,
- un smartphone/tablette ouvre la page web de commande,
- les recepteurs appliquent les effets en temps reel.

## 2. Materiel necessaire
- 1 emetteur ESP32
- 1 a N recepteurs ESP32 + bandeau APA102/SK9822
- 1 smartphone/tablette
- alimentation(s) 5V adaptee(s)

## 3. Mise en route rapide
1. Alimenter l'emetteur.
2. Alimenter les recepteurs.
3. Se connecter au Wi-Fi `DANSE` (mot de passe `fiona123`).
4. Ouvrir `http://danse.local` (ou `http://192.168.4.1`).
5. Cocher les groupes et lancer un effet.

## 4. Utilisation de l'interface
1. Choisir les groupes cibles (Bleu, Rouge, Vert, Violet, Blanc, Jaune).
2. Lancer un effet:
3. `0 - Eteint`
4. `1 - Blanc fixe`
5. `2 - Vague lumineuse`
6. `3 - Pulsation`
7. `4 - Stroboscope`
8. `5 - Arc-en-ciel`
9. `6 - Explosion centrale`
10. `7 - Scanner gauche-droite`
11. `8 - Etincelles`
12. `9 - Couleur fixe`
13. `10 - Acceleration`
14. `Modele perso + musique`

## 5. Configuration groupe en lot
1. Allumer uniquement les recepteurs a configurer.
2. Dans la page web, choisir le groupe cible.
3. Cliquer "Configurer les recepteurs allumes".
4. Les recepteurs confirment visuellement la configuration.

## 6. Configuration individuelle d'un recepteur
1. Allumer le recepteur.
2. Dans les 5 secondes, maintenir `BOOT` ~0.8s pour entrer en provisioning.
3. Le bandeau defile les couleurs de groupe.
4. Appuyer pour valider le groupe courant.
5. Le groupe est sauvegarde en memoire (NVS).

## 7. Verifications avant evenement
1. Tous les recepteurs demarrent sans reset.
2. Chaque recepteur reagit au bon groupe.
3. L'interface web est accessible.
4. Test rapide des effets principaux.

## 8. Depannage rapide
### A. Le Wi-Fi DANSE n'apparait pas
1. Verifier l'alimentation de l'emetteur.
2. Redemarrer l'emetteur.

### B. Un recepteur ne repond pas
1. Verifier alim 5V bandeau + recepteur.
2. Verifier masse commune.
3. Verifier groupe du recepteur.
4. Redemarrer le recepteur.

### C. Erreurs de flash ESP32
1. Verifier cable USB data et port USB.
2. Utiliser mode BOOT manuel si besoin.
3. Eviter double alimentation USB + 5V externe non protegee.

## 9. Bonnes pratiques
1. Etiqueter chaque module avec son groupe.
2. Garder une version firmware unique sur tout le parc.
3. Prevoir des modules de secours.
4. Verifier le cablage avant chaque prestation.

---
Document interne exploitation.
