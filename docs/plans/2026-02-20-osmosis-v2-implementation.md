# Osmosis v2.0 Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Evolve Osmosis from a single-language Spanish flashcard device into a multi-language learning platform with WiFi connectivity, downloadable language packs from a static CDN, and phonetic pronunciation support.

**Architecture:** Three layers: (1) Python content pipeline generates language packs from CSV vocab files + Apple Color Emoji + per-language VLW fonts, (2) VCodeWorks.dev static CDN on Cloudflare Pages hosts catalog.json + pack files, (3) ESP32 firmware v2.0 adds WiFi manager with captive portal, pack downloader, dynamic vocab loader from manifest.json, and expanded settings UI.

**Tech Stack:** ESP32 (esp32dev), PlatformIO, Arduino, TFT_eSPI, XPT2046_Touchscreen, SPIFFS, Preferences, WiFi, HTTPClient, ArduinoJson. Python 3 + Pillow + PyObjC for content pipeline. Cloudflare Pages for CDN.

**Design doc:** `docs/plans/2026-02-20-osmosis-v2-design.md`

**Key constants:**
- Display: 240x320 portrait, strip-based rendering (240x10 sprites, 32 strips)
- Emoji: 96x96 source PNG → ORLE .bin, displayed at 120x120 via nearest-neighbor
- Emoji naming: by Unicode codepoint (e.g. `1f4a7.bin`), shared across all packs
- Fonts: per-language VLW smooth font files (~25-35KB each)
- Pack structure: manifest.json + font.vlw per language/tier, shared emoji/ directory
- SPIFFS: 2.5MB (custom OTA partition table)
- Languages: Spanish, French, Portuguese, Chinese (Mandarin pinyin), Dutch
- Tiers: Beginner, Intermediate, Advanced (300 words each)

---

## Task 1: Custom OTA Partition Table

**Files:**
- Create: `partitions_ota.csv`
- Modify: `platformio.ini`

**Step 1: Create the custom partition table**

Create `partitions_ota.csv` in project root with OTA support and 2.5MB SPIFFS:

```csv
# Name,   Type, SubType, Offset,   Size,     Flags
nvs,      data, nvs,     0x9000,   0x5000,
otadata,  data, ota,     0xE000,   0x2000,
app0,     app,  ota_0,   0x10000,  0xC0000,
app1,     app,  ota_1,   0xD0000,  0xC0000,
spiffs,   data, spiffs,  0x190000, 0x270000,
```

Layout: 20KB NVS + 8KB OTA metadata + 768KB app0 + 768KB app1 + 2.5MB SPIFFS = 4MB total.

**Step 2: Update platformio.ini**

Change the partition reference and add ArduinoJson dependency:

```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
monitor_speed = 115200
board_build.filesystem = spiffs
board_build.partitions = partitions_ota.csv

lib_deps =
    bodmer/TFT_eSPI@^2.5.43
    https://github.com/PaulStoffregen/XPT2046_Touchscreen.git
    bblanchon/ArduinoJson@^7.0.0

build_flags =
    -D USER_SETUP_LOADED=1
    -D ILI9341_DRIVER=1
    -D TFT_WIDTH=240
    -D TFT_HEIGHT=320
    -D TFT_MISO=-1
    -D TFT_MOSI=13
    -D TFT_SCLK=14
    -D TFT_CS=15
    -D TFT_DC=2
    -D TFT_RST=4
    -D SPI_FREQUENCY=40000000
    -D SPI_READ_FREQUENCY=20000000
    -D LOAD_GLCD=1  -D LOAD_FONT2=1  -D LOAD_FONT4=1
    -D LOAD_FONT6=1  -D LOAD_FONT7=1  -D LOAD_FONT8=1
    -D LOAD_GFXFF=1  -D SMOOTH_FONT=1
```

**Step 3: Build to verify partition table compiles**

Run: `cd /Users/kevintomlinson/Coding/Osmosis && pio run`
Expected: BUILD SUCCESS (may show new partition layout warnings, that's OK)

**Step 4: Commit**

```bash
git add partitions_ota.csv platformio.ini
git commit -m "feat: add OTA partition table with 2.5MB SPIFFS for v2.0"
```

---

## Task 2: Update Constants and Settings for v2.0

**Files:**
- Modify: `src/constants.h`
- Modify: `src/settings_manager.h`
- Modify: `src/settings_manager.cpp`

**Step 1: Update constants.h**

Update firmware version to 2.0.0 and change image source size from 72 to 96:

In `src/constants.h`, change:
- `FW_VERSION` from `"1.0.0"` to `"2.0.0"`
- `IMG_W` from `72` to `96`
- `IMG_H` from `72` to `96`
- Update comment from `72x72 stored` to `96x96 stored`

```cpp
constexpr const char* FW_VERSION = "2.0.0";

// Image area (96x96 stored, displayed at ~1.25x = 120x120 via nearest-neighbor)
constexpr int IMG_W = 96;
constexpr int IMG_H = 96;
```

**Step 2: Expand OsmosisSettings struct**

In `src/settings_manager.h`, add v2.0 fields to the struct:

```cpp
struct OsmosisSettings {
    // Existing v1
    uint8_t wordsPerDay;
    uint16_t displaySecs;
    uint8_t brightness;
    uint16_t progressIndex;
    uint8_t lastDay;
    // v2.0
    bool     showPhonetic;
    bool     wifiConfigured;
    char     wifiSSID[33];
    char     wifiPass[65];
    char     installedLang[16];
    char     installedTier[16];
    uint8_t  installedVer;
};
```

**Step 3: Update settings_manager.cpp**

Add load/save for new fields. In `setDefaults()`:

```cpp
_settings.showPhonetic = false;
_settings.wifiConfigured = false;
memset(_settings.wifiSSID, 0, sizeof(_settings.wifiSSID));
memset(_settings.wifiPass, 0, sizeof(_settings.wifiPass));
memset(_settings.installedLang, 0, sizeof(_settings.installedLang));
memset(_settings.installedTier, 0, sizeof(_settings.installedTier));
_settings.installedVer = 0;
```

In `load()`, add after existing loads:

```cpp
_settings.showPhonetic = _prefs.getBool("phonetic", false);
_settings.wifiConfigured = _prefs.getBool("wificfg", false);
_prefs.getString("ssid", _settings.wifiSSID, sizeof(_settings.wifiSSID));
_prefs.getString("wpass", _settings.wifiPass, sizeof(_settings.wifiPass));
_prefs.getString("lang", _settings.installedLang, sizeof(_settings.installedLang));
_prefs.getString("tier", _settings.installedTier, sizeof(_settings.installedTier));
_settings.installedVer = _prefs.getUChar("packver", 0);
```

In `save()`, add after existing saves:

```cpp
_prefs.putBool("phonetic", _settings.showPhonetic);
_prefs.putBool("wificfg", _settings.wifiConfigured);
_prefs.putString("ssid", _settings.wifiSSID);
_prefs.putString("wpass", _settings.wifiPass);
_prefs.putString("lang", _settings.installedLang);
_prefs.putString("tier", _settings.installedTier);
_prefs.putUChar("packver", _settings.installedVer);
```

**Step 4: Build to verify**

Run: `cd /Users/kevintomlinson/Coding/Osmosis && pio run`
Expected: BUILD SUCCESS

**Step 5: Commit**

```bash
git add src/constants.h src/settings_manager.h src/settings_manager.cpp
git commit -m "feat: expand settings for v2.0 (WiFi, language, phonetic)"
```

---

## Task 3: Vocab Loader — Dynamic Word Data from manifest.json

This replaces the compile-time `word_data.h` with runtime JSON parsing from SPIFFS.

**Files:**
- Create: `src/vocab_loader.h`
- Create: `src/vocab_loader.cpp`
- Modify: `src/word_data.h` (keep struct, remove PROGMEM data)
- Modify: `src/card_manager.h`
- Modify: `src/card_manager.cpp`

**Step 1: Create vocab_loader.h**

```cpp
#pragma once
#include <cstdint>

struct WordEntry {
    char word[32];       // Foreign word
    char english[32];    // English translation
    char phonetic[40];   // Phonetic pronunciation
    char emoji[12];      // Codepoint hex string (e.g. "1f4a7")
    char category[16];   // Category
};

struct PackInfo {
    char language[16];
    char languageDisplay[24];
    char tier[16];
    char tierDisplay[16];
    uint8_t version;
    uint16_t wordCount;
    char fontFile[32];
};

namespace vocabLoader {
    bool load();                          // Parse /manifest.json from SPIFFS
    bool isLoaded();
    const WordEntry* words();             // Pointer to word array
    uint16_t wordCount();
    const PackInfo& packInfo();
}
```

**Step 2: Create vocab_loader.cpp**

```cpp
#include "vocab_loader.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <FS.h>
#include <SPIFFS.h>
#include <cstring>

static WordEntry* _words = nullptr;
static uint16_t _wordCount = 0;
static PackInfo _packInfo;
static bool _loaded = false;

namespace vocabLoader {

bool load() {
    // Free previous data
    if (_words) {
        free(_words);
        _words = nullptr;
    }
    _wordCount = 0;
    _loaded = false;
    memset(&_packInfo, 0, sizeof(_packInfo));

    fs::File f = SPIFFS.open("/manifest.json", "r");
    if (!f) {
        Serial.println("[vocab] No manifest.json found on SPIFFS");
        return false;
    }

    size_t fileSize = f.size();
    Serial.printf("[vocab] Loading manifest.json (%u bytes)\n", (uint32_t)fileSize);

    // Parse JSON — use a filter to reduce memory usage
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, f);
    f.close();

    if (err) {
        Serial.printf("[vocab] JSON parse error: %s\n", err.c_str());
        return false;
    }

    // Extract pack info
    strlcpy(_packInfo.language, doc["language"] | "", sizeof(_packInfo.language));
    strlcpy(_packInfo.languageDisplay, doc["languageDisplay"] | "", sizeof(_packInfo.languageDisplay));
    strlcpy(_packInfo.tier, doc["tier"] | "", sizeof(_packInfo.tier));
    strlcpy(_packInfo.tierDisplay, doc["tierDisplay"] | "", sizeof(_packInfo.tierDisplay));
    _packInfo.version = doc["version"] | 0;
    _packInfo.wordCount = doc["wordCount"] | 0;
    strlcpy(_packInfo.fontFile, doc["fontFile"] | "", sizeof(_packInfo.fontFile));

    // Extract words array
    JsonArray wordsArr = doc["words"];
    if (wordsArr.isNull()) {
        Serial.println("[vocab] No 'words' array in manifest");
        return false;
    }

    _wordCount = wordsArr.size();
    if (_wordCount == 0) {
        Serial.println("[vocab] Empty words array");
        return false;
    }

    // Allocate word array on heap
    _words = (WordEntry*)malloc(_wordCount * sizeof(WordEntry));
    if (!_words) {
        Serial.printf("[vocab] Failed to allocate %u bytes for %u words\n",
                      (uint32_t)(_wordCount * sizeof(WordEntry)), _wordCount);
        _wordCount = 0;
        return false;
    }
    memset(_words, 0, _wordCount * sizeof(WordEntry));

    for (uint16_t i = 0; i < _wordCount; i++) {
        JsonObject w = wordsArr[i];
        strlcpy(_words[i].word, w["word"] | "", sizeof(_words[i].word));
        strlcpy(_words[i].english, w["english"] | "", sizeof(_words[i].english));
        strlcpy(_words[i].phonetic, w["phonetic"] | "", sizeof(_words[i].phonetic));
        strlcpy(_words[i].emoji, w["emoji"] | "", sizeof(_words[i].emoji));
        strlcpy(_words[i].category, w["category"] | "", sizeof(_words[i].category));
    }

    _loaded = true;
    Serial.printf("[vocab] Loaded %u words (%s %s)\n",
                  _wordCount, _packInfo.languageDisplay, _packInfo.tierDisplay);
    return true;
}

bool isLoaded() { return _loaded; }
const WordEntry* words() { return _words; }
uint16_t wordCount() { return _wordCount; }
const PackInfo& packInfo() { return _packInfo; }

}  // namespace vocabLoader
```

**Step 3: Simplify word_data.h**

Replace the entire file — remove the 300-entry PROGMEM array, keep only backward-compatible references:

```cpp
#pragma once
#include "vocab_loader.h"

// v2.0: Word data now loaded dynamically from /manifest.json
// WORD_LIST and WORD_COUNT are compatibility shims for card_manager
#define WORD_LIST  (vocabLoader::words())
#define WORD_COUNT (vocabLoader::wordCount())
```

**Step 4: Update card_manager.h**

Remove `#include "word_data.h"`, add `#include "vocab_loader.h"`:

```cpp
#pragma once
#include "vocab_loader.h"
#include <cstdint>

class CardManager {
public:
    void init();
    bool update();
    void nextCard();
    void checkDayChange();

    const WordEntry& currentWord() const;
    int currentCardIndex() const;
    int totalCardsToday() const;

private:
    uint16_t _dailyBatch[25];
    uint8_t _batchSize = 0;
    uint8_t _currentPos = 0;
    uint32_t _lastCardChangeMs = 0;

    void buildBatch();
    void loadCurrentImage();
    void shuffleBatch();
};

extern CardManager cardMgr;
```

**Step 5: Update card_manager.cpp**

Change `loadCurrentImage()` to use emoji codepoint instead of imageFile:

```cpp
void CardManager::loadCurrentImage() {
    const WordEntry& word = currentWord();
    // v2.0: load by emoji codepoint (e.g. "1f4a7") instead of word-based filename
    imageRenderer::preloadImage(word.emoji);
    Serial.printf("[card] Card %d/%d: %s (%s)\n",
                  currentCardIndex(), totalCardsToday(),
                  word.word, word.english);
}
```

Also update `currentWord()` and references from `word.spanish` to `word.word` throughout `card_manager.cpp`. The `currentWord()` function changes to:

```cpp
const WordEntry& CardManager::currentWord() const {
    return WORD_LIST[_dailyBatch[_currentPos]];
}
```

This still works since `WORD_LIST` is now a macro pointing to `vocabLoader::words()`.

**Step 6: Build to verify**

Run: `cd /Users/kevintomlinson/Coding/Osmosis && pio run`
Expected: BUILD SUCCESS (note: will need a manifest.json on SPIFFS to actually run)

**Step 7: Commit**

```bash
git add src/vocab_loader.h src/vocab_loader.cpp src/word_data.h src/card_manager.h src/card_manager.cpp
git commit -m "feat: dynamic vocab loader replaces compile-time word data"
```

---

## Task 4: WiFi Manager with Captive Portal

**Files:**
- Create: `src/wifi_manager.h`
- Create: `src/wifi_manager.cpp`

**Step 1: Create wifi_manager.h**

```cpp
#pragma once
#include <cstdint>

enum class WiFiState : uint8_t {
    NotConfigured,
    CaptivePortalActive,
    Connecting,
    Connected,
    Disconnected
};

namespace wifiMgr {
    void init();                  // Check NVS for saved creds, attempt connect
    void startCaptivePortal();    // Launch AP "Osmosis-Setup" with config page
    void stopCaptivePortal();
    void connect();               // Connect using saved credentials
    void disconnect();
    void update();                // Call in loop() — handles DNS, portal requests

    WiFiState state();
    bool isConnected();
    const char* ssid();           // Current/saved SSID
    int8_t rssi();                // Signal strength (when connected)
}
```

**Step 2: Create wifi_manager.cpp**

```cpp
#include "wifi_manager.h"
#include "settings_manager.h"
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>

static WiFiState _state = WiFiState::NotConfigured;
static WebServer* _server = nullptr;
static DNSServer* _dns = nullptr;
static uint32_t _connectStartMs = 0;
static const uint32_t CONNECT_TIMEOUT_MS = 15000;

// Captive portal HTML — minimal form for SSID + password
static const char PORTAL_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html><head>
<meta name="viewport" content="width=device-width,initial-scale=1">
<style>
body{font-family:sans-serif;background:#1a1a2e;color:#e0e0e0;margin:0;padding:20px;text-align:center}
h1{color:#4fc3f7;margin-top:40px}
input{width:80%;padding:12px;margin:8px 0;border:1px solid #4fc3f7;border-radius:6px;background:#0d0d1a;color:#fff;font-size:16px}
button{width:84%;padding:14px;margin-top:16px;background:#4fc3f7;color:#000;border:none;border-radius:6px;font-size:18px;font-weight:bold;cursor:pointer}
.sub{color:#80b0cc;font-size:14px;margin-top:4px}
</style>
</head><body>
<h1>OSMOSIS</h1>
<p class="sub">WiFi Setup</p>
<form action="/save" method="POST">
<input name="ssid" placeholder="WiFi Network Name" required><br>
<input name="pass" type="password" placeholder="Password" required><br>
<button type="submit">Connect</button>
</form>
</body></html>
)rawliteral";

static void handleRoot() {
    _server->send(200, "text/html", PORTAL_HTML);
}

static void handleSave() {
    String ssid = _server->arg("ssid");
    String pass = _server->arg("pass");

    if (ssid.length() == 0) {
        _server->send(400, "text/html", "<h1>SSID required</h1>");
        return;
    }

    // Save to settings
    OsmosisSettings& s = settingsMgr.settings();
    strlcpy(s.wifiSSID, ssid.c_str(), sizeof(s.wifiSSID));
    strlcpy(s.wifiPass, pass.c_str(), sizeof(s.wifiPass));
    s.wifiConfigured = true;
    settingsMgr.save();

    _server->send(200, "text/html",
        "<html><body style='font-family:sans-serif;background:#1a1a2e;color:#e0e0e0;text-align:center;padding:40px'>"
        "<h1 style='color:#4fc3f7'>Saved!</h1>"
        "<p>Osmosis will now connect to your WiFi.</p>"
        "<p>You can close this page.</p>"
        "</body></html>");

    // Stop portal after short delay to let response send
    delay(1000);
    wifiMgr::stopCaptivePortal();
    wifiMgr::connect();
}

static void handleNotFound() {
    // Redirect all requests to the portal (captive portal behavior)
    _server->sendHeader("Location", "http://192.168.4.1/");
    _server->send(302, "text/plain", "");
}

namespace wifiMgr {

void init() {
    const OsmosisSettings& s = settingsMgr.settings();
    if (s.wifiConfigured && strlen(s.wifiSSID) > 0) {
        connect();
    } else {
        _state = WiFiState::NotConfigured;
        Serial.println("[wifi] No saved credentials");
    }
}

void startCaptivePortal() {
    Serial.println("[wifi] Starting captive portal...");

    WiFi.mode(WIFI_AP);
    WiFi.softAP("Osmosis-Setup");
    delay(100);

    Serial.printf("[wifi] AP IP: %s\n", WiFi.softAPIP().toString().c_str());

    // DNS server redirects all domains to our IP
    if (!_dns) _dns = new DNSServer();
    _dns->start(53, "*", WiFi.softAPIP());

    // Web server
    if (!_server) _server = new WebServer(80);
    _server->on("/", handleRoot);
    _server->on("/save", HTTP_POST, handleSave);
    _server->onNotFound(handleNotFound);
    _server->begin();

    _state = WiFiState::CaptivePortalActive;
    Serial.println("[wifi] Captive portal active at http://192.168.4.1");
}

void stopCaptivePortal() {
    if (_server) {
        _server->stop();
        delete _server;
        _server = nullptr;
    }
    if (_dns) {
        _dns->stop();
        delete _dns;
        _dns = nullptr;
    }
    WiFi.softAPdisconnect(true);
    Serial.println("[wifi] Captive portal stopped");
}

void connect() {
    const OsmosisSettings& s = settingsMgr.settings();
    if (strlen(s.wifiSSID) == 0) {
        _state = WiFiState::NotConfigured;
        return;
    }

    WiFi.mode(WIFI_STA);
    WiFi.begin(s.wifiSSID, s.wifiPass);
    _state = WiFiState::Connecting;
    _connectStartMs = millis();
    Serial.printf("[wifi] Connecting to '%s'...\n", s.wifiSSID);
}

void disconnect() {
    WiFi.disconnect(true);
    _state = WiFiState::Disconnected;
}

void update() {
    // Handle captive portal DNS + HTTP
    if (_state == WiFiState::CaptivePortalActive) {
        if (_dns) _dns->processNextRequest();
        if (_server) _server->handleClient();
        return;
    }

    // Handle connection state
    if (_state == WiFiState::Connecting) {
        if (WiFi.status() == WL_CONNECTED) {
            _state = WiFiState::Connected;
            Serial.printf("[wifi] Connected! IP: %s\n", WiFi.localIP().toString().c_str());
        } else if (millis() - _connectStartMs > CONNECT_TIMEOUT_MS) {
            _state = WiFiState::Disconnected;
            Serial.println("[wifi] Connection timeout");
        }
    }

    // Detect disconnect while connected
    if (_state == WiFiState::Connected && WiFi.status() != WL_CONNECTED) {
        _state = WiFiState::Disconnected;
        Serial.println("[wifi] Connection lost");
    }
}

WiFiState state() { return _state; }
bool isConnected() { return _state == WiFiState::Connected; }
const char* ssid() { return settingsMgr.settings().wifiSSID; }
int8_t rssi() { return WiFi.RSSI(); }

}  // namespace wifiMgr
```

**Step 3: Build to verify**

Run: `cd /Users/kevintomlinson/Coding/Osmosis && pio run`
Expected: BUILD SUCCESS

**Step 4: Commit**

```bash
git add src/wifi_manager.h src/wifi_manager.cpp
git commit -m "feat: WiFi manager with captive portal for network setup"
```

---

## Task 5: Pack Manager — Download and Install Language Packs

**Files:**
- Create: `src/pack_manager.h`
- Create: `src/pack_manager.cpp`

**Step 1: Create pack_manager.h**

```cpp
#pragma once
#include <cstdint>
#include <ArduinoJson.h>

enum class PackDownloadState : uint8_t {
    Idle,
    FetchingCatalog,
    FetchingManifest,
    FetchingFont,
    FetchingEmoji,
    CleaningOrphans,
    Complete,
    Error
};

struct CatalogLanguage {
    char id[16];
    char name[24];
    char flag[4];
};

struct CatalogTier {
    char id[16];
    char name[16];
    uint16_t words;
    uint8_t version;
    uint32_t manifestSize;
    uint32_t fontSize;
};

namespace packMgr {
    // Catalog
    bool fetchCatalog();                        // Download catalog.json
    uint8_t languageCount();
    const CatalogLanguage& language(uint8_t i);
    uint8_t tierCount(uint8_t langIdx);
    const CatalogTier& tier(uint8_t langIdx, uint8_t tierIdx);

    // Download
    bool startDownload(uint8_t langIdx, uint8_t tierIdx);
    void update();                              // Call in loop() during download
    PackDownloadState state();
    uint8_t progressPercent();                  // 0-100
    const char* statusText();                   // Human-readable status

    // Installed pack
    bool hasInstalledPack();
}
```

**Step 2: Create pack_manager.cpp**

```cpp
#include "pack_manager.h"
#include "settings_manager.h"
#include "wifi_manager.h"
#include <Arduino.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <FS.h>
#include <SPIFFS.h>
#include <cstring>

static const char* BASE_URL = "https://vcodeworks.dev/api/osmosis";

static PackDownloadState _state = PackDownloadState::Idle;
static uint8_t _progress = 0;
static char _statusBuf[48] = "";

// Catalog data
static const uint8_t MAX_LANGUAGES = 10;
static const uint8_t MAX_TIERS = 4;
static CatalogLanguage _languages[MAX_LANGUAGES];
static CatalogTier _tiers[MAX_LANGUAGES][MAX_TIERS];
static uint8_t _langCount = 0;
static uint8_t _tierCounts[MAX_LANGUAGES] = {};
static bool _catalogLoaded = false;

// Download state
static uint8_t _dlLangIdx = 0;
static uint8_t _dlTierIdx = 0;
static uint16_t _emojiTotal = 0;
static uint16_t _emojiDone = 0;

// List of emoji codepoints from the downloaded manifest
static const uint16_t MAX_EMOJI = 350;
static char _emojiList[MAX_EMOJI][12];
static uint16_t _emojiCount = 0;

static bool httpDownloadToFile(const char* url, const char* path) {
    HTTPClient http;
    http.begin(url);
    http.setTimeout(15000);
    int code = http.GET();

    if (code != 200) {
        Serial.printf("[pack] HTTP %d for %s\n", code, url);
        http.end();
        return false;
    }

    fs::File f = SPIFFS.open(path, "w");
    if (!f) {
        Serial.printf("[pack] Failed to open %s for writing\n", path);
        http.end();
        return false;
    }

    WiFiClient* stream = http.getStreamPtr();
    uint8_t buf[512];
    int total = 0;
    while (http.connected() && stream->available()) {
        int len = stream->readBytes(buf, sizeof(buf));
        if (len > 0) {
            f.write(buf, len);
            total += len;
        }
    }

    f.close();
    http.end();
    Serial.printf("[pack] Downloaded %s (%d bytes)\n", path, total);
    return true;
}

static bool httpDownloadToSpiffs(const char* url, const char* spiffsPath) {
    return httpDownloadToFile(url, spiffsPath);
}

namespace packMgr {

bool fetchCatalog() {
    if (!wifiMgr::isConnected()) {
        Serial.println("[pack] WiFi not connected");
        return false;
    }

    _state = PackDownloadState::FetchingCatalog;
    strlcpy(_statusBuf, "Fetching catalog...", sizeof(_statusBuf));

    char url[128];
    snprintf(url, sizeof(url), "%s/catalog.json", BASE_URL);

    HTTPClient http;
    http.begin(url);
    http.setTimeout(10000);
    int code = http.GET();

    if (code != 200) {
        Serial.printf("[pack] Catalog fetch failed: HTTP %d\n", code);
        _state = PackDownloadState::Error;
        strlcpy(_statusBuf, "Catalog fetch failed", sizeof(_statusBuf));
        http.end();
        return false;
    }

    String payload = http.getString();
    http.end();

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, payload);
    if (err) {
        Serial.printf("[pack] Catalog JSON error: %s\n", err.c_str());
        _state = PackDownloadState::Error;
        return false;
    }

    // Parse languages
    JsonArray langs = doc["languages"];
    _langCount = 0;
    for (JsonObject lang : langs) {
        if (_langCount >= MAX_LANGUAGES) break;
        uint8_t li = _langCount;
        strlcpy(_languages[li].id, lang["id"] | "", sizeof(_languages[li].id));
        strlcpy(_languages[li].name, lang["name"] | "", sizeof(_languages[li].name));
        strlcpy(_languages[li].flag, lang["flag"] | "", sizeof(_languages[li].flag));

        _tierCounts[li] = 0;
        JsonArray tiers = lang["tiers"];
        for (JsonObject t : tiers) {
            if (_tierCounts[li] >= MAX_TIERS) break;
            uint8_t ti = _tierCounts[li];
            strlcpy(_tiers[li][ti].id, t["id"] | "", sizeof(_tiers[li][ti].id));
            strlcpy(_tiers[li][ti].name, t["name"] | "", sizeof(_tiers[li][ti].name));
            _tiers[li][ti].words = t["words"] | 0;
            _tiers[li][ti].version = t["version"] | 0;
            _tiers[li][ti].manifestSize = t["manifestSize"] | 0;
            _tiers[li][ti].fontSize = t["fontSize"] | 0;
            _tierCounts[li]++;
        }
        _langCount++;
    }

    _catalogLoaded = true;
    _state = PackDownloadState::Idle;
    Serial.printf("[pack] Catalog loaded: %d languages\n", _langCount);
    return true;
}

uint8_t languageCount() { return _langCount; }
const CatalogLanguage& language(uint8_t i) { return _languages[i]; }
uint8_t tierCount(uint8_t langIdx) { return _tierCounts[langIdx]; }
const CatalogTier& tier(uint8_t langIdx, uint8_t tierIdx) { return _tiers[langIdx][tierIdx]; }

bool startDownload(uint8_t langIdx, uint8_t tierIdx) {
    if (!wifiMgr::isConnected() || !_catalogLoaded) return false;
    if (langIdx >= _langCount || tierIdx >= _tierCounts[langIdx]) return false;

    _dlLangIdx = langIdx;
    _dlTierIdx = tierIdx;
    _progress = 0;
    _emojiDone = 0;
    _emojiCount = 0;

    const char* lang = _languages[langIdx].id;
    const char* tr = _tiers[langIdx][tierIdx].id;

    // Step 1: Download manifest.json
    _state = PackDownloadState::FetchingManifest;
    snprintf(_statusBuf, sizeof(_statusBuf), "Downloading %s...", _languages[langIdx].name);
    _progress = 5;

    char url[128];
    snprintf(url, sizeof(url), "%s/packs/%s/%s/manifest.json", BASE_URL, lang, tr);
    if (!httpDownloadToSpiffs(url, "/manifest.json")) {
        _state = PackDownloadState::Error;
        strlcpy(_statusBuf, "Manifest download failed", sizeof(_statusBuf));
        return false;
    }
    _progress = 15;

    // Step 2: Download font.vlw
    _state = PackDownloadState::FetchingFont;
    strlcpy(_statusBuf, "Downloading font...", sizeof(_statusBuf));

    snprintf(url, sizeof(url), "%s/packs/%s/%s/font.vlw", BASE_URL, lang, tr);
    if (!httpDownloadToSpiffs(url, "/font.vlw")) {
        _state = PackDownloadState::Error;
        strlcpy(_statusBuf, "Font download failed", sizeof(_statusBuf));
        return false;
    }
    _progress = 25;

    // Step 3: Parse manifest to get emoji list
    fs::File f = SPIFFS.open("/manifest.json", "r");
    if (!f) {
        _state = PackDownloadState::Error;
        return false;
    }

    JsonDocument doc;
    deserializeJson(doc, f);
    f.close();

    JsonArray wordsArr = doc["words"];
    _emojiCount = 0;
    for (JsonObject w : wordsArr) {
        if (_emojiCount >= MAX_EMOJI) break;
        const char* em = w["emoji"] | "";
        if (strlen(em) > 0) {
            strlcpy(_emojiList[_emojiCount], em, sizeof(_emojiList[0]));
            _emojiCount++;
        }
    }

    // Deduplicate emoji list
    uint16_t unique = 0;
    for (uint16_t i = 0; i < _emojiCount; i++) {
        bool dup = false;
        for (uint16_t j = 0; j < unique; j++) {
            if (strcmp(_emojiList[i], _emojiList[j]) == 0) {
                dup = true;
                break;
            }
        }
        if (!dup) {
            if (unique != i) strlcpy(_emojiList[unique], _emojiList[i], sizeof(_emojiList[0]));
            unique++;
        }
    }
    _emojiCount = unique;
    _emojiTotal = _emojiCount;
    _emojiDone = 0;

    Serial.printf("[pack] Need %u unique emoji\n", _emojiCount);

    // Step 4: Download emoji (skip existing)
    _state = PackDownloadState::FetchingEmoji;
    for (uint16_t i = 0; i < _emojiCount; i++) {
        char binPath[32];
        snprintf(binPath, sizeof(binPath), "/%s.bin", _emojiList[i]);

        // Skip if already on SPIFFS
        if (SPIFFS.exists(binPath)) {
            _emojiDone++;
            _progress = 25 + (_emojiDone * 65 / _emojiTotal);
            continue;
        }

        snprintf(_statusBuf, sizeof(_statusBuf), "Emoji %u/%u", _emojiDone + 1, _emojiTotal);
        snprintf(url, sizeof(url), "%s/packs/emoji/%s.bin", BASE_URL, _emojiList[i]);

        if (!httpDownloadToSpiffs(url, binPath)) {
            Serial.printf("[pack] Failed to download emoji %s\n", _emojiList[i]);
            // Continue despite individual failures
        }

        _emojiDone++;
        _progress = 25 + (_emojiDone * 65 / _emojiTotal);
        yield();  // Let WiFi stack breathe
    }

    // Step 5: Clean orphaned emoji files
    _state = PackDownloadState::CleaningOrphans;
    strlcpy(_statusBuf, "Cleaning up...", sizeof(_statusBuf));
    _progress = 92;

    // Build set of needed files
    fs::File root = SPIFFS.open("/");
    fs::File file = root.openNextFile();
    while (file) {
        String name = file.name();
        file.close();

        // Only consider .bin files (emoji), skip manifest.json and font.vlw
        if (name.endsWith(".bin")) {
            // Extract codepoint from filename (remove leading / and .bin)
            String cp = name;
            if (cp.startsWith("/")) cp = cp.substring(1);
            cp = cp.substring(0, cp.length() - 4);

            bool needed = false;
            for (uint16_t i = 0; i < _emojiCount; i++) {
                if (cp == _emojiList[i]) {
                    needed = true;
                    break;
                }
            }

            if (!needed) {
                Serial.printf("[pack] Removing orphan: %s\n", name.c_str());
                SPIFFS.remove(name);
            }
        }
        file = root.openNextFile();
    }

    // Step 6: Update settings
    _progress = 98;
    OsmosisSettings& s = settingsMgr.settings();
    strlcpy(s.installedLang, lang, sizeof(s.installedLang));
    strlcpy(s.installedTier, tr, sizeof(s.installedTier));
    s.installedVer = _tiers[langIdx][tierIdx].version;
    s.progressIndex = 0;  // Reset progress for new pack
    s.lastDay = 0;
    settingsMgr.save();

    _state = PackDownloadState::Complete;
    _progress = 100;
    snprintf(_statusBuf, sizeof(_statusBuf), "%s ready!", _languages[langIdx].name);
    Serial.printf("[pack] Install complete: %s %s\n", lang, tr);
    return true;
}

void update() {
    // Currently downloads are synchronous in startDownload()
    // This is a placeholder for future async download support
}

PackDownloadState state() { return _state; }
uint8_t progressPercent() { return _progress; }
const char* statusText() { return _statusBuf; }

bool hasInstalledPack() {
    return SPIFFS.exists("/manifest.json");
}

}  // namespace packMgr
```

**Step 3: Build to verify**

Run: `cd /Users/kevintomlinson/Coding/Osmosis && pio run`
Expected: BUILD SUCCESS

**Step 4: Commit**

```bash
git add src/pack_manager.h src/pack_manager.cpp
git commit -m "feat: pack manager for downloading language packs from CDN"
```

---

## Task 6: Update Card Screen for Dynamic Language and Phonetic Support

**Files:**
- Modify: `src/card_screen.cpp`
- Modify: `src/card_screen.h`

**Step 1: Update card_screen.h**

Add `reloadFont()` for when a new pack is installed:

```cpp
#pragma once
#include <TFT_eSPI.h>

namespace cardScreen {
    void init();        // Load smooth font data from SPIFFS (call after SPIFFS.begin + vocab load)
    void reloadFont();  // Reload font after pack switch
    void render();      // Full strip-based render of the flashcard screen
}
```

**Step 2: Rewrite card_screen.cpp**

Key changes:
- Load font from `/font.vlw` (generic name, per-pack) instead of `/SpanishFont26.vlw`
- Subtitle shows dynamic language name from `vocabLoader::packInfo().languageDisplay`
- Add phonetic text below English when `settingsMgr.settings().showPhonetic` is true
- Use `word.word` instead of `word.spanish`
- Use `word.emoji` codepoint for image loading

Replace the `init()` function:

```cpp
void init() {
    reloadFont();
}

void reloadFont() {
    // Free previous font
    if (fontData26) {
        free(fontData26);
        fontData26 = nullptr;
        smoothFontReady = false;
    }
    // Load the pack's font file
    fontData26 = loadVlwFromSpiffs("font");  // loads /font.vlw
    if (fontData26) {
        smoothFontReady = true;
        Serial.println("[font] Smooth font ready");
    } else {
        Serial.println("[font] WARNING: Smooth font not loaded");
    }
}
```

Update the subtitle to show the installed language dynamically:

```cpp
// "~ Spanish ~" subtitle → dynamic language name
{
    int textScreenY = 20;
    int y = textScreenY - stripY;
    if (y >= -16 && y < STRIP_H) {
        char subtitle[32];
        const char* langName = vocabLoader::isLoaded()
            ? vocabLoader::packInfo().languageDisplay : "No Pack";
        snprintf(subtitle, sizeof(subtitle), "~ %s ~", langName);
        spr.setTextDatum(TC_DATUM);
        spr.setTextColor(CLR_TEXT_SECONDARY, CLR_HEADER_BG);
        spr.drawString(subtitle, SCREEN_W / 2, y, 2);
    }
}
```

Update the Spanish word section to use `word.word`:

```cpp
// Foreign word — use smooth font
{
    int textScreenY = WORD_BOX_Y + 6;
    int y = textScreenY - stripY;
    if (y >= -28 && y < STRIP_H) {
        if (smoothFontReady) spr.loadFont(fontData26);
        spr.setTextDatum(TC_DATUM);
        spr.setTextColor(CLR_TEXT_PRIMARY, CLR_WORD_BOX_BG);
        if (smoothFontReady) {
            spr.drawString(word.word, SCREEN_W / 2, y);
        } else {
            spr.drawString(word.word, SCREEN_W / 2, y, 4);
        }
        if (spr.fontLoaded) spr.unloadFont();
    }
}
```

Add phonetic text rendering (new section between English word and counter):

```cpp
// Phonetic pronunciation (optional, below English)
if (settingsMgr.settings().showPhonetic && strlen(word.phonetic) > 0) {
    int textScreenY = WORD_BOX_Y + 46;
    int y = textScreenY - stripY;
    if (y >= -12 && y < STRIP_H) {
        spr.setTextDatum(TC_DATUM);
        spr.setTextColor(CLR_TEXT_DIM, CLR_BG_DARK);
        spr.drawString(word.phonetic, SCREEN_W / 2, y, 1);
    }
}
```

Note: When phonetic is shown, the word box height needs to grow. Update `WORD_BOX_H` to be dynamic or increase it in constants.h from 54 to 66 to accommodate the extra line.

**Step 3: Build to verify**

Run: `cd /Users/kevintomlinson/Coding/Osmosis && pio run`
Expected: BUILD SUCCESS

**Step 4: Commit**

```bash
git add src/card_screen.h src/card_screen.cpp
git commit -m "feat: dynamic language display and phonetic support on card screen"
```

---

## Task 7: Update Settings UI — Language, WiFi, and Phonetic Sections

**Files:**
- Modify: `src/ui_settings.h`
- Modify: `src/ui_settings.cpp`

**Step 1: Update ui_settings.h**

Add new screen states for the language browser and download progress:

```cpp
#pragma once
#include <TFT_eSPI.h>
#include "touch_handler.h"

enum class SettingsPage : uint8_t {
    Main,           // Original settings + new sections
    LanguageBrowser, // Language/tier selection from catalog
    DownloadProgress // Full-screen download progress bar
};

class SettingsScreen {
public:
    bool isActive() const { return _active; }
    void show();
    void hide();
    void draw(TFT_eSprite& spr, int stripY);
    bool handleTap(TouchPoint pt);
    SettingsPage currentPage() const { return _page; }

private:
    bool _active = false;
    SettingsPage _page = SettingsPage::Main;
    int8_t _selectedLang = -1;  // Selected language index in browser
    int8_t _scrollOffset = 0;

    struct Button { int x, y, w, h; };

    bool hitTest(const Button& btn, TouchPoint pt);
    void drawButton(TFT_eSprite& spr, const Button& btn, int stripY,
                    const char* label, bool selected);

    void drawMainPage(TFT_eSprite& spr, int stripY);
    void drawLanguageBrowser(TFT_eSprite& spr, int stripY);
    void drawDownloadProgress(TFT_eSprite& spr, int stripY);

    bool handleMainTap(TouchPoint pt);
    bool handleBrowserTap(TouchPoint pt);
};

extern SettingsScreen settingsUI;
```

**Step 2: Rewrite ui_settings.cpp**

The main settings page adds three new rows below brightness:
- **Phonetic** toggle: ON / OFF
- **Language** button: shows current language, taps opens language browser
- **WiFi** button: shows connection status, taps starts captive portal

The language browser page shows a scrollable list of languages from the catalog, then tiers once a language is selected.

The download progress page shows a full-screen progress bar during pack install.

This is a large rewrite. The core structure of `drawMainPage()` preserves the existing 3 rows (words/day, display time, brightness) and adds the new sections below. The language browser and download progress are separate draw functions.

Key layout additions to main page:

```
Y=278: Phonetic row — "Phonetic" label + ON/OFF buttons
Y=320: (overflow — need to add scrolling or reduce spacing)
```

Since the screen is 320px, the main settings page is getting crowded. Reduce spacing between existing rows and add the new ones. Alternatively, make settings two-page scrollable. For simplicity, tighten the layout:

- Title: y=6
- Words/day: label y=30, buttons y=44, h=36
- Display time: label y=86, buttons y=100, h=36
- Brightness: label y=142, buttons y=156, h=36
- Phonetic: label y=198, buttons y=212, h=36
- Language: button y=256, h=30
- WiFi: button y=290, h=24 (status only, taps to portal)

**Step 3: Build to verify**

Run: `cd /Users/kevintomlinson/Coding/Osmosis && pio run`
Expected: BUILD SUCCESS

**Step 4: Commit**

```bash
git add src/ui_settings.h src/ui_settings.cpp
git commit -m "feat: settings UI with language browser, phonetic toggle, WiFi status"
```

---

## Task 8: Update main.cpp — Boot Flow with Pack Detection

**Files:**
- Modify: `src/main.cpp`

**Step 1: Rewrite main.cpp for v2.0 boot sequence**

The new boot flow:
1. Init display, show splash
2. Init SPIFFS
3. Init touch
4. Load settings
5. Init WiFi (try to connect with saved creds)
6. Check if a pack is installed (manifest.json exists)
   - **Yes:** Load vocab from manifest, init card screen, enter card mode
   - **No:** Show "No Pack" screen, prompt WiFi setup, enter language browser
7. Main loop handles WiFi update, pack manager update, and normal card/settings rendering

```cpp
#include <Arduino.h>
#include <SPIFFS.h>
#include "constants.h"
#include "display_manager.h"
#include "splash_screen.h"
#include "touch_handler.h"
#include "settings_manager.h"
#include "card_manager.h"
#include "card_screen.h"
#include "ui_settings.h"
#include "image_renderer.h"
#include "wifi_manager.h"
#include "pack_manager.h"
#include "vocab_loader.h"

static uint32_t lastTouchPoll = 0;
static uint32_t lastRender = 0;
static bool needsRender = true;
static bool packInstalled = false;

enum class AppState : uint8_t {
    Cards,          // Normal flashcard display
    Settings,       // Settings menu
    NoPack,         // No pack installed — shows setup prompt
    Downloading     // Pack download in progress
};

static AppState appState = AppState::NoPack;

void setup() {
    Serial.begin(115200);
    Serial.println("Osmosis v2.0.0 booting...");

    display.init();
    delay(100);

    if (!SPIFFS.begin(true)) {
        Serial.println("SPIFFS mount failed!");
    }

    splash::show();
    touch.init();
    settingsMgr.init();
    display.setBrightnessLevel(settingsMgr.settings().brightness);

    // Init WiFi (non-blocking connect attempt)
    wifiMgr::init();

    // Check for installed pack
    if (packMgr::hasInstalledPack() && vocabLoader::load()) {
        packInstalled = true;
        cardMgr.init();
        cardScreen::init();
        appState = AppState::Cards;
        Serial.println("Pack loaded, entering card mode");
    } else {
        packInstalled = false;
        appState = AppState::NoPack;
        Serial.println("No pack installed, entering setup mode");
    }

    needsRender = true;
    Serial.println("Osmosis ready!");
}
```

The loop() function needs to handle the new app states and WiFi/pack manager updates. This is a significant rewrite of the main loop to route gestures and rendering through the app state machine.

**Step 2: Build to verify**

Run: `cd /Users/kevintomlinson/Coding/Osmosis && pio run`
Expected: BUILD SUCCESS

**Step 3: Commit**

```bash
git add src/main.cpp
git commit -m "feat: v2.0 boot flow with pack detection and WiFi init"
```

---

## Task 9: Update Splash Screen for v2.0

**Files:**
- Modify: `src/splash_screen.cpp`

**Step 1: Update splash text**

Change version string from `"v1.0 - Spanish"` to `"v2.0"` and remove hardcoded language:

```cpp
// Was: tft.drawString("v1.0 - Spanish", SCREEN_W / 2, 260, 2);
tft.drawString("v2.0", SCREEN_W / 2, 260, 2);
```

**Step 2: Build and commit**

```bash
git add src/splash_screen.cpp
git commit -m "feat: update splash screen for v2.0"
```

---

## Task 10: Content Pipeline — Emoji Renderer at 96x96

**Files:**
- Modify: `tools/render_apple_emoji.py`
- Modify: `tools/convert_emoji.py`

**Step 1: Update render_apple_emoji.py**

Key changes:
- Render at 96x96 instead of 72x72
- Accept a vocab CSV file as input instead of parsing word_data.h
- Output files named by codepoint (e.g. `1f4a7.png`) instead of word
- Can also accept a list of codepoints directly

Change `IMG_SIZE = 72` to `IMG_SIZE = 96` and `FONT_SIZE = 64` to `FONT_SIZE = 86`.

Add a new mode that reads codepoints from a text file (one per line) or from all vocab CSVs:

```python
def parse_vocab_csvs(vocab_dir):
    """Parse all vocab CSV files to extract unique emoji codepoints."""
    codepoints = set()
    for csv_file in sorted(vocab_dir.glob('*.csv')):
        with open(csv_file) as f:
            reader = csv.DictReader(f)
            for row in reader:
                cp = row.get('emoji', '').strip()
                if cp:
                    codepoints.add(cp)
    return sorted(codepoints)
```

Output filenames use codepoint: `{codepoint}.png` (e.g. `1f4a7.png`).

**Step 2: Update convert_emoji.py**

Change `IMG_SIZE = 72` to `IMG_SIZE = 96`. Update SPIFFS budget comment from 1500KB to 2500KB.

**Step 3: Test the pipeline locally**

Run: `cd /Users/kevintomlinson/Coding/Osmosis && python3 tools/render_apple_emoji.py tools/emoji_apple_png`
Expected: 96x96 PNG files generated in `tools/emoji_apple_png/`

Run: `python3 tools/convert_emoji.py tools/emoji_apple_png data`
Expected: 96x96 ORLE .bin files generated in `data/`

**Step 4: Commit**

```bash
git add tools/render_apple_emoji.py tools/convert_emoji.py
git commit -m "feat: update emoji pipeline for 96x96 codepoint-based naming"
```

---

## Task 11: Content Pipeline — Multi-Language Font Generator

**Files:**
- Modify: `tools/generate_vlw_font.py`

**Step 1: Add per-language character sets**

Replace the hardcoded `SPANISH_CHARS` with a language-configurable system:

```python
LANGUAGE_CHARS = {
    'spanish': list("áéíóúñüÁÉÍÓÚÑÜ¿¡"),
    'french': list("àâæçéèêëîïôœùûüÿÀÂÆÇÉÈÊËÎÏÔŒÙÛÜŸ"),
    'portuguese': list("àáâãçéêíóôõúÀÁÂÃÇÉÊÍÓÔÕÚ"),
    'dutch': list("éëïüÉËÏÜ"),
    'chinese': list("āáǎàēéěèīíǐìōóǒòūúǔùǖǘǚǜĀÁǍÀĒÉĚÈĪÍǏÌŌÓǑÒŪÚǓÙǕǗǙǛ"),
}
```

Accept a `--language` argument and generate `font.vlw` per language.

**Step 2: Test font generation**

Run: `python3 tools/generate_vlw_font.py --language spanish --size 26 --output data/font.vlw`
Expected: VLW file with ASCII + Spanish chars

**Step 3: Commit**

```bash
git add tools/generate_vlw_font.py
git commit -m "feat: multi-language VLW font generator"
```

---

## Task 12: Content Pipeline — Vocab CSV Format and Manifest Builder

**Files:**
- Create: `tools/vocab/spanish_beginner.csv` (sample, first 20 words)
- Create: `tools/build_pack.py`

**Step 1: Create sample vocab CSV**

Create `tools/vocab/spanish_beginner.csv` with the first 20 words migrated from word_data.h:

```csv
emoji,english,translation,phonetic,category
1f4a7,water,agua,AH-gwah,nature
1f3e0,house,casa,KAH-sah,home
1f468,man,hombre,OHM-breh,people
1f469,woman,mujer,moo-HEHR,people
1f466,boy,niño,NEEN-yoh,people
270b,hand,mano,MAH-noh,body
1f441,eye,ojo,OH-hoh,body
2600,sun,sol,sohl,weather
1f415,dog,perro,PEH-rroh,animals
1f408,cat,gato,GAH-toh,animals
1f4d6,book,libro,LEE-broh,objects
1f30d,earth,tierra,tee-EH-rrah,nature
1f525,fire,fuego,FWEH-goh,nature
2764,heart,corazón,koh-rah-SOHN,body
1f338,flower,flor,flohr,nature
1f467,girl,niña,NEEN-yah,people
1f319,moon,luna,LOO-nah,weather
2b50,star,estrella,ehs-TREH-yah,weather
1f35e,bread,pan,pahn,food
1f95b,milk,leche,LEH-cheh,food
```

**Step 2: Create build_pack.py**

This script reads a vocab CSV, generates manifest.json, generates the font.vlw, and optionally renders/converts emoji:

```python
#!/usr/bin/env python3
"""Build a language pack from a vocab CSV file.

Usage:
    python3 tools/build_pack.py --csv tools/vocab/spanish_beginner.csv --output packs/spanish/beginner/
"""

import argparse
import csv
import json
from pathlib import Path

def build_manifest(csv_path, output_dir, language, tier):
    """Read vocab CSV and generate manifest.json."""
    words = []
    with open(csv_path) as f:
        reader = csv.DictReader(f)
        for row in reader:
            words.append({
                "word": row["translation"],
                "english": row["english"],
                "phonetic": row["phonetic"],
                "emoji": row["emoji"],
                "category": row["category"],
            })

    # Language display names
    LANG_DISPLAY = {
        "spanish": "Spanish", "french": "French",
        "portuguese": "Portuguese", "chinese": "Chinese",
        "dutch": "Dutch"
    }
    TIER_DISPLAY = {
        "beginner": "Beginner", "intermediate": "Intermediate",
        "advanced": "Advanced"
    }

    manifest = {
        "language": language,
        "languageDisplay": LANG_DISPLAY.get(language, language.title()),
        "tier": tier,
        "tierDisplay": TIER_DISPLAY.get(tier, tier.title()),
        "version": 1,
        "wordCount": len(words),
        "fontFile": "font.vlw",
        "words": words,
    }

    output_dir = Path(output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)

    manifest_path = output_dir / "manifest.json"
    with open(manifest_path, 'w') as f:
        json.dump(manifest, f, indent=2, ensure_ascii=False)

    print(f"Written {manifest_path} ({len(words)} words)")
    return manifest

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--csv", required=True)
    parser.add_argument("--output", required=True)
    parser.add_argument("--language", required=True)
    parser.add_argument("--tier", required=True)
    args = parser.parse_args()
    build_manifest(args.csv, args.output, args.language, args.tier)
```

**Step 3: Test the manifest builder**

Run: `python3 tools/build_pack.py --csv tools/vocab/spanish_beginner.csv --output packs/spanish/beginner/ --language spanish --tier beginner`
Expected: `packs/spanish/beginner/manifest.json` created with 20 words

**Step 4: Commit**

```bash
git add tools/vocab/spanish_beginner.csv tools/build_pack.py
git commit -m "feat: vocab CSV format and manifest builder for content pipeline"
```

---

## Task 13: No-Pack and Download UI Screens

**Files:**
- Create: `src/no_pack_screen.h`
- Create: `src/no_pack_screen.cpp`
- Create: `src/download_screen.h`
- Create: `src/download_screen.cpp`

**Step 1: Create no_pack_screen.h**

```cpp
#pragma once
#include <TFT_eSPI.h>

namespace noPackScreen {
    void render();  // Full strip-based render: "No Pack Installed" + WiFi setup instructions
}
```

**Step 2: Create no_pack_screen.cpp**

Renders a simple screen with:
- "OSMOSIS" header (same as card screen)
- "No Language Pack" centered message
- "Connect to WiFi to download" instruction
- "Long press for settings" hint

Uses the same strip-based rendering pattern as card_screen.cpp.

**Step 3: Create download_screen.h/cpp**

Full-screen download progress bar. Reads from `packMgr::progressPercent()` and `packMgr::statusText()`. Renders:
- "Downloading..." title
- Large progress bar (200px wide, 20px tall, centered)
- Percentage text
- Status message

**Step 4: Build and commit**

```bash
git add src/no_pack_screen.h src/no_pack_screen.cpp src/download_screen.h src/download_screen.cpp
git commit -m "feat: no-pack and download progress UI screens"
```

---

## Task 14: Integration — Wire Everything Together in main.cpp

**Files:**
- Modify: `src/main.cpp`

**Step 1: Complete the main loop state machine**

The loop() must handle all app states:

- **Cards:** Normal card rotation + rendering. Tap = next card. Long press = settings.
- **Settings:** Draw settings. Tap = handle. Long press = close. Language browser tap = start catalog fetch + download flow.
- **NoPack:** Draw no-pack screen. Long press = settings (to configure WiFi and browse languages).
- **Downloading:** Draw download progress. Block touch. When complete, reload vocab + font, switch to Cards.

The state transitions:
```
NoPack → (long press) → Settings → Language Browser → Downloading → Cards
Cards → (long press) → Settings → ...
```

**Step 2: Build and upload firmware**

Run: `cd /Users/kevintomlinson/Coding/Osmosis && pio run`
Expected: BUILD SUCCESS

**Step 3: Commit**

```bash
git add src/main.cpp
git commit -m "feat: complete v2.0 main loop state machine"
```

---

## Task 15: Build Static CDN Content Structure

**Files:**
- Create: `site/index.html` (landing page placeholder)
- Create: `site/api/osmosis/catalog.json`
- Organize pack files under `site/api/osmosis/packs/`

**Step 1: Create catalog.json**

Create `site/api/osmosis/catalog.json` with the initial Spanish beginner pack:

```json
{
  "apiVersion": 1,
  "baseUrl": "https://vcodeworks.dev/api/osmosis/packs",
  "emojiVersion": 1,
  "emojiCount": 20,
  "languages": [
    {
      "id": "spanish",
      "name": "Spanish",
      "flag": "ES",
      "tiers": [
        {
          "id": "beginner",
          "name": "Beginner",
          "words": 20,
          "version": 1,
          "manifestSize": 2800,
          "fontFile": "font.vlw",
          "fontSize": 30000
        }
      ]
    }
  ]
}
```

**Step 2: Copy pack files to site structure**

```
site/api/osmosis/
  catalog.json
  packs/
    emoji/
      1f4a7.bin, 1f3e0.bin, ... (codepoint-named)
    spanish/
      beginner/
        manifest.json
        font.vlw
```

**Step 3: Commit**

```bash
git add site/
git commit -m "feat: static CDN structure with catalog and Spanish beginner pack"
```

---

## Task 16: Full Spanish Pack — Migrate All 300 Words

**Files:**
- Modify: `tools/vocab/spanish_beginner.csv` (expand to 300 words or split into tiers)
- Create: `tools/vocab/spanish_intermediate.csv`
- Create: `tools/vocab/spanish_advanced.csv`
- Create: `tools/migrate_word_data.py`

**Step 1: Create migration script**

Write `tools/migrate_word_data.py` to parse the existing `word_data.h` and output a CSV file with all 300 words including codepoint-based emoji references and phonetic placeholders:

```python
#!/usr/bin/env python3
"""Migrate word_data.h entries to vocab CSV format."""
# Parse the WORD_LIST PROGMEM array, extract:
# - emoji codepoint from comments (U+XXXX)
# - spanish word, english translation
# - category
# Output as CSV with phonetic placeholder
```

**Step 2: Split into 3 tiers of ~100 words each**

Divide the 300 words into beginner (1-100), intermediate (101-200), advanced (201-300) based on frequency ranking.

**Step 3: Generate all 3 Spanish packs**

Run the build_pack.py for each tier.

**Step 4: Commit**

```bash
git add tools/vocab/ tools/migrate_word_data.py
git commit -m "feat: migrate 300 Spanish words to CSV and split into 3 tiers"
```

---

## Task 17: Generate Remaining Language Packs (French, Portuguese, Chinese, Dutch)

**Files:**
- Create: `tools/vocab/french_beginner.csv` (+ intermediate, advanced)
- Create: `tools/vocab/portuguese_beginner.csv` (+ intermediate, advanced)
- Create: `tools/vocab/chinese_beginner.csv` (+ intermediate, advanced)
- Create: `tools/vocab/dutch_beginner.csv` (+ intermediate, advanced)

**Step 1: Create vocab CSVs for each language**

Each CSV follows the same format: `emoji,english,translation,phonetic,category`

For Chinese, the `translation` column contains pinyin (e.g. "shuǐ") and `phonetic` contains tone numbers (e.g. "shui3").

**Step 2: Generate fonts for each language**

Run `generate_vlw_font.py` with each language's character set.

**Step 3: Build all 15 packs**

Create a `tools/build_all_packs.sh` script that runs build_pack.py for all 15 language/tier combinations.

**Step 4: Update catalog.json with all languages**

**Step 5: Commit**

```bash
git add tools/vocab/ site/
git commit -m "feat: add French, Portuguese, Chinese, Dutch language packs"
```

---

## Task 18: Update Image Renderer for 96x96 Source

**Files:**
- Modify: `src/image_renderer.cpp`

**Step 1: Update dimension check**

In `preloadImage()`, the dimension check on line 108-113 validates `width == IMG_W && height == IMG_H`. Since `IMG_W`/`IMG_H` are now 96 in constants.h (from Task 2), the code already handles this correctly. However, the image buffer allocation also uses `IMG_W * IMG_H`:

```cpp
imageBuffer = (uint16_t*)malloc(IMG_W * IMG_H * sizeof(uint16_t));
```

At 96x96, this is `96*96*2 = 18,432 bytes` (was 10,368 for 72x72). Verify heap has room — ESP32 has ~300KB heap, so this is fine.

**Step 2: Build and verify**

Run: `cd /Users/kevintomlinson/Coding/Osmosis && pio run`
Expected: BUILD SUCCESS. The renderer already uses `IMG_W`/`IMG_H` from constants.h everywhere, so the 96px change propagates automatically.

**Step 3: Commit (if any changes needed)**

```bash
git add src/image_renderer.cpp
git commit -m "fix: verify image renderer works with 96x96 source images"
```

---

## Task 19: End-to-End Testing with Spanish Beginner Pack

**Step 1: Generate the test pack locally**

```bash
cd /Users/kevintomlinson/Coding/Osmosis
# Generate font
python3 tools/generate_vlw_font.py --language spanish --size 26 --output data/font.vlw
# Build manifest from CSV
python3 tools/build_pack.py --csv tools/vocab/spanish_beginner.csv --output data/ --language spanish --tier beginner
# Rename manifest output to data/manifest.json for SPIFFS
```

The `data/` directory should now contain: `manifest.json`, `font.vlw`, and codepoint-named `.bin` emoji files.

**Step 2: Upload SPIFFS and firmware**

```bash
pio run --target uploadfs
pio run --target upload
```

**Step 3: Verify on device**

- Splash screen shows "v2.0"
- Device loads manifest.json and displays flashcards
- Subtitle shows "~ Spanish ~" (from manifest)
- Foreign word renders with smooth font (accented chars work)
- English translation shows below
- Card counter works
- Long press opens settings with new sections

**Step 4: Commit any fixes**

```bash
git add -A
git commit -m "fix: end-to-end testing fixes for v2.0"
```

---

## Task 20: Deploy to VCodeWorks.dev

**Step 1: Set up Cloudflare Pages**

- Connect the `site/` directory to Cloudflare Pages
- Configure custom domain: vcodeworks.dev
- Verify catalog.json is accessible at `https://vcodeworks.dev/api/osmosis/catalog.json`

**Step 2: Test WiFi download flow on device**

- Long press → Settings → Language → Browse
- Device fetches catalog.json from CDN
- Select Spanish → Beginner
- Download progress shows
- Pack installs, device shows flashcards

**Step 3: Commit deployment config if needed**

```bash
git add -A
git commit -m "feat: deploy language packs to VCodeWorks.dev CDN"
```

---

## Summary

| Task | Component | Key Files |
|------|-----------|-----------|
| 1 | OTA Partition Table | `partitions_ota.csv`, `platformio.ini` |
| 2 | Constants + Settings | `constants.h`, `settings_manager.h/.cpp` |
| 3 | Vocab Loader | `vocab_loader.h/.cpp`, `word_data.h`, `card_manager.h/.cpp` |
| 4 | WiFi Manager | `wifi_manager.h/.cpp` |
| 5 | Pack Manager | `pack_manager.h/.cpp` |
| 6 | Card Screen v2 | `card_screen.h/.cpp` |
| 7 | Settings UI v2 | `ui_settings.h/.cpp` |
| 8 | Main Boot Flow | `main.cpp` |
| 9 | Splash Screen v2 | `splash_screen.cpp` |
| 10 | Emoji Pipeline 96x96 | `tools/render_apple_emoji.py`, `tools/convert_emoji.py` |
| 11 | Font Generator | `tools/generate_vlw_font.py` |
| 12 | Manifest Builder | `tools/build_pack.py`, `tools/vocab/*.csv` |
| 13 | No-Pack + Download UI | `no_pack_screen.h/.cpp`, `download_screen.h/.cpp` |
| 14 | Main Loop Integration | `main.cpp` |
| 15 | CDN Structure | `site/` |
| 16 | Full Spanish Pack | `tools/vocab/spanish_*.csv` |
| 17 | Other Language Packs | `tools/vocab/*.csv` |
| 18 | Image Renderer 96x96 | `image_renderer.cpp` |
| 19 | E2E Testing | All files |
| 20 | CDN Deployment | `site/` |
