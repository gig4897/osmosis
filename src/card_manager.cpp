#include "card_manager.h"
#include "settings_manager.h"
#include "image_renderer.h"
#include <Arduino.h>
#include <ctime>

CardManager cardMgr;

void CardManager::init() {
    buildBatch();
    loadCurrentImage();
    _lastCardChangeMs = millis();
}

void CardManager::buildBatch() {
    _batchSize = settingsMgr.settings().wordsPerDay;
    uint16_t startIdx = settingsMgr.settings().progressIndex;

    for (uint8_t i = 0; i < _batchSize; i++) {
        _dailyBatch[i] = (startIdx + i) % WORD_COUNT;
    }

    shuffleBatch();
    _currentPos = 0;
}

void CardManager::shuffleBatch() {
    // Fisher-Yates shuffle
    for (int i = _batchSize - 1; i > 0; i--) {
        int j = random(0, i + 1);
        uint16_t tmp = _dailyBatch[i];
        _dailyBatch[i] = _dailyBatch[j];
        _dailyBatch[j] = tmp;
    }
}

bool CardManager::update() {
    uint32_t now = millis();
    uint32_t intervalMs = (uint32_t)settingsMgr.settings().displaySecs * 1000UL;

    if (now - _lastCardChangeMs >= intervalMs) {
        _currentPos++;
        if (_currentPos >= _batchSize) {
            _currentPos = 0;
        }
        loadCurrentImage();
        _lastCardChangeMs = now;
        return true;
    }
    return false;
}

void CardManager::checkDayChange() {
    time_t now = time(nullptr);

    // If time is not set (returns 0 or very small value), don't advance
    if (now < 86400) return;

    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    uint8_t today = timeinfo.tm_mday;

    if (today != settingsMgr.settings().lastDay) {
        // Day has changed -- advance progress
        uint16_t newProgress = (settingsMgr.settings().progressIndex +
                                settingsMgr.settings().wordsPerDay) % WORD_COUNT;
        settingsMgr.settings().progressIndex = newProgress;
        settingsMgr.settings().lastDay = today;
        settingsMgr.saveProgress();

        buildBatch();
        loadCurrentImage();
        _lastCardChangeMs = millis();

        Serial.printf("[card] Day changed to %d, progress now at %d\n",
                      today, newProgress);
    }
}

const WordEntry& CardManager::currentWord() const {
    return WORD_LIST[_dailyBatch[_currentPos]];
}

int CardManager::currentCardIndex() const {
    return _currentPos + 1;
}

int CardManager::totalCardsToday() const {
    return _batchSize;
}

void CardManager::loadCurrentImage() {
    const WordEntry& word = currentWord();
    imageRenderer::preloadImage(word.imageFile);
    Serial.printf("[card] Card %d/%d: %s (%s)\n",
                  currentCardIndex(), totalCardsToday(),
                  word.spanish, word.english);
}
