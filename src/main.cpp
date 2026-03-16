#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <SD.h>
#include <SPI.h>
#include "webpages.generated.h"

static const uint8_t FRAME_GROUP_MASK_ALL = 0x3F; // bits groupes 0..5

// Ecriture "humaine" des groupes:
// GROUPS(0134) -> groupes 0,1,3,4
// GROUPS(25)   -> groupes 2,5
// Les caracteres hors 0..5 sont ignores.
constexpr uint8_t groupMaskFromDigits(const char* s, uint8_t acc = 0) {
  return (*s == '\0')
             ? acc
             : groupMaskFromDigits(
                   s + 1,
                   (uint8_t)((*s >= '0' && *s <= '5') ? (acc | (uint8_t)(1U << (*s - '0'))) : acc));
}
#define GROUPS(x) groupMaskFromDigits(#x)

struct Frame {
  uint8_t r;
  uint8_t g;
  uint8_t b;
  uint8_t brightness;
  uint16_t durationMs;
  uint8_t groupMask;

  constexpr Frame()
      : r(0), g(0), b(0), brightness(0), durationMs(1), groupMask(FRAME_GROUP_MASK_ALL) {}

  // groupMask est optionnel: par defaut, la frame cible tous les groupes (0..5).
  constexpr Frame(uint8_t r_, uint8_t g_, uint8_t b_, uint8_t brightness_, uint16_t durationMs_,
                  uint8_t groupMask_ = FRAME_GROUP_MASK_ALL)
      : r(r_),
        g(g_),
        b(b_),
        brightness(brightness_),
        durationMs(durationMs_),
        groupMask(groupMask_) {}
};

// Format d'une frame:
// { r, g, b, brightness, durationMs, [groupMask optionnel] }
// r          : intensite rouge (0-255)
// g          : intensite vert (0-255)
// b          : intensite bleu (0-255)
// brightness : luminosite globale FastLED (0-255)
// durationMs : duree d'affichage de la frame en millisecondes
// groupMask  : bits groupes 0..5 (optionnel, defaut=0x3F tous groupes)
// Modèle interne si SD absente ou modèle SD non chargé
const Frame modele[] = {
    {255, 0, 0, 30, 1500},
    {255, 80, 0, 80, 1050},
    {255, 160, 0, 140, 150},
    {255, 220, 0, 200, 1500},
    {255, 220, 0, 120, 2050},
    {0, 0, 0, 0, 20},
};

const size_t NUM_FRAMES = sizeof(modele) / sizeof(modele[0]);

#define LED_EMETTEUR 2
#define BTN_MODELE 0

WebServer server(80);

// Variables
bool modeleActif = false; // mode externe actif ou non
int frameIndex = 0;
unsigned long nextFrameAtMs = 0;
uint8_t modeleMaskActif = 0xFF;
const uint8_t MODELE_BOUTON_MASK = 0x3F; // groupes 0..5
const uint8_t FLAG_COULEUR_FIXE = 0x80;  // trame couleur fixe (effet 9)
const uint8_t EFFET_MODELE_START = 200;
const uint8_t EFFET_MODELE_END = 201;
const uint8_t EFFET_TEST_GROUPE = 202;
const uint8_t EFFET_CFG_GROUPE_BASE = 240; // 240..245 => set groupe 0..5
const uint8_t CMD_CFG_LED_COUNT = 1;        // commande 3 octets: masque, cmd, value
const uint8_t CMD_CFG_MAX_CURRENT_MA = 2;    // commande 4 octets: masque, cmd, valueLo, valueHi
const uint8_t CMD_CFG_BRIGHTNESS = 3;        // commande 3 octets: masque, cmd, value
const uint8_t GROUPE_MIN = 0;
const uint8_t GROUPE_MAX = 5;
const uint8_t MASQUE_TOUT_GROUPES = 0x7F;
const uint8_t LED_COUNT_MIN = 1;
const uint8_t LED_COUNT_MAX = 250;
const uint16_t MAX_CURRENT_MA_MIN = 100;
const uint16_t MAX_CURRENT_MA_MAX = 10000;
const uint8_t LED_COUNT_DEFAULT = 30;
const uint16_t MAX_CURRENT_MA_DEFAULT = 2500;
const uint8_t HEARTBEAT_MAGIC = 0xA1;
const unsigned long RECEIVER_ACTIVE_TIMEOUT_MS = 3000;
const size_t MAX_TRACKED_RECEIVERS = 100;
const int AP_CHANNEL = 1;
const int8_t WIFI_TX_POWER_DEFAULT = 68; // 68 => ~17 dBm (valeur en 0.25 dBm)
const char* AP_SSID_DEFAULT = "Passion_DANCE";
const char* AP_PASSWORD_DEFAULT = ""; // AP ouvert par defaut
const char* WIFI_STA_SSID_DEFAULT = "";
const char* WIFI_STA_PASS_DEFAULT = "";

String runtimeApSsid = AP_SSID_DEFAULT;
String runtimeApPassword = AP_PASSWORD_DEFAULT;
String runtimeStaSsid = WIFI_STA_SSID_DEFAULT;
String runtimeStaPass = WIFI_STA_PASS_DEFAULT;
int8_t runtimeWiFiTxPower = WIFI_TX_POWER_DEFAULT;
uint8_t runtimeLedCountConfig = LED_COUNT_DEFAULT;
uint16_t runtimeMaxCurrentMaConfig = MAX_CURRENT_MA_DEFAULT;

// SD (VSPI)
const int SD_CS = 18;
const int SD_SCK = 19;
const int SD_MISO = 33;
const int SD_MOSI = 32;
const char* SD_CONFIG_JSON_FILE = "/config.json";
const char* SD_PRIMARY_MODEL = "/modele_personnalise.txt";
const char* SD_MODELS_DIR = "/modeles";
const char* SD_MUSIC_DIR = "/musique";
const char* SD_LOG_FILE = "/logs.txt";
const size_t MAX_SD_FRAMES = 5000;

bool sdReady = false;
Frame modeleSD[MAX_SD_FRAMES] = {};
size_t modeleSDCount = 0;
bool modeleSDActive = false;
String modeleSourcePath = "interne";

// Bouton: INPUT_PULLUP (HIGH relache, LOW appuye)
bool btnStateStable = HIGH;
bool btnStateLastRead = HIGH;
unsigned long btnLastChangeMs = 0;
const unsigned long BTN_DEBOUNCE_MS = 30;

// Broadcast
uint8_t broadcastMAC[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

struct ReceiverSeen {
  bool used;
  uint8_t mac[6];
  unsigned long lastSeenMs;
};

ReceiverSeen trackedReceivers[MAX_TRACKED_RECEIVERS] = {};
portMUX_TYPE receiverMux = portMUX_INITIALIZER_UNLOCKED;

bool sameMac(const uint8_t* a, const uint8_t* b) {
  if (!a || !b) return false;
  for (size_t i = 0; i < 6; i++) {
    if (a[i] != b[i]) return false;
  }
  return true;
}

void markReceiverHeartbeat(const uint8_t* mac) {
  if (!mac) return;

  unsigned long now = millis();
  portENTER_CRITICAL(&receiverMux);

  for (size_t i = 0; i < MAX_TRACKED_RECEIVERS; i++) {
    if (trackedReceivers[i].used && sameMac(trackedReceivers[i].mac, mac)) {
      trackedReceivers[i].lastSeenMs = now;
      portEXIT_CRITICAL(&receiverMux);
      return;
    }
  }

  for (size_t i = 0; i < MAX_TRACKED_RECEIVERS; i++) {
    if (!trackedReceivers[i].used) {
      trackedReceivers[i].used = true;
      memcpy(trackedReceivers[i].mac, mac, 6);
      trackedReceivers[i].lastSeenMs = now;
      break;
    }
  }

  portEXIT_CRITICAL(&receiverMux);
}

size_t countActiveReceivers() {
  unsigned long now = millis();
  size_t active = 0;

  portENTER_CRITICAL(&receiverMux);
  for (size_t i = 0; i < MAX_TRACKED_RECEIVERS; i++) {
    if (!trackedReceivers[i].used) continue;
    if ((now - trackedReceivers[i].lastSeenMs) <= RECEIVER_ACTIVE_TIMEOUT_MS) {
      active++;
    }
  }
  portEXIT_CRITICAL(&receiverMux);

  return active;
}

void onDataRecv(const uint8_t* mac_addr, const uint8_t* data, int len) {
  if (!mac_addr || !data) return;
  if (len != 1) return;
  if (data[0] != HEARTBEAT_MAGIC) return;

  markReceiverHeartbeat(mac_addr);
}

void sendCommandeESPNow(uint8_t masque, uint8_t effet);
void sendCommandeESPNowRepete(uint8_t masque, uint8_t effet, uint8_t repeats = 3, uint16_t spacingMs = 25);
void sendConfigGroupeLot(uint8_t groupe);
void sendConfigLedCountLot(uint8_t ledCount);
void sendConfigMaxCurrentLot(uint16_t maxCurrentMa);

String readFileRawFromSd(const char* path, String* errorMsg = nullptr);
String jsonGetString(const String& json, const char* key, const String& defaultValue);
int jsonGetInt(const String& json, const char* key, int defaultValue);
bool loadRuntimeConfigFromSdJson(String* errorMsg = nullptr);
bool loadModeleFromSd(const String& inputPath, String* errorMsg);
bool writeTextFileChunkToSd(const String& rawPath, const String& content, bool append, String* errorMsg = nullptr);
const Frame& getFrameByIndex(size_t idx);
size_t getFrameCount();
uint32_t getCurrentModeleDurationMs();
String formatDurationSec(uint32_t durationMs);

void logEvent(const String& msg) {
  Serial.println(msg);
  if (!sdReady) {
    return;
  }
  File f = SD.open(SD_LOG_FILE, FILE_APPEND);
  if (!f) {
    return;
  }
  f.print(millis());
  f.print(" ");
  f.println(msg);
  f.close();
}

bool isSafeSdPath(const String& path) {
  if (!path.startsWith("/")) return false;
  if (path.indexOf("..") >= 0) return false;
  if (!path.endsWith(".txt")) return false;
  return true;
}

bool hasAllowedAudioExt(const String& path) {
  String p = path;
  p.toLowerCase();
  return p.endsWith(".mp3") || p.endsWith(".wav") || p.endsWith(".ogg");
}

bool isSafeMusicPath(const String& path) {
  if (!path.startsWith("/")) return false;
  if (path.indexOf("..") >= 0) return false;
  if (!path.startsWith(String(SD_MUSIC_DIR) + "/")) return false;
  if (!hasAllowedAudioExt(path)) return false;
  return true;
}

String normalizeModelPath(const String& inputPath) {
  String p = inputPath;
  p.trim();
  if (p.length() == 0) return String();
  if (!p.startsWith("/")) {
    p = String(SD_MODELS_DIR) + "/" + p;
  }
  if (!p.endsWith(".txt")) {
    p += ".txt";
  }
  if (!isSafeSdPath(p)) return String();
  return p;
}

String normalizeMusicPath(const String& inputPath) {
  String p = inputPath;
  p.trim();
  if (p.length() == 0) return String();
  if (!p.startsWith("/")) {
    p = String(SD_MUSIC_DIR) + "/" + p;
  }
  if (!isSafeMusicPath(p)) return String();
  return p;
}

String getAudioContentType(const String& path) {
  String p = path;
  p.toLowerCase();
  if (p.endsWith(".mp3")) return "audio/mpeg";
  if (p.endsWith(".wav")) return "audio/wav";
  if (p.endsWith(".ogg")) return "audio/ogg";
  return "application/octet-stream";
}

String readFileRawFromSd(const char* path, String* errorMsg) {
  if (!sdReady) {
    if (errorMsg) *errorMsg = "SD non initialisee";
    return String();
  }

  File f = SD.open(path, FILE_READ);
  if (!f) {
    if (errorMsg) *errorMsg = "Ouverture impossible";
    return String();
  }

  String content;
  while (f.available()) {
    content += char(f.read());
  }
  f.close();
  return content;
}

String jsonGetString(const String& json, const char* key, const String& defaultValue) {
  String pattern = "\"" + String(key) + "\"";
  int keyPos = json.indexOf(pattern);
  if (keyPos < 0) return defaultValue;

  int colon = json.indexOf(':', keyPos + pattern.length());
  if (colon < 0) return defaultValue;

  int i = colon + 1;
  while (i < (int)json.length() && (json.charAt(i) == ' ' || json.charAt(i) == '\n' || json.charAt(i) == '\r' || json.charAt(i) == '\t')) i++;
  if (i >= (int)json.length() || json.charAt(i) != '"') return defaultValue;

  i++;
  String out;
  while (i < (int)json.length()) {
    char c = json.charAt(i);
    if (c == '"') break;
    if (c == '\\' && i + 1 < (int)json.length()) {
      i++;
      c = json.charAt(i);
    }
    out += c;
    i++;
  }
  return out;
}

int jsonGetInt(const String& json, const char* key, int defaultValue) {
  String pattern = "\"" + String(key) + "\"";
  int keyPos = json.indexOf(pattern);
  if (keyPos < 0) return defaultValue;

  int colon = json.indexOf(':', keyPos + pattern.length());
  if (colon < 0) return defaultValue;

  int i = colon + 1;
  while (i < (int)json.length() && (json.charAt(i) == ' ' || json.charAt(i) == '\n' || json.charAt(i) == '\r' || json.charAt(i) == '\t')) i++;
  if (i >= (int)json.length()) return defaultValue;

  int start = i;
  if (json.charAt(i) == '-') i++;
  while (i < (int)json.length() && json.charAt(i) >= '0' && json.charAt(i) <= '9') i++;
  if (i == start || (i == start + 1 && json.charAt(start) == '-')) return defaultValue;

  return json.substring(start, i).toInt();
}

bool loadRuntimeConfigFromSdJson(String* errorMsg) {
  if (!sdReady) {
    if (errorMsg) *errorMsg = "SD non initialisee";
    return false;
  }
  if (!SD.exists(SD_CONFIG_JSON_FILE)) {
    if (errorMsg) *errorMsg = "config.json absent";
    return false;
  }

  String txt = readFileRawFromSd(SD_CONFIG_JSON_FILE, errorMsg);
  if (txt.length() == 0) {
    if (errorMsg && errorMsg->length() == 0) *errorMsg = "config.json vide";
    return false;
  }

  String ssid = jsonGetString(txt, "ap_ssid", runtimeApSsid);
  ssid.trim();
  if (ssid.length() == 0) ssid = AP_SSID_DEFAULT;

  String password = jsonGetString(txt, "ap_password", runtimeApPassword);
  password.trim();

  String staSsid = jsonGetString(txt, "ssid", runtimeStaSsid);
  staSsid.trim();

  String staPassword = jsonGetString(txt, "password", runtimeStaPass);
  staPassword.trim();

  int tx = constrain(jsonGetInt(txt, "wifi_tx_power", runtimeWiFiTxPower), 8, 84);
  int ledCountCfg = constrain(jsonGetInt(txt, "led_count", runtimeLedCountConfig), LED_COUNT_MIN, LED_COUNT_MAX);
  int maxCurrentCfg = constrain(jsonGetInt(txt, "max_current_ma", runtimeMaxCurrentMaConfig), MAX_CURRENT_MA_MIN, MAX_CURRENT_MA_MAX);

  runtimeApSsid = ssid;
  runtimeApPassword = password;
  runtimeStaSsid = staSsid;
  runtimeStaPass = staPassword;
  runtimeWiFiTxPower = (int8_t)tx;
  runtimeLedCountConfig = (uint8_t)ledCountCfg;
  runtimeMaxCurrentMaConfig = (uint16_t)maxCurrentCfg;

  String modeleToLoad = jsonGetString(txt, "modele_actif", "");
  modeleToLoad.trim();
  if (modeleToLoad.length() > 0 && modeleToLoad != "interne") {
    String loadErr;
    if (!loadModeleFromSd(modeleToLoad, &loadErr)) {
      if (errorMsg) *errorMsg = "config.json lu, modele_actif KO: " + loadErr;
      return false;
    }
  }

  return true;
}

uint8_t parseGroupMaskText(const String& token) {
  if (token.length() == 0) {
    return FRAME_GROUP_MASK_ALL;
  }

  uint8_t mask = 0;
  for (size_t i = 0; i < token.length(); i++) {
    char c = token.charAt(i);
    if (c >= '0' && c <= '5') {
      mask |= (uint8_t)(1U << (c - '0'));
    }
  }
  return (mask == 0) ? FRAME_GROUP_MASK_ALL : mask;
}

bool parseFrameLine(const String& line, Frame& outFrame) {
  int p1 = line.indexOf(',');
  if (p1 < 0) return false;
  int p2 = line.indexOf(',', p1 + 1);
  if (p2 < 0) return false;
  int p3 = line.indexOf(',', p2 + 1);
  if (p3 < 0) return false;
  int p4 = line.indexOf(',', p3 + 1);
  if (p4 < 0) return false;
  int p5 = line.indexOf(',', p4 + 1);

  int r = line.substring(0, p1).toInt();
  int g = line.substring(p1 + 1, p2).toInt();
  int b = line.substring(p2 + 1, p3).toInt();
  int brightness = line.substring(p3 + 1, p4).toInt();

  int duration;
  String groupText;
  if (p5 < 0) {
    duration = line.substring(p4 + 1).toInt();
    groupText = "";
  } else {
    duration = line.substring(p4 + 1, p5).toInt();
    groupText = line.substring(p5 + 1);
    groupText.trim();
  }

  outFrame = Frame((uint8_t)constrain(r, 0, 255),
                   (uint8_t)constrain(g, 0, 255),
                   (uint8_t)constrain(b, 0, 255),
                   (uint8_t)constrain(brightness, 0, 255),
                   (uint16_t)constrain(duration, 1, 60000),
                   parseGroupMaskText(groupText));
  return true;
}

bool loadModeleFromSd(const String& inputPath, String* errorMsg = nullptr) {
  if (!sdReady) {
    if (errorMsg) *errorMsg = "SD non initialisee";
    return false;
  }

  String path = normalizeModelPath(inputPath);
  if (path.length() == 0) {
    if (errorMsg) *errorMsg = "Chemin modele invalide";
    return false;
  }

  if (!SD.exists(path)) {
    if (errorMsg) *errorMsg = "Fichier introuvable";
    return false;
  }

  File f = SD.open(path, FILE_READ);
  if (!f) {
    if (errorMsg) *errorMsg = "Ouverture impossible";
    return false;
  }

  size_t loaded = 0;
  int lineNum = 0;
  while (f.available()) {
    String line = f.readStringUntil('\n');
    lineNum++;
    line.trim();
    if (line.length() == 0 || line.startsWith("#")) {
      continue;
    }

    if (loaded >= MAX_SD_FRAMES) {
      f.close();
      if (errorMsg) *errorMsg = "Trop de frames";
      return false;
    }

    Frame frame = Frame(0, 0, 0, 0, 1);
    if (!parseFrameLine(line, frame)) {
      f.close();
      if (errorMsg) {
        *errorMsg = "Ligne invalide: " + String(lineNum);
      }
      return false;
    }

    modeleSD[loaded++] = frame;
  }
  f.close();

  if (loaded == 0) {
    if (errorMsg) *errorMsg = "Aucune frame valide";
    return false;
  }

  modeleSDCount = loaded;
  modeleSDActive = true;
  modeleSourcePath = path;
  logEvent("Modele SD charge: " + path + " (" + String(loaded) + " frames, " + formatDurationSec(getCurrentModeleDurationMs()) + ")");
  return true;
}

String readTextFileFromSd(const String& inputPath, String* errorMsg = nullptr) {
  if (!sdReady) {
    if (errorMsg) *errorMsg = "SD non initialisee";
    return String();
  }

  String path = normalizeModelPath(inputPath);
  if (path.length() == 0) {
    if (errorMsg) *errorMsg = "Chemin invalide";
    return String();
  }

  File f = SD.open(path, FILE_READ);
  if (!f) {
    if (errorMsg) *errorMsg = "Ouverture impossible";
    return String();
  }

  String content;
  while (f.available()) {
    content += char(f.read());
  }
  f.close();
  return content;
}

bool writeTextFileToSd(const String& rawPath, const String& content, String* errorMsg = nullptr) {
  if (!sdReady) {
    if (errorMsg) *errorMsg = "SD non initialisee";
    return false;
  }

  String path = normalizeModelPath(rawPath);
  if (path.length() == 0) {
    if (errorMsg) *errorMsg = "Chemin invalide";
    return false;
  }

  int slash = path.lastIndexOf('/');
  if (slash > 0) {
    String dir = path.substring(0, slash);
    if (!SD.exists(dir)) {
      if (!SD.mkdir(dir)) {
        if (errorMsg) *errorMsg = "Creation dossier impossible";
        return false;
      }
    }
  }

  if (SD.exists(path)) {
    SD.remove(path);
  }

  File f = SD.open(path, FILE_WRITE);
  if (!f) {
    if (errorMsg) *errorMsg = "Ouverture ecriture impossible";
    return false;
  }

  f.seek(0);
  f.print(content);
  f.close();

  logEvent("Fichier SD ecrit: " + path + " (" + String(content.length()) + " octets)");
  return true;
}


bool writeTextFileChunkToSd(const String& rawPath, const String& content, bool append, String* errorMsg) {
  if (!sdReady) {
    if (errorMsg) *errorMsg = "SD non initialisee";
    return false;
  }

  String path = normalizeModelPath(rawPath);
  if (path.length() == 0) {
    if (errorMsg) *errorMsg = "Chemin invalide";
    return false;
  }

  int slash = path.lastIndexOf('/');
  if (slash > 0) {
    String dir = path.substring(0, slash);
    if (!SD.exists(dir)) {
      if (!SD.mkdir(dir)) {
        if (errorMsg) *errorMsg = "Creation dossier impossible";
        return false;
      }
    }
  }

  if (!append && SD.exists(path)) {
    SD.remove(path);
  }

  File f = SD.open(path, append ? FILE_APPEND : FILE_WRITE);
  if (!f) {
    if (errorMsg) *errorMsg = "Ouverture ecriture impossible";
    return false;
  }

  if (content.length() > 0) {
    f.print(content);
  }
  f.close();
  return true;
}


String getModelsJson() {
  String json = "{";
  json += "\"sdReady\":";
  json += (sdReady ? "true" : "false");
  json += ",\"current\":\"" + modeleSourcePath + "\",";
  json += "\"currentFrames\":" + String(getFrameCount()) + ",";
  json += "\"currentDurationMs\":" + String(getCurrentModeleDurationMs()) + ",";
  json += "\"models\":[";

  bool first = true;
  if (sdReady && SD.exists(SD_MODELS_DIR)) {
    File dir = SD.open(SD_MODELS_DIR);
    if (dir) {
      File entry = dir.openNextFile();
      while (entry) {
        if (!entry.isDirectory()) {
          String name = String(entry.name());
          if (!first) json += ",";
          json += "\"" + name + "\"";
          first = false;
        }
        entry.close();
        entry = dir.openNextFile();
      }
      dir.close();
    }
  }

  json += "]}";
  return json;
}

String getMusicListJson() {
  String json = "{";
  json += "\"sdReady\":";
  json += (sdReady ? "true" : "false");
  json += ",\"files\":[";

  bool first = true;
  if (sdReady && SD.exists(SD_MUSIC_DIR)) {
    File dir = SD.open(SD_MUSIC_DIR);
    if (dir) {
      File entry = dir.openNextFile();
      while (entry) {
        if (!entry.isDirectory()) {
          String name = String(entry.name());
          if (hasAllowedAudioExt(name)) {
            int slash = name.lastIndexOf('/');
            if (slash >= 0) {
              name = name.substring(slash + 1);
            }
            if (!first) json += ",";
            json += "\"" + name + "\"";
            first = false;
          }
        }
        entry.close();
        entry = dir.openNextFile();
      }
      dir.close();
    }
  }

  json += "]}";
  return json;
}
const Frame& getFrameByIndex(size_t idx) {
  if (modeleSDActive) return modeleSD[idx];
  return modele[idx];
}

size_t getFrameCount() {
  if (modeleSDActive) return modeleSDCount;
  return NUM_FRAMES;
}

uint32_t getCurrentModeleDurationMs() {
  uint32_t totalMs = 0;
  size_t totalFrames = getFrameCount();
  for (size_t i = 0; i < totalFrames; i++) {
    totalMs += getFrameByIndex(i).durationMs;
  }
  return totalMs;
}

String formatDurationSec(uint32_t durationMs) {
  uint32_t sec = durationMs / 1000;
  uint32_t dec = (durationMs % 1000) / 100; // 1 decimale
  return String(sec) + "." + String(dec) + "s";
}

void initSd() {
  SPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
  sdReady = SD.begin(SD_CS);

  if (!sdReady) {
    logEvent("SD absente ou init KO");
    modeleSDActive = false;
    modeleSourcePath = "interne";
    return;
  }

  if (!SD.exists(SD_MODELS_DIR)) {
    SD.mkdir(SD_MODELS_DIR);
  }
  if (!SD.exists(SD_MUSIC_DIR)) {
    SD.mkdir(SD_MUSIC_DIR);
  }

  if (SD.exists(SD_PRIMARY_MODEL)) {
    String err;
    if (!loadModeleFromSd(SD_PRIMARY_MODEL, &err)) {
      logEvent("Echec autoload modele principal: " + err);
      modeleSDActive = false;
      modeleSourcePath = "interne";
    }
  } else {
    modeleSDActive = false;
    modeleSourcePath = "interne";
    logEvent("Modele principal absent -> modele interne");
  }
}

// =====================
// playModele : envoie les frames du modèle sans bloquer loop()
// =====================
void playModele() {
  if (!modeleActif) return;

  unsigned long now = millis();
  if ((long)(now - nextFrameAtMs) < 0) return;

  size_t totalFrames = getFrameCount();
  if ((size_t)frameIndex >= totalFrames) {
    modeleActif = false;
    sendCommandeESPNowRepete(modeleMaskActif, EFFET_MODELE_END);
    logEvent("FIN MODELE " + modeleSourcePath);
    return;
  }

  const Frame& frame = getFrameByIndex((size_t)frameIndex);

  uint8_t frameMask = (uint8_t)(modeleMaskActif & (frame.groupMask & FRAME_GROUP_MASK_ALL));
  if (frameMask != 0) {
    uint8_t msg[5] = {frameMask, frame.r, frame.g, frame.b, frame.brightness};

    esp_now_send(broadcastMAC, msg, 5);
  }

  nextFrameAtMs = now + frame.durationMs;
  frameIndex++;
}

// =====================
// sendESPNow : envoie un masque et un effet
// =====================
void sendCommandeESPNow(uint8_t masque, uint8_t effet) {
  uint8_t msg[2] = {masque, effet};
  esp_now_send(broadcastMAC, msg, 2);
}

void sendCommandeESPNowRepete(uint8_t masque, uint8_t effet, uint8_t repeats, uint16_t spacingMs) {
  if (repeats == 0) return;
  for (uint8_t i = 0; i < repeats; i++) {
    sendCommandeESPNow(masque, effet);
    if (i + 1 < repeats) delay(spacingMs);
  }
}

void sendESPNow(uint8_t masque, uint8_t effet) {
  uint8_t masqueEffectif = (uint8_t)(masque & FRAME_GROUP_MASK_ALL);

  Serial.print("Envoi masque=");
  Serial.print(masqueEffectif);
  Serial.print(" effet=");
  Serial.println(effet);

  // Toujours envoyer le masque + effet (repete pour fiabilite)
  sendCommandeESPNowRepete(masqueEffectif, effet);

  if (effet == EFFET_MODELE_START) {
    if (masqueEffectif == 0) {
      Serial.println("Modele perso ignore: masque effectif = 0");
      modeleActif = false;
      return;
    }
    modeleActif = true;
    modeleMaskActif = masqueEffectif;
    frameIndex = 0;
    nextFrameAtMs = millis() + 50; // laisse le temps aux récepteurs sans bloquer
    logEvent("Lecture modele lancee: " + modeleSourcePath + " (" + String(getFrameCount()) + " frames, " + formatDurationSec(getCurrentModeleDurationMs()) + ")");
  } else {
    modeleActif = false;
  }
}

void sendCouleurFixe(uint8_t masque, uint8_t r, uint8_t g, uint8_t b, uint8_t brightness) {
  uint8_t meta = (masque & 0x7F) | FLAG_COULEUR_FIXE;
  uint8_t msg[5] = {meta, r, g, b, brightness};
  esp_now_send(broadcastMAC, msg, 5);

  Serial.print("Couleur fixe r=");
  Serial.print(r);
  Serial.print(" g=");
  Serial.print(g);
  Serial.print(" b=");
  Serial.print(b);
  Serial.print(" bri=");
  Serial.println(brightness);
}

void sendConfigGroupeLot(uint8_t groupe) {
  if (groupe < GROUPE_MIN || groupe > GROUPE_MAX) {
    return;
  }

  uint8_t effetCfg = EFFET_CFG_GROUPE_BASE + groupe;
  // Repetition plus forte pour fiabiliser en lot.
  sendCommandeESPNowRepete(MASQUE_TOUT_GROUPES, effetCfg, 5, 40);

  logEvent("CONFIG LOT -> groupe " + String(groupe));
}

void sendConfigLedCountLot(uint8_t ledCount) {
  uint8_t countSafe = ledCount;
  if (countSafe < LED_COUNT_MIN) countSafe = LED_COUNT_MIN;
  if (countSafe > LED_COUNT_MAX) countSafe = LED_COUNT_MAX;

  uint8_t msg[3] = {MASQUE_TOUT_GROUPES, CMD_CFG_LED_COUNT, countSafe};
  for (uint8_t i = 0; i < 5; i++) {
    esp_now_send(broadcastMAC, msg, sizeof(msg));
    if (i + 1 < 5) delay(40);
  }

  logEvent("CONFIG LOT -> ledCount " + String(countSafe));
}

void sendConfigMaxCurrentLot(uint16_t maxCurrentMa) {
  uint16_t currentSafe = maxCurrentMa;
  if (currentSafe < MAX_CURRENT_MA_MIN) currentSafe = MAX_CURRENT_MA_MIN;
  if (currentSafe > MAX_CURRENT_MA_MAX) currentSafe = MAX_CURRENT_MA_MAX;

  uint8_t msg[4] = {
    MASQUE_TOUT_GROUPES,
    CMD_CFG_MAX_CURRENT_MA,
    (uint8_t)(currentSafe & 0xFF),
    (uint8_t)((currentSafe >> 8) & 0xFF)
  };

  for (uint8_t i = 0; i < 5; i++) {
    esp_now_send(broadcastMAC, msg, sizeof(msg));
    if (i + 1 < 5) delay(40);
  }

  logEvent("CONFIG LOT -> maxCurrentMa " + String(currentSafe));
}

void sendConfigBrightnessLot(uint8_t masque, uint8_t brightness) {
  uint8_t value = brightness;
  uint8_t msg[3] = {masque, CMD_CFG_BRIGHTNESS, value};

  for (uint8_t i = 0; i < 5; i++) {
    esp_now_send(broadcastMAC, msg, sizeof(msg));
    if (i + 1 < 5) delay(40);
  }

  logEvent("CONFIG LOT -> luminosite " + String(value));
}

// =====================
// handleSend : gestion HTTP GET
// =====================
void handleSend() {
  if (!server.hasArg("g") || !server.hasArg("e")) {
    server.send(400, "text/plain", "Parametres manquants");
    return;
  }
  uint8_t masque = server.arg("g").toInt();
  uint8_t effet = server.arg("e").toInt();
  sendESPNow(masque, effet);

  // Effet 9: couleur fixe optionnelle via cr/cg/cb (0..255)
  if (effet == 9 && server.hasArg("cr") && server.hasArg("cg") && server.hasArg("cb")) {
    uint8_t r = (uint8_t)constrain(server.arg("cr").toInt(), 0, 255);
    uint8_t g = (uint8_t)constrain(server.arg("cg").toInt(), 0, 255);
    uint8_t b = (uint8_t)constrain(server.arg("cb").toInt(), 0, 255);
    sendCouleurFixe(masque, r, g, b, 255);
  }

  server.send(200, "text/plain", "OK");
}

void handleSetGroup() {
  if (!server.hasArg("g")) {
    server.send(400, "text/plain", "Parametre g manquant");
    return;
  }

  int g = server.arg("g").toInt();
  if (g < GROUPE_MIN || g > GROUPE_MAX) {
    server.send(400, "text/plain", "Groupe invalide (0..5)");
    return;
  }

  sendConfigGroupeLot((uint8_t)g);
  server.send(200, "text/plain", "CONFIG LOT OK");
}

void handleSetLedCount() {
  if (!server.hasArg("n")) {
    server.send(400, "text/plain", "Parametre n manquant");
    return;
  }

  int n = server.arg("n").toInt();
  if (n < LED_COUNT_MIN || n > LED_COUNT_MAX) {
    server.send(400, "text/plain", "Nombre de LEDs invalide (1..250)");
    return;
  }

  sendConfigLedCountLot((uint8_t)n);
  server.send(200, "text/plain", "CONFIG LED COUNT OK");
}

void handleSetMaxCurrent() {
  if (!server.hasArg("ma")) {
    server.send(400, "text/plain", "Parametre ma manquant");
    return;
  }

  int ma = server.arg("ma").toInt();
  if (ma < MAX_CURRENT_MA_MIN || ma > MAX_CURRENT_MA_MAX) {
    server.send(400, "text/plain", "Courant max invalide (100..10000 mA)");
    return;
  }

  sendConfigMaxCurrentLot((uint16_t)ma);
  server.send(200, "text/plain", "CONFIG MAX CURRENT OK");
}

void handleSetBrightness() {
  if (!server.hasArg("b")) {
    server.send(400, "text/plain", "Parametre b manquant");
    return;
  }

  int b = server.arg("b").toInt();
  if (b < 0 || b > 255) {
    server.send(400, "text/plain", "Luminosite invalide (0..255)");
    return;
  }

  uint8_t masque = MASQUE_TOUT_GROUPES;
  if (server.hasArg("g")) {
    masque = (uint8_t)server.arg("g").toInt();
  }

  sendConfigBrightnessLot(masque, (uint8_t)b);
  server.send(200, "text/plain", "CONFIG BRIGHTNESS OK");
}

void handleTestGroupe() {
  Serial.println("TEST GROUPE: envoi clignotement");
  sendCommandeESPNowRepete(MASQUE_TOUT_GROUPES, EFFET_TEST_GROUPE, 5, 40);
  server.send(200, "text/plain", "TEST GROUPE OK");
}

void handleRuntimeConfig() {
  String json = "{";
  json += "\"ledCount\":" + String(runtimeLedCountConfig) + ",";
  json += "\"maxCurrentMa\":" + String(runtimeMaxCurrentMaConfig) + ",";
  json += "\"activeReceivers\":" + String(countActiveReceivers());
  json += "}";
  server.send(200, "application/json", json);
}

void handleSdStatus() {
  String status = String("SD=") + (sdReady ? "OK" : "KO") +
                  " current=" + modeleSourcePath +
                  " frames=" + String(getFrameCount()) +
                  " duree=" + formatDurationSec(getCurrentModeleDurationMs());
  server.send(200, "text/plain", status);
}

void handleModels() {
  server.send(200, "application/json", getModelsJson());
}

void handleMusicList() {
  server.send(200, "application/json", getMusicListJson());
}

void handleMusicGet() {
  if (!sdReady) {
    server.send(400, "text/plain", "SD non initialisee");
    return;
  }
  if (!server.hasArg("path")) {
    server.send(400, "text/plain", "Parametre path manquant");
    return;
  }
  String path = normalizeMusicPath(server.arg("path"));
  if (path.length() == 0) {
    server.send(400, "text/plain", "Chemin invalide");
    return;
  }
  if (!SD.exists(path)) {
    server.send(404, "text/plain", "Fichier introuvable");
    return;
  }
  File f = SD.open(path, FILE_READ);
  if (!f) {
    server.send(500, "text/plain", "Lecture fichier impossible");
    return;
  }
  String ct = getAudioContentType(path);
  server.streamFile(f, ct);
  f.close();
}

void handleReadModel() {
  if (!server.hasArg("path")) {
    server.send(400, "text/plain", "Parametre path manquant");
    return;
  }

  String err;
  String txt = readTextFileFromSd(server.arg("path"), &err);
  if (txt.length() == 0 && err.length() > 0) {
    server.send(400, "text/plain", err);
    return;
  }
  server.send(200, "text/plain", txt);
}

void handleSaveModel() {
  if (!server.hasArg("path")) {
    server.send(400, "text/plain", "Parametre path manquant");
    return;
  }

  String body = server.arg("plain");
  if (body.length() == 0) {
    server.send(400, "text/plain", "Contenu vide");
    return;
  }

  String err;
  if (!writeTextFileToSd(server.arg("path"), body, &err)) {
    server.send(400, "text/plain", err);
    return;
  }

  server.send(200, "text/plain", "MODELE SAUVEGARDE");
}

void handleLoadModel() {
  if (!server.hasArg("path")) {
    server.send(400, "text/plain", "Parametre path manquant");
    return;
  }

  String err;
  if (!loadModeleFromSd(server.arg("path"), &err)) {
    server.send(400, "text/plain", err);
    return;
  }

  server.send(200, "text/plain", "MODELE CHARGE");
}

void handleSaveModelChunk() {
  if (!server.hasArg("path")) {
    server.send(400, "text/plain", "Parametre path manquant");
    return;
  }

  bool append = server.hasArg("append") && server.arg("append").toInt() != 0;
  String body = server.arg("plain");
  if (body.length() == 0) {
    server.send(400, "text/plain", "Contenu vide");
    return;
  }

  String err;
  if (!writeTextFileChunkToSd(server.arg("path"), body, append, &err)) {
    server.send(400, "text/plain", err);
    return;
  }

  server.send(200, "text/plain", "CHUNK OK");
}

void handleLogs() {
  if (!sdReady) {
    server.send(400, "text/plain", "SD non initialisee");
    return;
  }
  if (!SD.exists(SD_LOG_FILE)) {
    server.send(200, "text/plain", "Aucun log");
    return;
  }
  File f = SD.open(SD_LOG_FILE, FILE_READ);
  if (!f) {
    server.send(500, "text/plain", "Lecture logs impossible");
    return;
  }
  String txt;
  while (f.available()) {
    txt += char(f.read());
  }
  f.close();
  server.send(200, "text/plain", txt);
}


void handleBoutonModele() {
  bool reading = digitalRead(BTN_MODELE);
  unsigned long now = millis();

  if (reading != btnStateLastRead) {
    btnLastChangeMs = now;
    btnStateLastRead = reading;
  }

  if (now - btnLastChangeMs >= BTN_DEBOUNCE_MS && reading != btnStateStable) {
    btnStateStable = reading;
    if (btnStateStable == LOW) {
      Serial.println("Bouton modele: lancement");
      sendESPNow(MODELE_BOUTON_MASK, EFFET_MODELE_START);
    }
  }
}

// =====================
// setup : initialisation
// =====================
void setup() {
  Serial.begin(115200);
  delay(500);

  // LED émetteur
  pinMode(LED_EMETTEUR, OUTPUT);
  digitalWrite(LED_EMETTEUR, HIGH);
  pinMode(BTN_MODELE, INPUT_PULLUP);

  Serial.println("=== Emetteur LED APA102 ===");

  initSd();

  String cfgErr;
  if (loadRuntimeConfigFromSdJson(&cfgErr)) {
    logEvent("Config JSON chargee: SSID=" + runtimeApSsid +
             " tx=" + String(runtimeWiFiTxPower) +
             " ledCount=" + String(runtimeLedCountConfig) +
             " maxCurrentMa=" + String(runtimeMaxCurrentMaConfig));
  } else {
    logEvent("Config JSON non chargee: " + cfgErr + " -> defauts compilation");
  }

  // Wi-Fi STA (reseau local)
  WiFi.mode(WIFI_AP_STA);
  WiFi.setSleep(false); // evite une reprise lente apres inactivite web
  bool staOk = false;
  uint8_t wifiChannel = AP_CHANNEL;
  if (runtimeStaSsid.length() > 0) {
    WiFi.begin(runtimeStaSsid.c_str(), runtimeStaPass.c_str());
    unsigned long startMs = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - startMs) < 8000) {
      delay(50);
    }
    staOk = (WiFi.status() == WL_CONNECTED);
    if (staOk) {
      wifiChannel = (uint8_t)WiFi.channel();
      IPAddress ip = WiFi.localIP();
      Serial.print("IP STA: ");
      Serial.println(ip);
    } else {
      Serial.printf("WiFi non connecte a '%s'\n", runtimeStaSsid.c_str());
      WiFi.disconnect(true);
    }
  }

  // Fallback AP si pas de Wi-Fi local
  if (!staOk) {
    bool apOk = false;
    if (runtimeApPassword.length() >= 8) {
      apOk = WiFi.softAP(runtimeApSsid.c_str(), runtimeApPassword.c_str(), AP_CHANNEL);
    } else {
      apOk = WiFi.softAP(runtimeApSsid.c_str(), nullptr, AP_CHANNEL);
    }
    if (!apOk) {
      Serial.println("Erreur demarrage AP");
    }
    IPAddress ip = WiFi.softAPIP();
    Serial.print("IP AP: ");
    Serial.println(ip);
  }

  esp_wifi_set_max_tx_power(runtimeWiFiTxPower); // Puissance radio fixee pour stabilite

  // mDNS
  if (MDNS.begin("danse")) {
    Serial.println("mDNS actif: http://danse.local");
  } else {
    Serial.println("Erreur mDNS");
  }

  // ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Erreur init ESP-NOW");
    return;
  }

  esp_now_register_recv_cb(onDataRecv);

  // Peer broadcast
  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, broadcastMAC, 6);
  peer.channel = wifiChannel;
  peer.encrypt = false;

  if (esp_now_add_peer(&peer) != ESP_OK) {
    Serial.println("Erreur ajout peer broadcast");
  } else {
    Serial.println("Peer broadcast ajoute");
  }

  // Serveur web
  server.on("/", []() {
    server.sendHeader("Location", "/effets", true);
    server.send(302, "text/plain", "Redirect /effets");
  });
  server.on("/effets", []() { server.send(200, "text/html", webPagePageEffets); });
  server.on("/modele", []() { server.send(200, "text/html", webPagePageModele); });
  server.on("/config", []() { server.send(200, "text/html", webPagePageConfig); });
  server.on("/modele_generator.js", []() { server.send(200, "application/javascript", webPageAssetModeleGeneratorJs); });
  server.on("/send", HTTP_GET, handleSend);
  server.on("/setgroup", HTTP_GET, handleSetGroup);
  server.on("/setledcount", HTTP_GET, handleSetLedCount);
  server.on("/setmaxcurrent", HTTP_GET, handleSetMaxCurrent);
  server.on("/setbrightness", HTTP_GET, handleSetBrightness);
  server.on("/testgroupe", HTTP_GET, handleTestGroupe);
  server.on("/runtimecfg", HTTP_GET, handleRuntimeConfig);
  server.on("/sdstatus", HTTP_GET, handleSdStatus);
  server.on("/models", HTTP_GET, handleModels);
  server.on("/musique/list", HTTP_GET, handleMusicList);
  server.on("/musique/get", HTTP_GET, handleMusicGet);
  server.on("/readmodel", HTTP_GET, handleReadModel);
  server.on("/loadmodel", HTTP_GET, handleLoadModel);
  server.on("/savemodel", HTTP_POST, handleSaveModel);
  server.on("/savemodelchunk", HTTP_POST, handleSaveModelChunk);
  server.on("/logs", HTTP_GET, handleLogs);
  server.begin();
  Serial.println("Serveur web demarre");
}

// =====================
// loop : traitement web uniquement
// =====================
void loop() {
  server.handleClient(); // web toujours actif
  handleBoutonModele();
  playModele();
}
