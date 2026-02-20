#pragma once
#include "word_data.h"
#include <cstdint>

class CardManager {
public:
    void init();
    bool update();              // Returns true if card changed
    void checkDayChange();

    const WordEntry& currentWord() const;
    int currentCardIndex() const;   // 1-based
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
