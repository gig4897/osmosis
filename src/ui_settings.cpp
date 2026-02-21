#include "ui_settings.h"
#include "display_manager.h"
#include "settings_manager.h"
#include "wifi_manager.h"
#include "pack_manager.h"
#include "constants.h"
#include <Esp.h>

SettingsScreen settingsUI;

// --- Words per day row ---
static const int WPD_VALUES[]   = {10, 15, 20};
static const char* WPD_LABELS[] = {"10", "15", "20"};
static const int WPD_COUNT      = 3;
static const int WPD_LABEL_Y    = 30;
static const int WPD_Y          = 44;
static const int WPD_H          = 36;
static const int WPD_W          = 68;
static const int WPD_X[]        = {10, 86, 162};

// --- Display time row ---
static const int DT_VALUES[]    = {30, 60, 120, 300};
static const char* DT_LABELS[]  = {"30s", "1m", "2m", "5m"};
static const int DT_COUNT       = 4;
static const int DT_LABEL_Y     = 86;
static const int DT_Y           = 100;
static const int DT_H           = 36;
static const int DT_W           = 52;
static const int DT_X[]         = {8, 66, 124, 182};

// --- Brightness row ---
static const int BR_VALUES[]    = {0, 1, 2};
static const char* BR_LABELS[]  = {"Low", "Med", "High"};
static const int BR_COUNT       = 3;
static const int BR_LABEL_Y     = 142;
static const int BR_Y           = 156;
static const int BR_H           = 36;
static const int BR_W           = 68;
static const int BR_X[]         = {10, 86, 162};

// --- Phonetic toggle row ---
static const char* PH_LABELS[]  = {"OFF", "ON"};
static const int PH_COUNT       = 2;
static const int PH_LABEL_Y     = 198;
static const int PH_Y           = 212;
static const int PH_H           = 36;
static const int PH_W           = 100;
static const int PH_X[]         = {20, 130};

// --- Language button ---
static const int LANG_X = 20;
static const int LANG_Y = 252;
static const int LANG_W = 200;
static const int LANG_H = 24;

// --- Bottom row: WiFi + Close side by side ---
static const int BOTTOM_Y = 290;
static const int BOTTOM_H = 26;
static const int WIFI_X = 10;
static const int WIFI_W = 105;
static const int CLOSE_X = 125;
static const int CLOSE_W = 105;

// -------------------------------------------------------
void SettingsScreen::show() {
    _active = true;
    _page = SettingsPage::Main;
    _selectedLang = -1;
}

void SettingsScreen::hide() {
    _active = false;
    _page = SettingsPage::Main;
}

// -------------------------------------------------------
bool SettingsScreen::hitTest(const Button& btn, TouchPoint pt) {
    bool hit = pt.x >= btn.x && pt.x < btn.x + btn.w &&
               pt.y >= btn.y && pt.y < btn.y + btn.h;
    if (hit) {
        _pressedBtn = btn;
        _pressedMs = millis();
    }
    return hit;
}

// -------------------------------------------------------
void SettingsScreen::drawButton(TFT_eSprite& spr, const Button& btn,
                                int stripY, const char* label, bool selected) {
    int btnTop    = btn.y;
    int btnBottom = btn.y + btn.h;
    int stripTop  = stripY;
    int stripBot  = stripY + STRIP_H;

    if (btnBottom <= stripTop || btnTop >= stripBot) return;

    int drawY = btnTop - stripY;
    int drawH = btn.h;
    if (drawY < 0) { drawH += drawY; drawY = 0; }
    if (drawY + drawH > STRIP_H) { drawH = STRIP_H - drawY; }

    // Check if this button is currently "pressed" (flash feedback)
    bool pressed = (_pressedMs > 0 && (millis() - _pressedMs < PRESS_FLASH_MS) &&
                    btn.x == _pressedBtn.x && btn.y == _pressedBtn.y &&
                    btn.w == _pressedBtn.w && btn.h == _pressedBtn.h);

    uint16_t fillClr  = pressed ? CLR_ACCENT : (selected ? CLR_BTN_ACTIVE : CLR_BTN_INACTIVE);
    uint16_t textClr  = pressed ? CLR_BG_DARK : (selected ? CLR_TEXT_PRIMARY : CLR_TEXT_SECONDARY);

    spr.fillRect(btn.x, drawY, btn.w, drawH, fillClr);

    uint16_t borderClr = selected ? CLR_ACCENT : CLR_BTN_INACTIVE;
    if (btnTop >= stripTop && btnTop < stripBot)
        spr.drawFastHLine(btn.x, btnTop - stripY, btn.w, borderClr);
    if (btnBottom - 1 >= stripTop && btnBottom - 1 < stripBot)
        spr.drawFastHLine(btn.x, btnBottom - 1 - stripY, btn.w, borderClr);
    spr.drawFastVLine(btn.x, drawY, drawH, borderClr);
    spr.drawFastVLine(btn.x + btn.w - 1, drawY, drawH, borderClr);

    int textScreenY = btnTop + (btn.h - 16) / 2;
    int y = textScreenY - stripY;
    if (y >= -16 && y < STRIP_H) {
        spr.setTextColor(textClr, fillClr);
        spr.setTextDatum(TC_DATUM);
        spr.drawString(label, btn.x + btn.w / 2, y, 2);
    }
}

// -------------------------------------------------------
void SettingsScreen::draw(TFT_eSprite& spr, int stripY) {
    switch (_page) {
        case SettingsPage::Main:
            drawMainPage(spr, stripY);
            break;
        case SettingsPage::LanguageBrowser:
            drawLanguageBrowser(spr, stripY);
            break;
        case SettingsPage::DownloadProgress:
            drawDownloadProgress(spr, stripY);
            break;
    }
}

// -------------------------------------------------------
void SettingsScreen::drawMainPage(TFT_eSprite& spr, int stripY) {
    spr.fillSprite(CLR_BG_DARK);
    const OsmosisSettings& s = settingsMgr.settings();

    // Title
    {
        int y = 6 - stripY;
        if (y >= -26 && y < STRIP_H) {
            spr.setTextColor(CLR_ACCENT, CLR_BG_DARK);
            spr.setTextDatum(TC_DATUM);
            spr.drawString("SETTINGS", SCREEN_W / 2, y, 4);
        }
    }

    // Row 1: Words per day
    {
        int y = WPD_LABEL_Y - stripY;
        if (y >= -16 && y < STRIP_H) {
            spr.setTextColor(CLR_TEXT_SECONDARY, CLR_BG_DARK);
            spr.setTextDatum(TC_DATUM);
            spr.drawString("Words per day", SCREEN_W / 2, y, 2);
        }
    }
    for (int i = 0; i < WPD_COUNT; i++) {
        Button btn = {WPD_X[i], WPD_Y, WPD_W, WPD_H};
        drawButton(spr, btn, stripY, WPD_LABELS[i], s.wordsPerDay == WPD_VALUES[i]);
    }

    // Row 2: Display time
    {
        int y = DT_LABEL_Y - stripY;
        if (y >= -16 && y < STRIP_H) {
            spr.setTextColor(CLR_TEXT_SECONDARY, CLR_BG_DARK);
            spr.setTextDatum(TC_DATUM);
            spr.drawString("Display time", SCREEN_W / 2, y, 2);
        }
    }
    for (int i = 0; i < DT_COUNT; i++) {
        Button btn = {DT_X[i], DT_Y, DT_W, DT_H};
        drawButton(spr, btn, stripY, DT_LABELS[i], s.displaySecs == (uint16_t)DT_VALUES[i]);
    }

    // Row 3: Brightness
    {
        int y = BR_LABEL_Y - stripY;
        if (y >= -16 && y < STRIP_H) {
            spr.setTextColor(CLR_TEXT_SECONDARY, CLR_BG_DARK);
            spr.setTextDatum(TC_DATUM);
            spr.drawString("Brightness", SCREEN_W / 2, y, 2);
        }
    }
    for (int i = 0; i < BR_COUNT; i++) {
        Button btn = {BR_X[i], BR_Y, BR_W, BR_H};
        drawButton(spr, btn, stripY, BR_LABELS[i], s.brightness == (uint8_t)BR_VALUES[i]);
    }

    // Row 4: Phonetic toggle
    {
        int y = PH_LABEL_Y - stripY;
        if (y >= -16 && y < STRIP_H) {
            spr.setTextColor(CLR_TEXT_SECONDARY, CLR_BG_DARK);
            spr.setTextDatum(TC_DATUM);
            spr.drawString("Phonetic", SCREEN_W / 2, y, 2);
        }
    }
    for (int i = 0; i < PH_COUNT; i++) {
        Button btn = {PH_X[i], PH_Y, PH_W, PH_H};
        bool sel = (i == 0 && !s.showPhonetic) || (i == 1 && s.showPhonetic);
        drawButton(spr, btn, stripY, PH_LABELS[i], sel);
    }

    // Row 5: Language button
    {
        Button btn = {LANG_X, LANG_Y, LANG_W, LANG_H};
        char label[32];
        if (strlen(s.installedLang) > 0) {
            snprintf(label, sizeof(label), "Lang: %s", s.installedLang);
        } else {
            strlcpy(label, "Browse Languages", sizeof(label));
        }
        drawButton(spr, btn, stripY, label, false);
    }

    // Bottom row: WiFi + Close side by side
    {
        Button wifiBtn = {WIFI_X, BOTTOM_Y, WIFI_W, BOTTOM_H};
        drawButton(spr, wifiBtn, stripY, "WiFi", false);
    }
    {
        Button closeBtn = {CLOSE_X, BOTTOM_Y, CLOSE_W, BOTTOM_H};
        drawButton(spr, closeBtn, stripY, "CLOSE", false);
    }
}

// -------------------------------------------------------
void SettingsScreen::drawLanguageBrowser(TFT_eSprite& spr, int stripY) {
    spr.fillSprite(CLR_BG_DARK);

    // Title
    {
        int y = 6 - stripY;
        if (y >= -26 && y < STRIP_H) {
            spr.setTextColor(CLR_ACCENT, CLR_BG_DARK);
            spr.setTextDatum(TC_DATUM);
            spr.drawString("LANGUAGES", SCREEN_W / 2, y, 4);
        }
    }

    uint8_t count = packMgr::languageCount();

    if (count == 0) {
        // No catalog loaded — show status
        {
            int y = 80 - stripY;
            if (y >= -16 && y < STRIP_H) {
                spr.setTextDatum(TC_DATUM);
                if (!wifiMgr::isConnected()) {
                    WiFiState ws = wifiMgr::state();
                    if (ws == WiFiState::Connecting) {
                        spr.setTextColor(CLR_TEXT_SECONDARY, CLR_BG_DARK);
                        spr.drawString("Connecting to WiFi...", SCREEN_W / 2, y, 2);
                    } else if (ws == WiFiState::CaptivePortalActive) {
                        spr.setTextColor(CLR_TEXT_SECONDARY, CLR_BG_DARK);
                        spr.drawString("Join 'Osmosis-Setup' WiFi", SCREEN_W / 2, y, 2);
                        int y2 = 100 - stripY;
                        if (y2 >= -16 && y2 < STRIP_H) {
                            spr.drawString("then open 192.168.4.1", SCREEN_W / 2, y2, 2);
                        }
                        int y3 = 120 - stripY;
                        if (y3 >= -16 && y3 < STRIP_H) {
                            spr.setTextColor(CLR_ACCENT, CLR_BG_DARK);
                            spr.drawString("vcodeworks.dev", SCREEN_W / 2, y3, 2);
                        }
                    } else {
                        spr.setTextColor(CLR_TEXT_SECONDARY, CLR_BG_DARK);
                        spr.drawString("WiFi not connected", SCREEN_W / 2, y, 2);
                    }
                } else {
                    spr.setTextColor(CLR_TEXT_SECONDARY, CLR_BG_DARK);
                    spr.drawString("Fetching catalog...", SCREEN_W / 2, y, 2);
                }
            }
        }

        // Retry button — try fetching catalog again when WiFi is connected
        if (wifiMgr::isConnected()) {
            Button retry = {60, 140, 120, 30};
            drawButton(spr, retry, stripY, "Retry", false);
        }

        // Back button
        Button back = {60, 290, 120, 26};
        drawButton(spr, back, stripY, "< BACK", false);
        return;
    }

    if (_selectedLang < 0) {
        // Show language list with pagination (6 per page)
        const int LANGS_PER_PAGE = 6;
        int startIdx = _scrollOffset * LANGS_PER_PAGE;
        int endIdx = startIdx + LANGS_PER_PAGE;
        if (endIdx > count) endIdx = count;
        int totalPages = (count + LANGS_PER_PAGE - 1) / LANGS_PER_PAGE;

        for (int i = startIdx; i < endIdx; i++) {
            int row = i - startIdx;
            int btnY = 40 + row * 40;
            Button btn = {20, btnY, 200, 34};
            drawButton(spr, btn, stripY, packMgr::language(i).name, false);
        }

        // Page indicator
        if (totalPages > 1) {
            int y = 280 - stripY;
            if (y >= -8 && y < STRIP_H) {
                char pgBuf[8];
                snprintf(pgBuf, sizeof(pgBuf), "%d/%d", _scrollOffset + 1, totalPages);
                spr.setTextColor(CLR_TEXT_DIM, CLR_BG_DARK);
                spr.setTextDatum(TC_DATUM);
                spr.drawString(pgBuf, SCREEN_W / 2, y, 1);
            }
        }

        // Navigation buttons: < BACK and NEXT >
        if (_scrollOffset > 0) {
            Button back = {10, 290, 105, 26};
            drawButton(spr, back, stripY, "< BACK", false);
        } else {
            Button back = {10, 290, 105, 26};
            drawButton(spr, back, stripY, "< BACK", false);
        }

        if (_scrollOffset < totalPages - 1) {
            Button next = {125, 290, 105, 26};
            drawButton(spr, next, stripY, "NEXT >", false);
        }
    } else {
        // Show tiers for selected language
        {
            int y = 36 - stripY;
            if (y >= -16 && y < STRIP_H) {
                spr.setTextColor(CLR_TEXT_SECONDARY, CLR_BG_DARK);
                spr.setTextDatum(TC_DATUM);
                spr.drawString(packMgr::language(_selectedLang).name, SCREEN_W / 2, y, 2);
            }
        }

        uint8_t tCount = packMgr::tierCount(_selectedLang);
        for (uint8_t i = 0; i < tCount; i++) {
            int btnY = 60 + i * 42;
            Button btn = {20, btnY, 200, 36};
            char label[32];
            const CatalogTier& t = packMgr::tier(_selectedLang, i);
            snprintf(label, sizeof(label), "%s (%u words)", t.name, t.words);
            drawButton(spr, btn, stripY, label, false);
        }

        // Back to language list
        Button back = {60, 280, 120, 26};
        drawButton(spr, back, stripY, "< BACK", false);
    }
}

// -------------------------------------------------------
void SettingsScreen::drawDownloadProgress(TFT_eSprite& spr, int stripY) {
    spr.fillSprite(CLR_BG_DARK);

    // Title
    {
        int y = 60 - stripY;
        if (y >= -26 && y < STRIP_H) {
            spr.setTextColor(CLR_ACCENT, CLR_BG_DARK);
            spr.setTextDatum(TC_DATUM);
            spr.drawString("Downloading", SCREEN_W / 2, y, 4);
        }
    }

    uint8_t pct = packMgr::progressPercent();

    // Progress bar outline (200x20, centered)
    {
        int barX = 20;
        int barY = 140;
        int barW = 200;
        int barH = 20;
        int barBottom = barY + barH;

        if (barBottom > stripY && barY < stripY + STRIP_H) {
            int drawY = barY - stripY;
            int drawH = barH;
            if (drawY < 0) { drawH += drawY; drawY = 0; }
            if (drawY + drawH > STRIP_H) { drawH = STRIP_H - drawY; }

            // Background
            spr.fillRect(barX, drawY, barW, drawH, CLR_BTN_INACTIVE);

            // Fill
            int fillW = (int)((uint32_t)barW * pct / 100);
            if (fillW > 0) {
                spr.fillRect(barX, drawY, fillW, drawH, CLR_ACCENT);
            }

            // Border
            if (barY >= stripY && barY < stripY + STRIP_H)
                spr.drawFastHLine(barX, barY - stripY, barW, CLR_TEXT_SECONDARY);
            if (barBottom - 1 >= stripY && barBottom - 1 < stripY + STRIP_H)
                spr.drawFastHLine(barX, barBottom - 1 - stripY, barW, CLR_TEXT_SECONDARY);
            spr.drawFastVLine(barX, drawY, drawH, CLR_TEXT_SECONDARY);
            spr.drawFastVLine(barX + barW - 1, drawY, drawH, CLR_TEXT_SECONDARY);
        }
    }

    // Percentage text
    {
        int y = 170 - stripY;
        if (y >= -16 && y < STRIP_H) {
            char buf[8];
            snprintf(buf, sizeof(buf), "%u%%", pct);
            spr.setTextColor(CLR_TEXT_PRIMARY, CLR_BG_DARK);
            spr.setTextDatum(TC_DATUM);
            spr.drawString(buf, SCREEN_W / 2, y, 4);
        }
    }

    // Status message
    {
        int y = 210 - stripY;
        if (y >= -16 && y < STRIP_H) {
            spr.setTextColor(CLR_TEXT_SECONDARY, CLR_BG_DARK);
            spr.setTextDatum(TC_DATUM);
            spr.drawString(packMgr::statusText(), SCREEN_W / 2, y, 2);
        }
    }
}

// -------------------------------------------------------
bool SettingsScreen::handleTap(TouchPoint pt) {
    switch (_page) {
        case SettingsPage::Main:
            return handleMainTap(pt);
        case SettingsPage::LanguageBrowser:
            return handleBrowserTap(pt);
        case SettingsPage::DownloadProgress:
            return false;  // No touch during download
    }
    return false;
}

// -------------------------------------------------------
bool SettingsScreen::handleMainTap(TouchPoint pt) {
    OsmosisSettings& s = settingsMgr.settings();

    // Words per day
    for (int i = 0; i < WPD_COUNT; i++) {
        Button btn = {WPD_X[i], WPD_Y, WPD_W, WPD_H};
        if (hitTest(btn, pt)) {
            s.wordsPerDay = (uint8_t)WPD_VALUES[i];
            return true;
        }
    }

    // Display time
    for (int i = 0; i < DT_COUNT; i++) {
        Button btn = {DT_X[i], DT_Y, DT_W, DT_H};
        if (hitTest(btn, pt)) {
            s.displaySecs = (uint16_t)DT_VALUES[i];
            return true;
        }
    }

    // Brightness
    for (int i = 0; i < BR_COUNT; i++) {
        Button btn = {BR_X[i], BR_Y, BR_W, BR_H};
        if (hitTest(btn, pt)) {
            s.brightness = (uint8_t)BR_VALUES[i];
            display.setBrightnessLevel((uint8_t)BR_VALUES[i]);
            return true;
        }
    }

    // Phonetic toggle
    for (int i = 0; i < PH_COUNT; i++) {
        Button btn = {PH_X[i], PH_Y, PH_W, PH_H};
        if (hitTest(btn, pt)) {
            s.showPhonetic = (i == 1);
            return true;
        }
    }

    // Language button — open browser
    {
        Button btn = {LANG_X, LANG_Y, LANG_W, LANG_H};
        if (hitTest(btn, pt)) {
            _page = SettingsPage::LanguageBrowser;
            _selectedLang = -1;
            _scrollOffset = 0;
            if (wifiMgr::isConnected()) {
                // Fetch catalog if not already loaded
                if (packMgr::languageCount() == 0) {
                    packMgr::fetchCatalog();
                }
            } else if (wifiMgr::state() == WiFiState::Disconnected ||
                       wifiMgr::state() == WiFiState::NotConfigured) {
                // Start captive portal only if truly disconnected (not still connecting)
                wifiMgr::startCaptivePortal();
            }
            // If still Connecting, just navigate — browser will show "Connecting..." status
            return true;
        }
    }

    // WiFi button — launch captive portal (even if already connected)
    {
        Button btn = {WIFI_X, BOTTOM_Y, WIFI_W, BOTTOM_H};
        if (hitTest(btn, pt)) {
            WiFiState ws = wifiMgr::state();
            if (ws != WiFiState::CaptivePortalActive) {
                if (wifiMgr::isConnected() || ws == WiFiState::Connecting) {
                    wifiMgr::disconnect();
                }
                wifiMgr::startCaptivePortal();
            }
            return true;
        }
    }

    // Close button — exit settings
    {
        Button btn = {CLOSE_X, BOTTOM_Y, CLOSE_W, BOTTOM_H};
        if (hitTest(btn, pt)) {
            settingsMgr.save();
            hide();
            return true;
        }
    }

    return false;
}

// -------------------------------------------------------
bool SettingsScreen::handleBrowserTap(TouchPoint pt) {
    uint8_t count = packMgr::languageCount();

    if (count == 0) {
        // No catalog loaded — handle retry and back
        if (wifiMgr::isConnected()) {
            Button retry = {60, 140, 120, 30};
            if (hitTest(retry, pt)) {
                packMgr::fetchCatalog();
                return true;
            }
        }
        Button back = {60, 290, 120, 26};
        if (hitTest(back, pt)) {
            _page = SettingsPage::Main;
            return true;
        }
        return false;
    }

    if (_selectedLang < 0) {
        // Language list with pagination
        const int LANGS_PER_PAGE = 6;
        int startIdx = _scrollOffset * LANGS_PER_PAGE;
        int endIdx = startIdx + LANGS_PER_PAGE;
        if (endIdx > count) endIdx = count;
        int totalPages = (count + LANGS_PER_PAGE - 1) / LANGS_PER_PAGE;

        for (int i = startIdx; i < endIdx; i++) {
            int row = i - startIdx;
            int btnY = 40 + row * 40;
            Button btn = {20, btnY, 200, 34};
            if (hitTest(btn, pt)) {
                _selectedLang = i;
                return true;
            }
        }

        // Back button
        Button back = {10, 290, 105, 26};
        if (hitTest(back, pt)) {
            if (_scrollOffset > 0) {
                _scrollOffset--;
            } else {
                _page = SettingsPage::Main;
            }
            return true;
        }

        // Next button
        if (_scrollOffset < totalPages - 1) {
            Button next = {125, 290, 105, 26};
            if (hitTest(next, pt)) {
                _scrollOffset++;
                return true;
            }
        }
    } else {
        // Tier list
        uint8_t tCount = packMgr::tierCount(_selectedLang);
        for (uint8_t i = 0; i < tCount; i++) {
            int btnY = 60 + i * 42;
            Button btn = {20, btnY, 200, 36};
            if (hitTest(btn, pt)) {
                // Switch to download progress page
                _page = SettingsPage::DownloadProgress;

                // Set progress callback to redraw screen during download
                packMgr::setProgressCallback([]() {
                    TFT_eSprite& spr = display.getStrip();
                    for (int strip = 0; strip < NUM_STRIPS; strip++) {
                        int sy = strip * STRIP_H;
                        spr.fillSprite(CLR_BG_DARK);
                        settingsUI.drawDownloadProgress(spr, sy);
                        display.pushStrip(sy);
                    }
                });

                // Draw initial progress screen before blocking download
                {
                    TFT_eSprite& spr = display.getStrip();
                    for (int strip = 0; strip < NUM_STRIPS; strip++) {
                        int sy = strip * STRIP_H;
                        spr.fillSprite(CLR_BG_DARK);
                        settingsUI.drawDownloadProgress(spr, sy);
                        display.pushStrip(sy);
                    }
                }

                // Synchronous/blocking download (callback redraws progress during this)
                bool ok = packMgr::startDownload(_selectedLang, i);

                // Clear callback after download
                packMgr::setProgressCallback(nullptr);

                if (ok && packMgr::state() == PackDownloadState::Complete) {
                    // Show "Restarting..." message then reboot.
                    // Rebooting ensures full heap is available for vocab loading.
                    // Settings are already saved by startDownload().
                    TFT_eSPI& tft = display.tft();
                    tft.fillScreen(CLR_BG_DARK);
                    tft.setTextDatum(TC_DATUM);
                    tft.setTextColor(CLR_ACCENT);
                    tft.drawString("Pack Installed!", SCREEN_W / 2, 100, 4);
                    tft.setTextColor(CLR_TEXT_SECONDARY);
                    tft.drawString("Restarting...", SCREEN_W / 2, 150, 2);
                    delay(1500);
                    ESP.restart();
                } else {
                    // Download failed — show error briefly then return to browser
                    TFT_eSPI& tft = display.tft();
                    tft.fillScreen(CLR_BG_DARK);
                    tft.setTextDatum(TC_DATUM);
                    tft.setTextColor(0xF800);  // Red
                    tft.drawString("Download Failed", SCREEN_W / 2, 120, 4);
                    tft.setTextColor(CLR_TEXT_SECONDARY);
                    tft.drawString(packMgr::statusText(), SCREEN_W / 2, 160, 2);
                    delay(3000);
                    packMgr::resetState();
                    _selectedLang = -1;
                    _scrollOffset = 0;
                    _page = SettingsPage::LanguageBrowser;
                }
                return true;
            }
        }

        // Back to language list
        Button back = {60, 280, 120, 26};
        if (hitTest(back, pt)) {
            _selectedLang = -1;
            return true;
        }
    }

    return false;
}
