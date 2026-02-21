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

    // Parse JSON
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
