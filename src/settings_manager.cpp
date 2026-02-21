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
    _settings.showPhonetic  = false;
    _settings.wifiConfigured = false;
    memset(_settings.wifiSSID, 0, sizeof(_settings.wifiSSID));
    memset(_settings.wifiPass, 0, sizeof(_settings.wifiPass));
    memset(_settings.installedLang, 0, sizeof(_settings.installedLang));
    memset(_settings.installedTier, 0, sizeof(_settings.installedTier));
    _settings.installedVer  = 0;
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
    _settings.showPhonetic  = _prefs.getBool("phonetic", false);
    _settings.wifiConfigured = _prefs.getBool("wificfg", false);
    _prefs.getString("ssid", _settings.wifiSSID, sizeof(_settings.wifiSSID));
    _prefs.getString("wpass", _settings.wifiPass, sizeof(_settings.wifiPass));
    _prefs.getString("lang", _settings.installedLang, sizeof(_settings.installedLang));
    _prefs.getString("tier", _settings.installedTier, sizeof(_settings.installedTier));
    _settings.installedVer  = _prefs.getUChar("packver", 0);
}

void SettingsManager::save() {
    _prefs.putUChar("wpd",       _settings.wordsPerDay);
    _prefs.putUShort("dsecs",    _settings.displaySecs);
    _prefs.putUChar("bright",    _settings.brightness);
    _prefs.putUShort("progress", _settings.progressIndex);
    _prefs.putUChar("lastday",   _settings.lastDay);
    _prefs.putBool("phonetic",   _settings.showPhonetic);
    _prefs.putBool("wificfg",    _settings.wifiConfigured);
    _prefs.putString("ssid",     _settings.wifiSSID);
    _prefs.putString("wpass",    _settings.wifiPass);
    _prefs.putString("lang",     _settings.installedLang);
    _prefs.putString("tier",     _settings.installedTier);
    _prefs.putUChar("packver",   _settings.installedVer);
}

void SettingsManager::saveProgress() {
    _prefs.putUShort("progress", _settings.progressIndex);
    _prefs.putUChar("lastday",   _settings.lastDay);
}
