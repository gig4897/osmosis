#include "pack_manager.h"
#include "settings_manager.h"
#include "wifi_manager.h"
#include <Arduino.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <FS.h>
#include <SPIFFS.h>
#include <cstring>

static const char* BASE_URL = "https://www.vcodeworks.dev/api/osmosis";

static PackDownloadState _state = PackDownloadState::Idle;
static uint8_t _progress = 0;
static char _statusBuf[48] = "";

// Catalog data
static const uint8_t MAX_LANGUAGES = 16;
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
static packMgr::ProgressCallback _progressCb = nullptr;

// List of emoji codepoints from the downloaded manifest
static const uint16_t MAX_EMOJI = 350;
static char _emojiList[MAX_EMOJI][12];
static uint16_t _emojiCount = 0;

static bool httpDownloadToSpiffs(const char* url, const char* path) {
    HTTPClient http;
    http.begin(url);
    http.setTimeout(15000);
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    int code = http.GET();

    if (code != 200) {
        Serial.printf("[pack] HTTP %d for %s\n", code, url);
        http.end();
        return false;
    }

    int contentLen = http.getSize();  // -1 if chunked
    Serial.printf("[pack] Downloading %s (%d bytes, chunked=%s)\n",
                  path, contentLen, (contentLen < 0) ? "yes" : "no");

    fs::File f = SPIFFS.open(path, "w");
    if (!f) {
        Serial.printf("[pack] Failed to open %s for writing\n", path);
        http.end();
        return false;
    }

    // Use writeToStream which properly handles chunked transfer encoding
    Serial.printf("[pack] Free heap before write: %u\n", (uint32_t)ESP.getFreeHeap());
    int written = http.writeToStream(&f);
    f.close();
    http.end();

    if (written < 0) {
        Serial.printf("[pack] writeToStream failed: %d for %s (heap: %u)\n",
                      written, path, (uint32_t)ESP.getFreeHeap());
        SPIFFS.remove(path);
        return false;
    }

    Serial.printf("[pack] Downloaded %s (%d bytes)\n", path, written);
    return true;
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
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
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

    Serial.printf("[pack] SPIFFS: %u used / %u total, free heap: %u\n",
                  (uint32_t)SPIFFS.usedBytes(), (uint32_t)SPIFFS.totalBytes(),
                  (uint32_t)ESP.getFreeHeap());

    // Step 0: Wipe ALL old pack files to free SPIFFS space
    // (SPIFFS becomes unreliable above ~75% usage due to GC issues)
    {
        strlcpy(_statusBuf, "Preparing storage...", sizeof(_statusBuf));
        if (_progressCb) _progressCb();

        // Delete files in small batches to avoid stack overflow
        int totalDeleted = 0;
        bool moreFiles = true;
        while (moreFiles) {
            moreFiles = false;
            fs::File root = SPIFFS.open("/");
            fs::File file = root.openNextFile();
            // Collect a small batch
            char batch[10][32];
            int batchCount = 0;
            while (file && batchCount < 10) {
                const char* name = file.name();
                if (name[0] == '/') {
                    strlcpy(batch[batchCount], name, 32);
                } else {
                    batch[batchCount][0] = '/';
                    strlcpy(batch[batchCount] + 1, name, 31);
                }
                batchCount++;
                file.close();
                file = root.openNextFile();
            }
            if (file) { file.close(); moreFiles = true; }
            // Delete this batch
            for (int i = 0; i < batchCount; i++) {
                SPIFFS.remove(batch[i]);
                totalDeleted++;
            }
            if (batchCount > 0) moreFiles = true;  // Check for more
            if (batchCount == 0) moreFiles = false;
            yield();
        }
        Serial.printf("[pack] Cleared %d files, SPIFFS now: %u used / %u total\n",
                      totalDeleted, (uint32_t)SPIFFS.usedBytes(), (uint32_t)SPIFFS.totalBytes());
    }
    _progress = 3;
    if (_progressCb) _progressCb();

    char url[128];
    snprintf(url, sizeof(url), "%s/packs/%s/%s/manifest.json", BASE_URL, lang, tr);
    if (!httpDownloadToSpiffs(url, "/manifest.json")) {
        _state = PackDownloadState::Error;
        strlcpy(_statusBuf, "Manifest download failed", sizeof(_statusBuf));
        return false;
    }
    _progress = 15;
    if (_progressCb) _progressCb();
    yield();
    delay(100);

    // Step 2: Download font.vlw
    _state = PackDownloadState::FetchingFont;
    strlcpy(_statusBuf, "Downloading font...", sizeof(_statusBuf));

    snprintf(url, sizeof(url), "%s/packs/%s/font.vlw", BASE_URL, lang);
    if (!httpDownloadToSpiffs(url, "/font.vlw")) {
        _state = PackDownloadState::Error;
        strlcpy(_statusBuf, "Font download failed", sizeof(_statusBuf));
        return false;
    }
    _progress = 25;
    if (_progressCb) _progressCb();

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
        if (_progressCb) _progressCb();
        yield();  // Let WiFi stack breathe
    }

    // Step 5: No orphan cleanup needed — we wiped all files at start

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
    // Placeholder for future async download support
}

PackDownloadState state() { return _state; }
uint8_t progressPercent() { return _progress; }
const char* statusText() { return _statusBuf; }

void resetState() {
    _state = PackDownloadState::Idle;
    _progress = 0;
    _statusBuf[0] = '\0';
}

void setProgressCallback(ProgressCallback cb) { _progressCb = cb; }

bool hasInstalledPack() {
    return SPIFFS.exists("/manifest.json");
}

}  // namespace packMgr
