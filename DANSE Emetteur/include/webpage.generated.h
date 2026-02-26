#pragma once
// Auto-generated from web/webpage.html
const char* webPage = R"__WEBPAGE__(
<!DOCTYPE html>
<html lang="fr">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Contrôle LED APA102</title>
<style>
body { font-family: Arial, sans-serif; text-align: center; margin-top: 30px; background-color: #111; color: white; }
h1 { margin-bottom: 20px; }
.section { margin: 20px; }
button { font-size: 18px; padding: 12px; margin: 6px; width: 90%; max-width: 350px; border: none; border-radius: 8px; cursor: pointer; background-color:#2980b9; }
label { margin: 8px; display: inline-block; }
input[type="file"] { margin: 12px; }
.inline-couleur-fixe {
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 8px;
  flex-wrap: wrap;
  margin: 8px 0;
}
</style>
</head>
<body>

<h1>Contrôle LED APA102</h1>

<div class="section">
<h2>Choix des Groupes</h2>
<label><input type="checkbox" value="0" checked> Bleu</label>
<label><input type="checkbox" value="1" checked> Rouge</label>
<label><input type="checkbox" value="2" checked> Vert</label>
<label><input type="checkbox" value="3" checked> Noir</label>
<label><input type="checkbox" value="4" checked> Blanc</label>
<label><input type="checkbox" value="5" checked> Jaune</label>
</div>

<div class="section">
<h2>Effets lumineux</h2>
<button onclick="envoyer(0)">0 - Éteint</button>
<button onclick="envoyer(1)">Blanc fixe</button>
<button onclick="envoyer(2)">Vague lumineuse</button>
<button onclick="envoyer(3)">Pulsation</button>
<button onclick="envoyer(4)">Stroboscope</button>
<button onclick="envoyer(5)">Arc-en-ciel</button>
<button onclick="envoyer(6)">Explosion centrale</button>
<button onclick="envoyer(7)">Scanner gauche-droite</button>
<button onclick="envoyer(8)">Étincelles</button>
<div class="inline-couleur-fixe">
  <button onclick="envoyerCouleurFixe()">Couleur fixe choisie</button>
  <input type="color" id="couleurFixe" value="#0000ff" title="Choisir la couleur fixe">
</div>
<button onclick="envoyer(10)">Accélération </button>
<button onclick="modelePerso()">Modèle perso + musique</button>
</div>

<div class="section">
<h2>Lecture MP3 sur smartphone</h2>
<input type="file" id="mp3file" accept="audio/mp3">
<audio id="audioPlayer" controls></audio>
</div>

<script>
function getMasqueGroupes() {
  let checkboxes = document.querySelectorAll('input[type="checkbox"]');
  let masque = 0;
  checkboxes.forEach(cb => {
    if (cb.checked) {
      masque |= (1 << parseInt(cb.value));
    }
  });
  return masque;
}

function modelePerso() {
  let masque = getMasqueGroupes();

  // Dire aux récepteurs : mode perso
  fetch(`/send?g=${masque}&e=200`);

  // Lecture MP3
  let fileInput = document.getElementById('mp3file');
  let audioPlayer = document.getElementById('audioPlayer');

  if(fileInput.files.length > 0) {
    let file = fileInput.files[0];
    let url = URL.createObjectURL(file);
    audioPlayer.src = url;
    audioPlayer.play();
  }
}
function envoyer(effet) {
  let masque = getMasqueGroupes();

  // 2️⃣ Envoi au serveur ESP32 (ESP-NOW)
  fetch(`/send?g=${masque}&e=${effet}`);

  // 3️⃣ Jouer le MP3 seulement pour effet 10
  if(effet == 10) {
    let fileInput = document.getElementById('mp3file');
    let audioPlayer = document.getElementById('audioPlayer');
    if(fileInput.files.length > 0) {
      let file = fileInput.files[0];
      let url = URL.createObjectURL(file);
      audioPlayer.src = url;
      audioPlayer.play().catch(err => {
        console.log("Erreur lecture MP3 :", err);
      });
    } else {
      console.log("Aucun fichier MP3 sélectionné.");
    }
  }
}

function envoyerCouleurFixe() {
  let masque = getMasqueGroupes();
  let hex = document.getElementById('couleurFixe').value;
  let r = parseInt(hex.substring(1, 3), 16);
  let g = parseInt(hex.substring(3, 5), 16);
  let b = parseInt(hex.substring(5, 7), 16);
  fetch(`/send?g=${masque}&e=9&cr=${r}&cg=${g}&cb=${b}`);
}

// 4️⃣ Met à jour le player si on change de fichier
document.getElementById('mp3file').addEventListener('change', function() {
  let audioPlayer = document.getElementById('audioPlayer');
  if(this.files.length > 0) {
    let url = URL.createObjectURL(this.files[0]);
    audioPlayer.src = url;
  }
});
</script>

</body>
</html>

)__WEBPAGE__";
