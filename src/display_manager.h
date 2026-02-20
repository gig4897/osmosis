#pragma once
#include <TFT_eSPI.h>
#include "constants.h"

class DisplayManager {
public:
    void init();
    void setBrightness(uint8_t level);       // 0-255 raw
    void setBrightnessLevel(uint8_t idx);    // 0=Low, 1=Med, 2=High
    uint8_t getBrightnessLevel() const;

    TFT_eSprite& getStrip();
    void pushStrip(int y);

    TFT_eSPI& tft() { return _tft; }

    static uint16_t blendColor(uint16_t c1, uint16_t c2, float ratio);
    static uint16_t dimColor(uint16_t color, float factor);

private:
    TFT_eSPI _tft;
    TFT_eSprite _strip = TFT_eSprite(&_tft);
    uint8_t _brightnessIdx = DEFAULT_BRIGHTNESS;
};

extern DisplayManager display;
