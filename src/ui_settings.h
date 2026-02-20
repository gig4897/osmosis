#pragma once
#include <TFT_eSPI.h>
#include "touch_handler.h"

class SettingsScreen {
public:
    bool isActive() const { return _active; }
    void show();
    void hide();
    void draw(TFT_eSprite& spr, int stripY);
    bool handleTap(TouchPoint pt);  // Returns true if settings changed

private:
    bool _active = false;

    struct Button { int x, y, w, h; };

    bool hitTest(const Button& btn, TouchPoint pt);
    void drawButton(TFT_eSprite& spr, const Button& btn, int stripY,
                    const char* label, bool selected);
};

extern SettingsScreen settingsUI;
