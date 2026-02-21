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
