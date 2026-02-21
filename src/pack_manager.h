#pragma once
#include <cstdint>

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
