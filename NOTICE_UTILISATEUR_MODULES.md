# Notice Utilisateur - Modules DANSE

Version: V1.0
Public: utilisateurs non techniques

## Sommaire
1. Objet du systeme
2. Materiel necessaire
3. Mise en route rapide
4. Connexion au reseau de commande
5. Utilisation de l'interface
6. Choisir le groupe d'un recepteur
7. Verifications avant evenement
8. Depannage rapide
9. Bonnes pratiques (production 50 modules)
10. Informations utiles

## 1. Objet du systeme
Le systeme pilote des bandeaux LED (recepteurs) depuis un module emetteur ESP32.

Le principe:
- l'emetteur cree un reseau Wi-Fi local pour la commande,
- un smartphone/tablette se connecte a ce reseau,
- l'interface web permet de choisir les groupes et les effets,
- les recepteurs appliquent l'effet en temps reel.

## 2. Materiel necessaire
- 1 module **Emetteur ESP32** (point de commande)
- 1 a 50 modules **Recepteurs ESP32 + bandeau APA102**
- 1 smartphone ou tablette (Android/iOS)
- alimentations 5V adaptees a la puissance LED

## 3. Mise en route rapide
1. Alimenter l'emetteur.
2. Alimenter les recepteurs.
3. Sur smartphone, se connecter au Wi-Fi defini dans `config.json` (champ `ap_ssid`).
4. Mot de passe Wi-Fi: defini dans `config.json` (champ `ap_password`, vide = AP ouvert).
5. Ouvrir le navigateur:
6. URL 1 (recommandee): `http://danse.local`
7. URL 2 (secours): `http://192.168.4.1`
8. Choisir groupes + effet dans la page web.

## 4. Connexion au reseau de commande
Reseau emetteur (mode AP):
- SSID: defini dans `config.json` (`ap_ssid`)
- Mot de passe: defini dans `config.json` (`ap_password`, vide = AP ouvert)

Remarque:
- Internet n'est pas necessaire pendant l'utilisation.
- Le reseau sert uniquement a piloter les modules.
- Si `ssid` est renseigne dans `config.json`, l'emetteur tente d'abord une connexion au Wi-Fi local.
- Si la connexion echoue, l'AP est utilise automatiquement.

## 5. Utilisation de l'interface
Dans la page web:
1. Cocher les groupes cibles (Bleu, Rouge, Vert, Violet, Blanc, Jaune).
2. Appuyer sur un effet:
3. `0 - Eteint`
4. `1 - Blanc fixe`
5. `2 - Vague lumineuse`
6. `3 - Pulsation`
7. `4 - Stroboscope`
8. `5 - Arc-en-ciel`
9. `6 - Explosion centrale`
10. `7 - Scanner gauche-droite`
11. `8 - Etincelles`
12. `9 - Couleur fixe choisie`
13. `10 - Acceleration`
14. `Modele perso + musique`

Modele perso:
- vous pouvez charger un MP3 local dans la page,
- l'effet modele perso est declenche sur les groupes coches.

## 6. Choisir le groupe d'un recepteur
Chaque recepteur doit avoir un groupe (0 a 5).  
Le groupe est memorise meme apres extinction.

Correspondance groupe/couleur:
1. `0 = Bleu`
2. `1 = Rouge`
3. `2 = Vert`
4. `3 = Violet`
5. `4 = Blanc`
6. `5 = Jaune`

Procedure de provisioning (recepteur):
1. Allumer le recepteur normalement.
2. A n'importe quel moment, maintenir le bouton `BOOT` (`GPIO9`) environ 2 secondes.
3. Le recepteur entre en mode provisioning: le bandeau change de couleur toutes les 1 seconde.
4. Quand la couleur du groupe voulu apparait, appuyer une 2e fois.
5. Le bandeau clignote pour confirmer l'enregistrement.
6. Le groupe reste memorise apres extinction/reboot.

Important:
- Ne pas maintenir `BOOT` pendant la mise sous tension (risque de mode flash ESP32).

## 7. Verifications avant evenement
Check rapide:
1. Tous les recepteurs s'allument au boot.
2. Chaque recepteur reagit uniquement a son groupe.
3. L'URL `http://danse.local` s'ouvre.
4. Le mot de passe Wi-Fi est connu de l'operateur (voir `config.json`).
5. Un test de tous les effets est valide avant public.

## 8. Depannage rapide
### A. Je ne vois pas le Wi-Fi de l'emetteur
1. Verifier l'alimentation de l'emetteur.
2. Redemarrer l'emetteur.
3. Se rapprocher de l'emetteur.

### B. La page web ne s'ouvre pas
1. Verifier que le telephone est bien connecte a `DANSE`.
2. Essayer `http://192.168.4.1`.
3. Desactiver temporairement les donnees mobiles du telephone.

### C. Un recepteur ne repond pas
1. Verifier alimentation 5V du recepteur et du bandeau.
2. Verifier le groupe du recepteur (provisioning).
3. Verifier que le groupe est coche dans l'interface.
4. Redemarrer le recepteur.

### D. Mauvais groupe sur un recepteur
1. Refaire la procedure de provisioning groupe (section 6).
2. Valider sur la bonne couleur avant de cliquer.

## 9. Bonnes pratiques (production 50 modules)
1. Etiqueter chaque recepteur avec son numero et son groupe.
2. Faire un tableau de suivi: `Module`, `Groupe`, `Date test`, `Statut`.
3. Conserver 2 modules de secours preconfigures par groupe critique.
4. Utiliser des alimentations dimensionnees avec marge.
5. Verifier le serrage/connectique avant chaque prestation.
6. Garder la meme version firmware sur tout le parc.

## 10. Informations utiles
- Emetteur: ESP32 en mode AP + ESP-NOW
- Recepteur: ESP32 + APA102 + provisioning groupe persistant (NVS)
- Canal radio fixe: `1`
- Interface: navigateur web (mobile/PC)
- Site GitHub: `https://github.com/Monomaniak-Tek/LightDansing`

---
Document interne d'exploitation.  
Si besoin, une version "1 page terrain" peut etre generee a partir de cette notice.
