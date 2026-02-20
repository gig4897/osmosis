#include "settings_manager.h"
#include "constants.h"
#include <cstring>

SettingsManager settingsMgr;

void SettingsManager::setDefaults() {
    memset(&_settings, 0, sizeof(_settings));
    _settings.wordsPerDay   = DEFAULT_WORDS_PER_DAY;
    _settings.displaySecs   = DEFAULT_DISPLAY_SECS;
    _settings.brightness    = DEFAULT_BRIGHTNESS;
    _settings.progressIndex = 0;
    _settings.lastDay       = 0;
}

void SettingsManager::init() {
    _prefs.begin("osmosis", false);
    load();
}

void SettingsManager::load() {
    setDefaults();

    _settings.wordsPerDay   = _prefs.getUChar("wpd",      DEFAULT_WORDS_PER_DAY);
    _settings.displaySecs   = _prefs.getUShort("dsecs",   DEFAULT_DISPLAY_SECS);
    _settings.brightness    = _prefs.getUChar("bright",    DEFAULT_BRIGHTNESS);
    _settings.progressIndex = _prefs.getUShort("progress", 0);
    _settings.lastDay       = _prefs.getUChar("lastday",   0);
}

void SettingsManager::save() {
    _prefs.putUChar("wpd",       _settings.wordsPerDay);
    _prefs.putUShort("dsecs",    _settings.displaySecs);
    _prefs.putUChar("bright",    _settings.brightness);
    _prefs.putUShort("progress", _settings.progressIndex);
    _prefs.putUChar("lastday",   _settings.lastDay);
}

void SettingsManager::saveProgress() {
    _prefs.putUShort("progress", _settings.progressIndex);
    _prefs.putUChar("lastday",   _settings.lastDay);
}
