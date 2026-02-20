#pragma once
#include <Preferences.h>
#include <cstdint>

struct OsmosisSettings {
    uint8_t wordsPerDay;     // 5, 10, 15, 20, 25
    uint16_t displaySecs;    // 30, 60, 120, 300
    uint8_t brightness;      // 0=Low, 1=Med, 2=High
    uint16_t progressIndex;  // Next word index to start from (0-based)
    uint8_t lastDay;         // Day of month when batch was last advanced
};

class SettingsManager {
public:
    void init();
    void load();
    void save();
    void saveProgress();  // Save only progressIndex and lastDay (frequent writes)

    OsmosisSettings& settings() { return _settings; }
    const OsmosisSettings& settings() const { return _settings; }

private:
    Preferences _prefs;
    OsmosisSettings _settings;
    void setDefaults();
};

extern SettingsManager settingsMgr;
