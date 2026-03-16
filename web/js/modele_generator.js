async function uploadModeleEnChunks(targetPath, modeleTxt, chunkSize = 1200) {
  let offset = 0;
  let first = true;
  while (offset < modeleTxt.length) {
    const part = modeleTxt.slice(offset, offset + chunkSize);
    const append = first ? 0 : 1;
    const resp = await fetch(`/savemodelchunk?path=${encodeURIComponent(targetPath)}&append=${append}`, {
      method: 'POST',
      headers: { 'Content-Type': 'text/plain' },
      body: part
    });
    const msg = await resp.text();
    if (!resp.ok) throw new Error('Chunk KO: ' + msg);
    offset += part.length;
    first = false;
  }
}

function sanitizeModelName(raw) {
  const safe = (raw || '').trim().replace(/[^a-zA-Z0-9_\-]/g, '_');
  if (safe.length === 0) return 'audio_auto';
  return safe.slice(0, 48);
}

function nextPow2(n) {
  let p = 1;
  while (p < n) p <<= 1;
  return p;
}

function fftInPlace(re, im) {
  const n = re.length;
  let j = 0;
  for (let i = 1; i < n; i++) {
    let bit = n >> 1;
    while (j & bit) {
      j ^= bit;
      bit >>= 1;
    }
    j ^= bit;
    if (i < j) {
      const tr = re[i]; re[i] = re[j]; re[j] = tr;
      const ti = im[i]; im[i] = im[j]; im[j] = ti;
    }
  }

  for (let len = 2; len <= n; len <<= 1) {
    const half = len >> 1;
    const ang = -2 * Math.PI / len;
    const wlenCos = Math.cos(ang);
    const wlenSin = Math.sin(ang);

    for (let i = 0; i < n; i += len) {
      let wCos = 1;
      let wSin = 0;
      for (let k = 0; k < half; k++) {
        const uRe = re[i + k];
        const uIm = im[i + k];
        const vRe = re[i + k + half] * wCos - im[i + k + half] * wSin;
        const vIm = re[i + k + half] * wSin + im[i + k + half] * wCos;

        re[i + k] = uRe + vRe;
        im[i + k] = uIm + vIm;
        re[i + k + half] = uRe - vRe;
        im[i + k + half] = uIm - vIm;

        const nextCos = wCos * wlenCos - wSin * wlenSin;
        const nextSin = wCos * wlenSin + wSin * wlenCos;
        wCos = nextCos;
        wSin = nextSin;
      }
    }
  }
}

function robustNormalize(values, pLow = 5, pHigh = 99) {
  if (!values.length) return [];
  const sorted = Array.from(values).sort((a, b) => a - b);
  const loIdx = Math.floor((pLow / 100) * (sorted.length - 1));
  const hiIdx = Math.floor((pHigh / 100) * (sorted.length - 1));
  let lo = sorted[loIdx];
  let hi = sorted[Math.max(hiIdx, loIdx)];
  if (!(hi > lo)) hi = lo + 1e-9;

  return values.map(v => {
    const z = (v - lo) / (hi - lo);
    if (z < 0) return 0;
    if (z > 1) return 1;
    return z;
  });
}

function smoothSeries(values, alpha = 0.25) {
  if (!values.length) return [];
  const out = new Array(values.length);
  out[0] = values[0];
  for (let i = 1; i < values.length; i++) {
    out[i] = alpha * values[i] + (1 - alpha) * out[i - 1];
  }
  return out;
}

function buildModeleFromPcm(samples, sampleRate, groupText, windowMs = 60) {
  const win = Math.max(1, Math.round(sampleRate * windowMs / 1000));
  const durationMs = Math.max(1, Math.round(win * 1000 / sampleRate));
  const low = [20, 200];
  const mid = [200, 2000];
  const high = [2000, 8000];

  const lowE = [];
  const midE = [];
  const highE = [];
  const rmsArr = [];

  for (let start = 0; start < samples.length; start += win) {
    const end = Math.min(start + win, samples.length);
    const segLen = end - start;
    if (segLen <= 0) break;

    let rmsAcc = 0;
    for (let i = start; i < end; i++) rmsAcc += samples[i] * samples[i];
    rmsArr.push(Math.sqrt(rmsAcc / segLen));

    const nFft = nextPow2(segLen);
    const re = new Float32Array(nFft);
    const im = new Float32Array(nFft);

    for (let i = 0; i < segLen; i++) {
      const w = 0.5 - 0.5 * Math.cos((2 * Math.PI * i) / Math.max(1, segLen - 1));
      re[i] = samples[start + i] * w;
    }

    fftInPlace(re, im);

    let eLow = 0;
    let eMid = 0;
    let eHigh = 0;
    const half = nFft >> 1;
    for (let k = 1; k < half; k++) {
      const f = (k * sampleRate) / nFft;
      const mag = Math.hypot(re[k], im[k]);
      if (f >= low[0] && f < low[1]) eLow += mag;
      else if (f >= mid[0] && f < mid[1]) eMid += mag;
      else if (f >= high[0] && f < high[1]) eHigh += mag;
    }

    lowE.push(eLow);
    midE.push(eMid);
    highE.push(eHigh);
  }

  const lowN = smoothSeries(robustNormalize(lowE), 0.25);
  const midN = smoothSeries(robustNormalize(midE), 0.25);
  const highN = smoothSeries(robustNormalize(highE), 0.25);
  const rmsN = smoothSeries(robustNormalize(rmsArr), 0.25);

  const lines = [];
  const frameCount = Math.min(lowN.length, midN.length, highN.length, rmsN.length);
  for (let i = 0; i < frameCount; i++) {
    const r = Math.max(0, Math.min(255, Math.round(highN[i] * 255)));
    const g = Math.max(0, Math.min(255, Math.round(midN[i] * 255)));
    const b = Math.max(0, Math.min(255, Math.round(lowN[i] * 255)));
    // Luminosite: garde le max a 255, mais plus de variation (gamma + seuil bas)
    const v = Math.max(0, Math.min(1, (rmsN[i] - 0.05) / 0.95));
    const briNorm = Math.pow(v, 1.7);
    const bri = Math.max(5, Math.min(255, Math.round(briNorm * 255)));
    lines.push(`${r},${g},${b},${bri},${durationMs},${groupText}`);
  }
  return lines;
}

async function genererModeleDepuisMp3() {
  const fileInput = document.getElementById('mp3fileModel') || document.getElementById('mp3file');
  if (!fileInput || fileInput.files.length === 0) {
    setGenerationInfo('Generation KO: choisis un fichier MP3', true);
    return;
  }

  const file = fileInput.files[0];
  const modelName = sanitizeModelName(document.getElementById('modeleAutoNom')?.value);
  const targetPath = `/modeles/${modelName}.txt`;
  const groupText = getGroupDigitsText();

  try {
    setGenerationInfo('Generation en cours: decode audio...');
    const arr = await file.arrayBuffer();
    const audioCtx = new (window.AudioContext || window.webkitAudioContext)();
    const audioBuffer = await audioCtx.decodeAudioData(arr.slice(0));
    await audioCtx.close();

    const ch0 = audioBuffer.getChannelData(0);
    const samples = new Float32Array(ch0.length);
    samples.set(ch0);
    if (audioBuffer.numberOfChannels > 1) {
      const ch1 = audioBuffer.getChannelData(1);
      const n = Math.min(ch0.length, ch1.length);
      for (let i = 0; i < n; i++) samples[i] = (ch0[i] + ch1[i]) * 0.5;
    }

    const maxFrames = 5000;
    let windowMs = 60;
    const minWindowMs = 60;
    const neededMs = Math.ceil((samples.length / maxFrames) / audioBuffer.sampleRate * 1000);
    if (neededMs > windowMs) windowMs = neededMs;
    if (windowMs < minWindowMs) windowMs = minWindowMs;

    setGenerationInfo('Generation en cours: analyse spectrale...');
    const lines = buildModeleFromPcm(samples, audioBuffer.sampleRate, groupText, windowMs);
    if (lines.length === 0) throw new Error('Aucune frame generee');

    if (lines.length > maxFrames) {
      throw new Error('Trop de frames (' + lines.length + '). Augmente la fenetre.');
    }

    const modeleTxt = lines.join('\n') + '\n';
    setGenerationInfo(`Generation: ${lines.length} frames, envoi vers ESP (chunks)...`);

    await uploadModeleEnChunks(targetPath, modeleTxt, 1200);

    const loadResp = await fetch(`/loadmodel?path=${encodeURIComponent(targetPath)}`);
    const loadMsg = await loadResp.text();
    if (!loadResp.ok) throw new Error('Chargement KO: ' + loadMsg);

    setGenerationInfo(`Generation OK: ${lines.length} frames -> ${targetPath}`);
    setSdInfo('Modele genere et charge: ' + targetPath, false);
    await rafraichirModeles();
    await refreshSdStatus();
  } catch (err) {
    setGenerationInfo('Generation KO: ' + err, true);
  }
}
