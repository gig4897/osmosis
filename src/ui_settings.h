#pragma once
#include <TFT_eSPI.h>
#include "touch_handler.h"

enum class SettingsPage : uint8_t {
    Main,            // Original settings + new sections
    LanguageBrowser,  // Language/tier selection from catalog
    DownloadProgress  // Full-screen download progress bar
};

class SettingsScreen {
public:
    bool isActive() const { return _active; }
    void show();
    void hide();
    void draw(TFT_eSprite& spr, int stripY);
    bool handleTap(TouchPoint pt);
    SettingsPage currentPage() const { return _page; }
    void drawDownloadProgress(TFT_eSprite& spr, int stripY);

private:
    bool _active = false;
    SettingsPage _page = SettingsPage::Main;
    int8_t _selectedLang = -1;  // Selected language index in browser
    int8_t _scrollOffset = 0;

    struct Button { int x, y, w, h; };

    // Pressed button visual feedback
    Button _pressedBtn = {0, 0, 0, 0};
    uint32_t _pressedMs = 0;
    static const uint32_t PRESS_FLASH_MS = 200;

    bool hitTest(const Button& btn, TouchPoint pt);
    void flashPress();  // Immediate render to show blue press feedback
    void drawButton(TFT_eSprite& spr, const Button& btn, int stripY,
                    const char* label, bool selected);

    void drawMainPage(TFT_eSprite& spr, int stripY);
    void drawLanguageBrowser(TFT_eSprite& spr, int stripY);

    bool handleMainTap(TouchPoint pt);
    bool handleBrowserTap(TouchPoint pt);
};

extern SettingsScreen settingsUI;
