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
.modele-btn.btn-active {
  background-color: #27ae60;
  box-shadow: 0 0 0 2px #eaffea inset, 0 0 12px rgba(39, 174, 96, 0.8);
}
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
.inline-modele-perso {
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 8px;
  flex-wrap: wrap;
  margin: 8px 0;
}
.help-box {
  max-width: 700px;
  margin: 12px auto;
  padding: 12px;
  border: 1px solid #413d3d;
  border-radius: 8px;
  background: #1b1b1b;
  text-align: left;
}
.help-box h3 {
  margin-top: 0;
  margin-bottom: 8px;
}
.help-box ol {
  margin: 0 0 8px 20px;
  padding: 0;
}
.help-box p {
  margin: 0;
}
.conn-status {
  max-width: 700px;
  margin: 8px auto 14px;
  padding: 10px;
  border: 1px solid #35506a;
  border-radius: 8px;
  background: #151a20;
}
.conn-ok { border-color: #2d7d46; }
.conn-warn { border-color: #9a6e1f; }
.conn-ko { border-color: #8b2d2d; }
.sd-status {
  max-width: 700px;
  margin: 8px auto;
  padding: 10px;
  border: 1px solid #3a4a60;
  border-radius: 8px;
  background: #151a20;
}
</style>
</head>
<body>

<h1>Contrôle LED APA102</h1>
<div id="connStatus" class="conn-status">Connexion: verification...</div>

<div class="section">
<h2>Choix des Groupes</h2>
<label><input type="checkbox" value="0" checked> Bleu</label>
<label><input type="checkbox" value="1" checked> Rouge</label>
<label><input type="checkbox" value="2" checked> Vert</label>
<label><input type="checkbox" value="3" checked> Violet</label>
<label><input type="checkbox" value="4" checked> Blanc</label>
<label><input type="checkbox" value="5" checked> Jaune</label>
</div>

<div class="section">
<h2>Effets locaux</h2>
<button id="btnEffet0" class="modele-btn" onclick="envoyer(0, 'btnEffet0')">0 - Éteint</button>
<button id="btnEffet1" class="modele-btn" onclick="envoyer(1, 'btnEffet1')">Blanc fixe</button>
<button id="btnEffet2" class="modele-btn" onclick="envoyer(2, 'btnEffet2')">Vague lumineuse</button>
<button id="btnEffet3" class="modele-btn" onclick="envoyer(3, 'btnEffet3')">Pulsation</button>
<button id="btnEffet4" class="modele-btn" onclick="envoyer(4, 'btnEffet4')">Stroboscope</button>
<button id="btnEffet5" class="modele-btn" onclick="envoyer(5, 'btnEffet5')">Arc-en-ciel</button>
<button id="btnEffet6" class="modele-btn" onclick="envoyer(6, 'btnEffet6')">Explosion centrale</button>
<button id="btnEffet7" class="modele-btn" onclick="envoyer(7, 'btnEffet7')">Scanner gauche-droite</button>
<button id="btnEffet8" class="modele-btn" onclick="envoyer(8, 'btnEffet8')">Étincelles</button>
<div class="inline-couleur-fixe">
  <button id="btnCouleurFixe" class="modele-btn" onclick="envoyerCouleurFixe('btnCouleurFixe')">Couleur fixe choisie</button>
  <input type="color" id="couleurFixe" value="#0000ff" title="Choisir la couleur fixe">
</div>
<button id="btnEffet10" class="modele-btn" onclick="envoyer(10, 'btnEffet10')">Accélération </button>
</div>

<div class="section">
<h2>Effet perso</h2>
<p>Choisis un modele de la carte SD, puis lance "Modèle perso + musique".</p>
<div id="sdInfo" class="sd-status">SD: ...</div>
<div class="inline-couleur-fixe">
  <select id="listeModeles"></select>
  <button onclick="rafraichirModeles()">Rafraichir la liste</button>
  <button onclick="chargerModeleSelection()">Charger modele selectionne</button>
  <button onclick="window.open('/logs','_blank')">Voir logs</button>
</div>
<div class="inline-modele-perso">
  <button id="btnModelePerso" class="modele-btn" onclick="modelePerso('btnModelePerso')">Modèle perso + musique</button>
  <input type="file" id="mp3file" accept="audio/mp3">
  <audio id="audioPlayer" controls></audio>
</div>
</div>

<div class="section">
<h2>Config recepteur</h2>
<p>Allumer uniquement les recepteurs a configurer, puis choisir le groupe cible :</p>
<div class="inline-couleur-fixe">
  <label for="groupeConfig">Groupe :</label>
  <select id="groupeConfig">
    <option value="0">Bleu (groupe 0)</option>
    <option value="1">Rouge (groupe 1)</option>
    <option value="2">Vert (groupe 2)</option>
    <option value="3">Violet (groupe 3)</option>
    <option value="4">Blanc (groupe 4)</option>
    <option value="5">Jaune (groupe 5)</option>
  </select>
</div>
<button onclick="configurerLotDepuisChoix()">Configurer les recepteurs allumes</button>

<h3>Configuration en lot du nombre de LEDs</h3>
<div class="inline-couleur-fixe">
  <label for="ledCountConfig">Nombre de LEDs :</label>
  <input id="ledCountConfig" type="number" min="1" max="250" value="30">
</div>
<button onclick="configurerLedCountLotDepuisChoix()">Configurer LED count en lot</button>

<h3>Configuration en lot du courant max</h3>
<div class="inline-couleur-fixe">
  <label for="maxCurrentConfig">Courant max (mA) :</label>
  <input id="maxCurrentConfig" type="number" min="100" max="10000" value="2500">
</div>
<button onclick="configurerMaxCurrentLotDepuisChoix()">Configurer courant max en lot</button>

<div class="help-box">
  <h3>Aide: configurer 1 recepteur individuellement (bouton BOOT)</h3>
  <ol>
    <li>Allumer le recepteur normalement.</li>
    <li>A n'importe quel moment, maintenir le bouton <strong>BOOT</strong> environ 2 secondes.</li>
    <li>Le bandeau defile les couleurs de groupe (1 couleur par seconde).</li>
    <li>Quand la bonne couleur apparait, appuyer une 2e fois pour valider.</li>
    <li>Le bandeau clignote: le groupe est sauvegarde (meme apres reboot).</li>
  </ol>
  <p><strong>Important:</strong> ne pas maintenir BOOT pendant la mise sous tension (GPIO9 = pin de boot ESP32-C3).</p>
</div>
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

function setModeleActifButton(buttonId) {
  document.querySelectorAll('.modele-btn').forEach(btn => btn.classList.remove('btn-active'));
  const current = document.getElementById(buttonId);
  if (current) current.classList.add('btn-active');
}

function setSdInfo(text, isError = false) {
  const el = document.getElementById('sdInfo');
  if (!el) return;
  el.textContent = text;
  el.style.borderColor = isError ? '#8b2d2d' : '#3a4a60';
}

let lastConnOkMs = 0;
function setConnStatus(text, state) {
  const el = document.getElementById('connStatus');
  if (!el) return;
  el.textContent = text;
  el.classList.remove('conn-ok', 'conn-warn', 'conn-ko');
  if (state === 'ok') el.classList.add('conn-ok');
  else if (state === 'warn') el.classList.add('conn-warn');
  else el.classList.add('conn-ko');
}

async function pingConnexion() {
  const t0 = performance.now();
  try {
    const r = await fetch('/runtimecfg?ts=' + Date.now(), { cache: 'no-store' });
    if (!r.ok) throw new Error('HTTP ' + r.status);
    const data = await r.json();
    const dt = Math.round(performance.now() - t0);
    const now = Date.now();
    const gap = lastConnOkMs === 0 ? 0 : now - lastConnOkMs;
    const nbRecepteurs = (typeof data.activeReceivers === 'number') ? data.activeReceivers : '?';
    lastConnOkMs = now;
    if (gap > 30000) setConnStatus(`Connexion: reprise (${dt} ms) | Recepteurs actifs: ${nbRecepteurs}`, 'warn');
    else setConnStatus(`Connexion active (${dt} ms) | Recepteurs actifs: ${nbRecepteurs}`, 'ok');
  } catch (_) {
    setConnStatus('Connexion: hors ligne / attente | Recepteurs actifs: ?', 'ko');
  }
}

function formatDurationMs(ms) {
  const sec = Math.floor(ms / 1000);
  const dec = Math.floor((ms % 1000) / 100);
  return `${sec}.${dec}s`;
}

async function refreshSdStatus() {
  try {
    const r = await fetch('/sdstatus');
    const t = await r.text();
    setSdInfo(t, !r.ok);
  } catch (e) {
    setSdInfo('Erreur SD status: ' + e, true);
  }
}

async function rafraichirModeles() {
  try {
    const r = await fetch('/models');
    const data = await r.json();
    const sel = document.getElementById('listeModeles');
    if (!sel) return;
    sel.innerHTML = '';

    if (!data.models || data.models.length === 0) {
      const opt = document.createElement('option');
      opt.value = '';
      opt.textContent = '(aucun modele dans /modeles)';
      sel.appendChild(opt);
    } else {
      data.models.forEach(name => {
        const opt = document.createElement('option');
        opt.value = name;
        opt.textContent = name;
        sel.appendChild(opt);
      });
    }

    setSdInfo(
      'SD=' + (data.sdReady ? 'OK' : 'KO') +
      ' current=' + data.current +
      ' frames=' + (data.currentFrames ?? '?') +
      ' duree=' + formatDurationMs(data.currentDurationMs ?? 0),
      !data.sdReady
    );
  } catch (e) {
    setSdInfo('Erreur liste modeles: ' + e, true);
  }
}

async function chargerModele(path) {
  try {
    const r = await fetch('/loadmodel?path=' + encodeURIComponent(path));
    const t = await r.text();
    setSdInfo((r.ok ? 'Chargement OK: ' : 'Chargement KO: ') + t, !r.ok);
    if (r.ok) {
      refreshSdStatus();
      rafraichirModeles();
    }
  } catch (e) {
    setSdInfo('Erreur chargement modele: ' + e, true);
  }
}

function chargerModeleSelection() {
  const sel = document.getElementById('listeModeles');
  if (!sel || !sel.value) return;
  chargerModele('/modeles/' + sel.value);
}

function modelePerso(buttonId) {
  let masque = getMasqueGroupes();

  // Dire aux récepteurs : mode perso
  fetch(`/send?g=${masque}&e=200`)
    .then(() => {
      if (buttonId) setModeleActifButton(buttonId);
    })
    .catch(err => console.log("Erreur envoi modele perso:", err));

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
function envoyer(effet, buttonId) {
  let masque = getMasqueGroupes();

  // 2️⃣ Envoi au serveur ESP32 (ESP-NOW)
  fetch(`/send?g=${masque}&e=${effet}`)
    .then(() => {
      if (buttonId) setModeleActifButton(buttonId);
    })
    .catch(err => console.log("Erreur envoi effet:", err));

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

function envoyerCouleurFixe(buttonId) {
  let masque = getMasqueGroupes();
  let hex = document.getElementById('couleurFixe').value;
  let r = parseInt(hex.substring(1, 3), 16);
  let g = parseInt(hex.substring(3, 5), 16);
  let b = parseInt(hex.substring(5, 7), 16);
  fetch(`/send?g=${masque}&e=9&cr=${r}&cg=${g}&cb=${b}`)
    .then(() => {
      if (buttonId) setModeleActifButton(buttonId);
    })
    .catch(err => console.log("Erreur envoi couleur fixe:", err));
}

function configurerLot(groupe) {
  const ok = confirm(
    `Confirmer la configuration en lot du groupe ${groupe} ?\n` +
    `Tous les recepteurs allumes seront modifies.`
  );
  if (!ok) return;
  fetch(`/setgroup?g=${groupe}`)
    .then(r => r.text())
    .then(msg => console.log(msg))
    .catch(err => console.log("Erreur config lot:", err));
}

function configurerLotDepuisChoix() {
  const groupe = parseInt(document.getElementById('groupeConfig').value, 10);
  configurerLot(groupe);
}

function configurerLedCountLot(n) {
  const ok = confirm(
    `Confirmer la configuration en lot du nombre de LEDs a ${n} ?\n` +
    `Tous les recepteurs allumes seront modifies.`
  );
  if (!ok) return;
  fetch(`/setledcount?n=${n}`)
    .then(r => r.text())
    .then(msg => console.log(msg))
    .catch(err => console.log("Erreur config lot LED count:", err));
}

function configurerLedCountLotDepuisChoix() {
  let n = parseInt(document.getElementById('ledCountConfig').value, 10);
  if (isNaN(n)) n = 30;
  if (n < 1) n = 1;
  if (n > 250) n = 250;
  document.getElementById('ledCountConfig').value = n;
  configurerLedCountLot(n);
}

function configurerMaxCurrentLot(ma) {
  const ok = confirm(
    `Confirmer la configuration en lot du courant max a ${ma} mA ?\n` +
    `Tous les recepteurs allumes seront modifies.`
  );
  if (!ok) return;
  fetch(`/setmaxcurrent?ma=${ma}`)
    .then(r => r.text())
    .then(msg => console.log(msg))
    .catch(err => console.log("Erreur config lot courant max:", err));
}

function configurerMaxCurrentLotDepuisChoix() {
  let ma = parseInt(document.getElementById('maxCurrentConfig').value, 10);
  if (isNaN(ma)) ma = 2500;
  if (ma < 100) ma = 100;
  if (ma > 10000) ma = 10000;
  document.getElementById('maxCurrentConfig').value = ma;
  configurerMaxCurrentLot(ma);
}

async function chargerConfigRuntime() {
  try {
    const r = await fetch('/runtimecfg');
    if (!r.ok) return;
    const data = await r.json();

    if (typeof data.ledCount === 'number') {
      let n = data.ledCount;
      if (n < 1) n = 1;
      if (n > 250) n = 250;
      document.getElementById('ledCountConfig').value = n;
    }

    if (typeof data.maxCurrentMa === 'number') {
      let ma = data.maxCurrentMa;
      if (ma < 100) ma = 100;
      if (ma > 10000) ma = 10000;
      document.getElementById('maxCurrentConfig').value = ma;
    }
  } catch (err) {
    console.log("Erreur lecture runtimecfg:", err);
  }
}

// 4️⃣ Met à jour le player si on change de fichier
document.getElementById('mp3file').addEventListener('change', function() {
  let audioPlayer = document.getElementById('audioPlayer');
  if(this.files.length > 0) {
    let url = URL.createObjectURL(this.files[0]);
    audioPlayer.src = url;
  }
});

chargerConfigRuntime();
refreshSdStatus();
rafraichirModeles();
pingConnexion();
setInterval(refreshSdStatus, 15000);
setInterval(pingConnexion, 5000);
</script>

</body>
</html>

)__WEBPAGE__";
