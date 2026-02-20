# Osmosis v1.0 Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Build a passive Spanish vocabulary flashcard device for ESP32 + ILI9341 that auto-rotates 120x120 emoji images with word translations.

**Architecture:** Same hardware/pin config as Tide Clock 3.0. Portrait mode (240x320). Strip-based rendering (240x10 sprites). Global singleton pattern for all managers. Images stored as RLE-compressed RGB565 bitmaps in SPIFFS. Settings in NVS (Preferences). Word data compiled into a C header.

**Tech Stack:** ESP32 (esp32dev), PlatformIO, Arduino framework, TFT_eSPI, XPT2046_Touchscreen, SPIFFS, Preferences

**Reference Project:** `/Users/kevintomlinson/Coding/Tide Clock/Software/2026` — follow its patterns for display management, touch handling, settings, and global singletons.

---

## Task 1: Project Scaffolding

**Files:**
- Create: `platformio.ini`
- Create: `src/main.cpp`
- Create: `src/constants.h`

**Step 1: Create platformio.ini**

Match the Tide Clock config but drop ArduinoJson (not needed). Keep all TFT build_flags identical. Keep `board_build.filesystem = spiffs` and `board_build.partitions = huge_app.csv`.

```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
monitor_speed = 115200
board_build.filesystem = spiffs
board_build.partitions = huge_app.csv

lib_deps =
    bodmer/TFT_eSPI@^2.5.43
    https://github.com/PaulStoffregen/XPT2046_Touchscreen.git

build_flags =
    -D USER_SETUP_LOADED=1
    -D ILI9341_DRIVER=1
    -D TFT_WIDTH=240
    -D TFT_HEIGHT=320
    -D TFT_MISO=-1
    -D TFT_MOSI=13
    -D TFT_SCLK=14
    -D TFT_CS=15
    -D TFT_DC=2
    -D TFT_RST=4
    -D SPI_FREQUENCY=40000000
    -D SPI_READ_FREQUENCY=20000000
    -D LOAD_GLCD=1  -D LOAD_FONT2=1  -D LOAD_FONT4=1
    -D LOAD_FONT6=1  -D LOAD_FONT7=1  -D LOAD_FONT8=1
    -D LOAD_GFXFF=1  -D SMOOTH_FONT=1
```

**Step 2: Create src/constants.h**

Define all pin constants, display dimensions (portrait: W=240, H=320), strip size, color palette, and backlight PWM constants. Follow Tide Clock's `constexpr` pattern.

Key differences from Tide Clock:
- `SCREEN_W = 240`, `SCREEN_H = 320` (portrait, rotation 0)
- `STRIP_H = 10`, `NUM_STRIPS = 32` (320 / 10)
- New color palette for Osmosis theme (dark blue background, cyan accents matching the mockup)
- Image dimensions: `IMG_W = 120`, `IMG_H = 120`

```cpp
#pragma once
#include <cstdint>

// === Firmware ===
constexpr const char* FW_NAME    = "Osmosis";
constexpr const char* FW_VERSION = "1.0.0";

// === Pin Definitions ===
// TFT SPI pins defined in build_flags (MOSI=13, SCLK=14, CS=15, DC=2, RST=4)
constexpr int PIN_TFT_BL = 21;

// Touch HSPI pins
constexpr int PIN_TOUCH_CS   = 33;
constexpr int PIN_TOUCH_CLK  = 25;
constexpr int PIN_TOUCH_MOSI = 32;
constexpr int PIN_TOUCH_MISO = 39;

// === Display Layout (Portrait) ===
constexpr int SCREEN_W = 240;
constexpr int SCREEN_H = 320;
constexpr int STRIP_H  = 10;
constexpr int NUM_STRIPS = SCREEN_H / STRIP_H;  // 32

// Header bar
constexpr int HEADER_H = 30;

// Image area
constexpr int IMG_W = 120;
constexpr int IMG_H = 120;
constexpr int IMG_X = (SCREEN_W - IMG_W) / 2;  // 60
constexpr int IMG_Y = HEADER_H + 15;            // 45

// Word box
constexpr int WORD_BOX_Y = IMG_Y + IMG_H + 15;  // 180
constexpr int WORD_BOX_W = 200;
constexpr int WORD_BOX_H = 70;
constexpr int WORD_BOX_X = (SCREEN_W - WORD_BOX_W) / 2;  // 20

// Card counter
constexpr int COUNTER_Y = SCREEN_H - 20;  // 300

// === Backlight PWM ===
constexpr int BL_PWM_CHANNEL = 0;
constexpr int BL_PWM_FREQ    = 5000;
constexpr int BL_PWM_RES     = 8;
constexpr uint8_t BRIGHTNESS_LEVELS[] = {50, 150, 255};
constexpr int NUM_BRIGHTNESS = 3;

// === Touch Calibration ===
constexpr int TOUCH_X_MIN = 300;
constexpr int TOUCH_X_MAX = 3800;
constexpr int TOUCH_Y_MIN = 300;
constexpr int TOUCH_Y_MAX = 3800;
constexpr unsigned long LONG_PRESS_MS = 2000;

// === Color Palette (RGB565) ===
constexpr uint16_t CLR_BG_DARK       = 0x0000;  // Pure black
constexpr uint16_t CLR_BG_CARD       = 0x0861;  // Very dark navy (#0a1628)
constexpr uint16_t CLR_HEADER_BG     = 0x1269;  // Dark blue header
constexpr uint16_t CLR_ACCENT        = 0x4E1F;  // Cyan accent (#4fc3f7)
constexpr uint16_t CLR_TEXT_PRIMARY   = 0xFFFF;  // White
constexpr uint16_t CLR_TEXT_SECONDARY = 0x8C51;  // Muted blue-gray (#80b0cc)
constexpr uint16_t CLR_TEXT_DIM       = 0x4208;  // Dim gray
constexpr uint16_t CLR_WORD_BOX_BG   = 0x0882;  // Slight blue tint
constexpr uint16_t CLR_WORD_BOX_BORDER = 0x4E1F; // Cyan border
constexpr uint16_t CLR_SPLASH_GREEN  = 0x06D5;  // Osmosis green (#00d4aa)
constexpr uint16_t CLR_BTN_ACTIVE    = 0x4E1F;  // Active button
constexpr uint16_t CLR_BTN_INACTIVE  = 0x2104;  // Inactive button

// === Settings Defaults ===
constexpr uint8_t DEFAULT_WORDS_PER_DAY  = 15;
constexpr uint8_t DEFAULT_DISPLAY_SECS   = 60;  // 1 minute
constexpr uint8_t DEFAULT_BRIGHTNESS     = 1;   // Medium

// === Timing ===
constexpr unsigned long TOUCH_POLL_MS = 20;      // 50Hz
constexpr unsigned long SPLASH_DURATION_MS = 3000;
```

**Step 3: Create minimal src/main.cpp**

Bare bones: include constants.h, empty `setup()` that prints "Osmosis v1.0.0" to Serial, empty `loop()`.

```cpp
#include <Arduino.h>
#include "constants.h"

void setup() {
    Serial.begin(115200);
    Serial.println("Osmosis v1.0.0 booting...");
}

void loop() {
}
```

**Step 4: Build to verify compilation**

Run: `cd /Users/kevintomlinson/Coding/Osmosis && pio run`
Expected: BUILD SUCCESS (downloads platform/libs on first run)

**Step 5: Commit**

```bash
git add platformio.ini src/main.cpp src/constants.h
git commit -m "feat: project scaffolding with pin config and constants"
```

---

## Task 2: Display Manager

**Files:**
- Create: `src/display_manager.h`
- Create: `src/display_manager.cpp`
- Modify: `src/main.cpp`

**Step 1: Create src/display_manager.h**

Follow Tide Clock's pattern exactly: TFT_eSPI member, TFT_eSprite strip member, init/brightness/strip methods, global extern.

Portrait mode differences: strip is 240x10 (not 320x10), rotation 0.

```cpp
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
```

**Step 2: Create src/display_manager.cpp**

Implement init (same LED PWM setup as Tide Clock, rotation 0 for portrait), brightness control, strip sprite creation (240xSTRIP_H), pushStrip, blendColor, dimColor.

```cpp
#include "display_manager.h"

DisplayManager display;

void DisplayManager::init() {
    ledcSetup(BL_PWM_CHANNEL, BL_PWM_FREQ, BL_PWM_RES);
    ledcAttachPin(PIN_TFT_BL, BL_PWM_CHANNEL);
    ledcWrite(BL_PWM_CHANNEL, BRIGHTNESS_LEVELS[DEFAULT_BRIGHTNESS]);

    _tft.init();
    _tft.setRotation(0);  // Portrait: 240 wide x 320 tall
    _tft.fillScreen(CLR_BG_DARK);

    _strip.setColorDepth(16);
    _strip.createSprite(SCREEN_W, STRIP_H);  // 240 x 10
}

void DisplayManager::setBrightness(uint8_t level) {
    ledcWrite(BL_PWM_CHANNEL, level);
}

void DisplayManager::setBrightnessLevel(uint8_t idx) {
    if (idx >= NUM_BRIGHTNESS) idx = NUM_BRIGHTNESS - 1;
    _brightnessIdx = idx;
    ledcWrite(BL_PWM_CHANNEL, BRIGHTNESS_LEVELS[idx]);
}

uint8_t DisplayManager::getBrightnessLevel() const {
    return _brightnessIdx;
}

TFT_eSprite& DisplayManager::getStrip() {
    return _strip;
}

void DisplayManager::pushStrip(int y) {
    _strip.pushSprite(0, y);
}

uint16_t DisplayManager::blendColor(uint16_t c1, uint16_t c2, float ratio) {
    uint8_t r1 = (c1 >> 11) & 0x1F, g1 = (c1 >> 5) & 0x3F, b1 = c1 & 0x1F;
    uint8_t r2 = (c2 >> 11) & 0x1F, g2 = (c2 >> 5) & 0x3F, b2 = c2 & 0x1F;
    uint8_t r = r1 + (r2 - r1) * ratio;
    uint8_t g = g1 + (g2 - g1) * ratio;
    uint8_t b = b1 + (b2 - b1) * ratio;
    return (r << 11) | (g << 5) | b;
}

uint16_t DisplayManager::dimColor(uint16_t color, float factor) {
    uint8_t r = ((color >> 11) & 0x1F) * factor;
    uint8_t g = ((color >> 5) & 0x3F) * factor;
    uint8_t b = (color & 0x1F) * factor;
    return (r << 11) | (g << 5) | b;
}
```

**Step 3: Update src/main.cpp to init display**

Add `#include "display_manager.h"` and call `display.init()` in setup().

**Step 4: Build to verify**

Run: `cd /Users/kevintomlinson/Coding/Osmosis && pio run`
Expected: BUILD SUCCESS

**Step 5: Commit**

```bash
git add src/display_manager.h src/display_manager.cpp src/main.cpp
git commit -m "feat: display manager with strip rendering and backlight control"
```

---

## Task 3: Splash Screen

**Files:**
- Create: `src/splash_screen.h`
- Create: `src/splash_screen.cpp`
- Modify: `src/main.cpp`

**Step 1: Create src/splash_screen.h**

```cpp
#pragma once
#include "display_manager.h"

namespace splash {
    void show();  // Draws splash directly to TFT (not strip-based, like Tide Clock's showSplash)
}
```

**Step 2: Create src/splash_screen.cpp**

Draw directly to `display.tft()` (same pattern as Tide Clock's splash — direct TFT, not strips):
- Fill screen with radial gradient (dark navy center to black edges — approximate with horizontal bands)
- "OSMOSIS" in large font, centered, CLR_SPLASH_GREEN, with letter spacing
- "LEARN BY IMMERSION" smaller below, muted green
- Three animated dots (draw, delay 300ms, draw next, etc.)
- "v1.0 - Spanish" near bottom in dim text
- "by VCodeworks LLC" at very bottom in muted green
- Total splash duration ~3 seconds

```cpp
#include "splash_screen.h"
#include "constants.h"

void splash::show() {
    TFT_eSPI& tft = display.tft();

    // Background gradient (approximate radial with vertical bands)
    for (int y = 0; y < SCREEN_H; y++) {
        float dist = abs(y - SCREEN_H / 2) / (float)(SCREEN_H / 2);
        uint16_t color = DisplayManager::blendColor(0x1269, CLR_BG_DARK, dist);
        tft.drawFastHLine(0, y, SCREEN_W, color);
    }

    // Title: OSMOSIS
    tft.setTextDatum(TC_DATUM);
    tft.setTextColor(CLR_SPLASH_GREEN);
    tft.drawString("OSMOSIS", SCREEN_W / 2, 110, 6);  // Font 6 = large

    // Tagline
    tft.setTextColor(DisplayManager::dimColor(CLR_SPLASH_GREEN, 0.5));
    tft.drawString("LEARN BY IMMERSION", SCREEN_W / 2, 155, 2);

    // Animated loading dots
    int dotY = 200;
    int dotSpacing = 16;
    int dotStartX = SCREEN_W / 2 - dotSpacing;
    for (int i = 0; i < 3; i++) {
        tft.fillCircle(dotStartX + i * dotSpacing, dotY, 3, CLR_SPLASH_GREEN);
        delay(400);
    }

    // Version
    tft.setTextColor(CLR_TEXT_DIM);
    tft.drawString("v1.0 - Spanish", SCREEN_W / 2, 260, 2);

    // Company
    tft.setTextColor(DisplayManager::dimColor(CLR_SPLASH_GREEN, 0.5));
    tft.drawString("by VCodeworks LLC", SCREEN_W / 2, 280, 2);

    // Hold for remaining splash time
    delay(1000);
}
```

**Step 3: Update main.cpp to show splash on boot**

```cpp
#include "splash_screen.h"
// In setup():
display.init();
splash::show();
```

**Step 4: Build to verify**

Run: `cd /Users/kevintomlinson/Coding/Osmosis && pio run`
Expected: BUILD SUCCESS

**Step 5: Commit**

```bash
git add src/splash_screen.h src/splash_screen.cpp src/main.cpp
git commit -m "feat: animated splash screen with branding"
```

---

## Task 4: Touch Handler

**Files:**
- Create: `src/touch_handler.h`
- Create: `src/touch_handler.cpp`
- Modify: `src/main.cpp`

**Step 1: Create src/touch_handler.h**

Simplified version of Tide Clock's handler — we only need long press detection (no swipe, no tap needed for v1.0 card display). Keep the GestureType enum minimal.

```cpp
#pragma once
#include <cstdint>

enum GestureType : uint8_t {
    GESTURE_NONE,
    GESTURE_TAP,
    GESTURE_LONG_PRESS
};

struct TouchPoint { int16_t x; int16_t y; };

class TouchHandler {
public:
    void init();
    GestureType update();  // Call at 50Hz
    TouchPoint getLastTap() const;

private:
    enum State : uint8_t { IDLE, TOUCH_DOWN };

    void* _hspi = nullptr;   // SPIClass*
    void* _ts = nullptr;     // XPT2046_Touchscreen*

    State _state = IDLE;
    int16_t _startX = 0, _startY = 0;
    uint32_t _startTime = 0;
    bool _longPressFired = false;
    TouchPoint _lastTap = {0, 0};

    int16_t mapX(int16_t raw);
    int16_t mapY(int16_t raw);
    bool readTouch(int16_t& x, int16_t& y);
};

extern TouchHandler touch;
```

**Step 2: Create src/touch_handler.cpp**

Port Tide Clock's touch handler logic. Init HSPI + XPT2046, rotation 0 (portrait). State machine: IDLE → TOUCH_DOWN on contact. While held: check for long press (2s, <20px movement). On release: check for tap (<2s, <40px movement). Map raw coords to screen coords using calibration values.

**Step 3: Update main.cpp**

Add `#include "touch_handler.h"`, call `touch.init()` in setup(). In loop(), poll touch at 50Hz and check for GESTURE_LONG_PRESS (print to Serial for now).

**Step 4: Build to verify**

Run: `cd /Users/kevintomlinson/Coding/Osmosis && pio run`
Expected: BUILD SUCCESS

**Step 5: Commit**

```bash
git add src/touch_handler.h src/touch_handler.cpp src/main.cpp
git commit -m "feat: touch handler with long press detection"
```

---

## Task 5: Settings Manager (NVS Persistence)

**Files:**
- Create: `src/settings_manager.h`
- Create: `src/settings_manager.cpp`
- Modify: `src/main.cpp`

**Step 1: Create src/settings_manager.h**

Follow Tide Clock's ConfigManager pattern (Preferences/NVS). Much simpler struct — only 3 settings + progress tracking.

```cpp
#pragma once
#include <Preferences.h>
#include <cstdint>

struct OsmosisSettings {
    uint8_t wordsPerDay;     // 5, 10, 15, 20, 25
    uint8_t displaySecs;     // 30, 60, 120, 300
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
```

**Step 2: Create src/settings_manager.cpp**

- Namespace: `"osmosis"`
- Keys: `"wpd"` (wordsPerDay), `"dsecs"` (displaySecs), `"bright"` (brightness), `"progress"` (progressIndex), `"lastday"` (lastDay)
- `setDefaults()`: wordsPerDay=15, displaySecs=60, brightness=1, progressIndex=0, lastDay=0
- `load()`: setDefaults() then read from NVS with defaults as fallback
- `save()`: write all fields
- `saveProgress()`: write only progressIndex and lastDay

**Step 3: Update main.cpp**

Add `#include "settings_manager.h"`, call `settingsMgr.init()` in setup() after splash. Apply brightness from settings.

**Step 4: Build to verify**

Run: `cd /Users/kevintomlinson/Coding/Osmosis && pio run`
Expected: BUILD SUCCESS

**Step 5: Commit**

```bash
git add src/settings_manager.h src/settings_manager.cpp src/main.cpp
git commit -m "feat: settings manager with NVS persistence"
```

---

## Task 6: Word Data

**Files:**
- Create: `src/word_data.h`

**Step 1: Create src/word_data.h**

Define the word struct and a PROGMEM array of ~300 Spanish words ranked by frequency. Each entry: Spanish word, English translation, image filename (without extension), category tag.

Only include nouns, verbs, and adjectives that have clear emoji representations. Skip abstract/function words.

```cpp
#pragma once
#include <pgmspace.h>

struct WordEntry {
    const char* spanish;
    const char* english;
    const char* imageFile;  // filename in SPIFFS without extension, e.g. "casa"
    const char* category;
};

// ~300 entries, ordered by frequency (most common first)
// Each word has a clear emoji/image representation
const WordEntry WORD_LIST[] PROGMEM = {
    {"agua",      "water",      "agua",      "nature"},
    {"casa",      "house",      "casa",      "home"},
    {"perro",     "dog",        "perro",     "animals"},
    {"gato",      "cat",        "gato",      "animals"},
    {"sol",       "sun",        "sol",       "nature"},
    {"luna",      "moon",       "luna",      "nature"},
    {"libro",     "book",       "libro",     "objects"},
    {"mano",      "hand",       "mano",      "body"},
    {"ojo",       "eye",        "ojo",       "body"},
    {"coche",     "car",        "coche",     "transport"},
    // ... continue for ~300 entries
};

constexpr int WORD_COUNT = sizeof(WORD_LIST) / sizeof(WORD_LIST[0]);
```

The full list should be ~300 high-frequency Spanish words with emoji mappings. Build this list by cross-referencing Spanish frequency lists with available emoji. Categories: animals, food, nature, home, body, transport, objects, people, clothing, weather, sports, tools, places.

**Step 2: Build to verify**

Run: `cd /Users/kevintomlinson/Coding/Osmosis && pio run`
Expected: BUILD SUCCESS

**Step 3: Commit**

```bash
git add src/word_data.h
git commit -m "feat: Spanish word list with 300 frequency-ranked entries"
```

---

## Task 7: Image Converter Tool

**Files:**
- Create: `tools/convert_emoji.py`

**Step 1: Create tools/convert_emoji.py**

Python script that:
1. Takes an input directory of PNG emoji images (120x120)
2. Converts each to RGB565 format
3. Applies RLE compression
4. Writes .bin files to the `data/` directory for SPIFFS upload

RLE format:
- Byte header: if high bit set (0x80), next byte is a pixel repeated (count & 0x7F + 1) times. Each pixel is 2 bytes (RGB565 big-endian).
- If high bit clear, the following (count + 1) pixels are literal (uncompressed).
- This handles both solid-color regions (common in emoji) and detailed areas efficiently.

The script should also:
- Resize images to 120x120 if needed
- Report compression ratio per image and total SPIFFS usage
- Validate total fits in ~1.5MB

Dependencies: `pip install Pillow`

```python
#!/usr/bin/env python3
"""Convert emoji PNG images to RLE-compressed RGB565 .bin files for ESP32 SPIFFS."""

import os
import sys
import struct
from pathlib import Path
from PIL import Image

IMG_SIZE = 120

def rgb888_to_rgb565(r, g, b):
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)

def rle_compress(pixels):
    """RLE compress a list of RGB565 uint16 values."""
    result = bytearray()
    i = 0
    n = len(pixels)
    while i < n:
        # Check for run of identical pixels
        run_start = i
        while i < n - 1 and pixels[i] == pixels[i + 1] and (i - run_start) < 127:
            i += 1
        run_len = i - run_start + 1
        if run_len >= 3:
            # Encoded run: high bit set, count, pixel
            result.append(0x80 | (run_len - 1))
            result.extend(struct.pack('>H', pixels[run_start]))
            i += 1
        else:
            # Literal run: find extent of non-repeating pixels
            i = run_start
            lit_start = i
            while i < n and (i - lit_start) < 127:
                if i < n - 2 and pixels[i] == pixels[i+1] == pixels[i+2]:
                    break
                i += 1
            lit_len = i - lit_start
            result.append(lit_len - 1)  # high bit clear
            for j in range(lit_start, lit_start + lit_len):
                result.extend(struct.pack('>H', pixels[j]))
    return bytes(result)

def convert_image(input_path, output_path):
    img = Image.open(input_path).convert('RGB')
    img = img.resize((IMG_SIZE, IMG_SIZE), Image.LANCZOS)

    pixels = []
    for y in range(IMG_SIZE):
        for x in range(IMG_SIZE):
            r, g, b = img.getpixel((x, y))
            pixels.append(rgb888_to_rgb565(r, g, b))

    compressed = rle_compress(pixels)
    raw_size = IMG_SIZE * IMG_SIZE * 2

    with open(output_path, 'wb') as f:
        # Header: 4 bytes magic, 2 bytes width, 2 bytes height, 4 bytes compressed size
        f.write(b'ORLE')
        f.write(struct.pack('>HHI', IMG_SIZE, IMG_SIZE, len(compressed)))
        f.write(compressed)

    return raw_size, len(compressed) + 12  # +12 for header

def main():
    if len(sys.argv) < 3:
        print("Usage: convert_emoji.py <input_dir> <output_dir>")
        sys.exit(1)

    input_dir = Path(sys.argv[1])
    output_dir = Path(sys.argv[2])
    output_dir.mkdir(parents=True, exist_ok=True)

    total_raw = 0
    total_compressed = 0
    count = 0

    for png in sorted(input_dir.glob('*.png')):
        name = png.stem
        out_path = output_dir / f"{name}.bin"
        raw, compressed = convert_image(png, out_path)
        ratio = compressed / raw * 100
        print(f"  {name}: {raw} -> {compressed} bytes ({ratio:.1f}%)")
        total_raw += raw
        total_compressed += compressed
        count += 1

    print(f"\n{count} images converted")
    print(f"Total: {total_raw/1024:.1f}KB raw -> {total_compressed/1024:.1f}KB compressed")
    print(f"SPIFFS usage: {total_compressed/1024:.1f}KB of ~1500KB available")

    if total_compressed > 1500 * 1024:
        print("WARNING: Total exceeds SPIFFS capacity!")
        sys.exit(1)

if __name__ == '__main__':
    main()
```

**Step 2: Test with a sample image**

Create a test: make a simple 120x120 solid-color PNG, run the converter, verify .bin output exists and is smaller than 28KB.

Run: `cd /Users/kevintomlinson/Coding/Osmosis && python3 -c "from PIL import Image; img = Image.new('RGB', (120,120), (255,0,0)); img.save('test_emoji.png')" && python3 tools/convert_emoji.py . data/ && ls -la data/test_emoji.bin && rm test_emoji.png data/test_emoji.bin`

Expected: Solid color image should compress extremely well (~200 bytes).

**Step 3: Commit**

```bash
git add tools/convert_emoji.py
git commit -m "feat: emoji PNG to RLE-compressed RGB565 converter"
```

---

## Task 8: Emoji Source Images

**Files:**
- Create: `tools/download_emoji.py`
- Create: `data/*.bin` (generated)

**Step 1: Create tools/download_emoji.py**

Python script that:
1. Reads the word list from word_data.h (or a shared JSON/text mapping)
2. For each word, downloads the corresponding emoji from a free source (Twemoji/Noto Emoji via URL, or use a local emoji font renderer)
3. Saves as 120x120 PNGs in a `tools/emoji_png/` directory
4. Then runs convert_emoji.py to generate .bin files in `data/`

Emoji mapping: maintain a Python dict mapping each imageFile name to its Unicode codepoint (e.g. `"casa": "1f3e0"` for the house emoji). Use Twemoji SVGs rendered to PNG at 120x120, or Noto Color Emoji PNGs.

The script should handle the full pipeline: download/render → resize → convert → verify SPIFFS budget.

**Step 2: Run the pipeline**

Run: `cd /Users/kevintomlinson/Coding/Osmosis && python3 tools/download_emoji.py`
Expected: ~300 .bin files in data/, total under 1.5MB

**Step 3: Verify SPIFFS budget**

Run: `du -sh /Users/kevintomlinson/Coding/Osmosis/data/`
Expected: Under 1.5MB

**Step 4: Commit**

```bash
git add tools/download_emoji.py data/*.bin
git commit -m "feat: 300 emoji images converted for SPIFFS"
```

---

## Task 9: Image Renderer (RLE Decoder)

**Files:**
- Create: `src/image_renderer.h`
- Create: `src/image_renderer.cpp`
- Modify: `src/main.cpp`

**Step 1: Create src/image_renderer.h**

```cpp
#pragma once
#include <cstdint>

namespace imageRenderer {
    // Load and draw an RLE-compressed RGB565 image from SPIFFS
    // Draws into the strip sprite during strip rendering
    // x, y: screen position of top-left corner
    // filename: SPIFFS path e.g. "/casa.bin"
    bool drawImage(const char* filename, int x, int y, int stripY);

    // Preload image into RAM buffer for faster strip rendering
    // Call once when changing cards, then drawImage reads from buffer
    bool preloadImage(const char* filename);

    // Draw preloaded image into strip
    void drawPreloaded(int x, int y, int stripY);
}
```

**Step 2: Create src/image_renderer.cpp**

Two-phase approach for performance:
1. `preloadImage()` — reads .bin from SPIFFS, RLE-decompresses into a heap buffer (120*120*2 = 28,800 bytes). Called once per card change.
2. `drawPreloaded()` — for each strip call, copies the relevant 120-pixel-wide x STRIP_H rows from the buffer into the strip sprite.

This avoids re-reading SPIFFS and re-decompressing for every strip render pass.

```cpp
#include "image_renderer.h"
#include "display_manager.h"
#include "constants.h"
#include <SPIFFS.h>

static uint16_t* imageBuffer = nullptr;  // 120 * 120 = 14400 uint16s = 28800 bytes

bool imageRenderer::preloadImage(const char* filename) {
    char path[32];
    snprintf(path, sizeof(path), "/%s.bin", filename);

    File f = SPIFFS.open(path, "r");
    if (!f) return false;

    // Read header
    char magic[4];
    f.read((uint8_t*)magic, 4);
    if (memcmp(magic, "ORLE", 4) != 0) { f.close(); return false; }

    uint16_t w, h;
    uint32_t compSize;
    f.read((uint8_t*)&w, 2); w = (w >> 8) | (w << 8);  // big-endian to native
    f.read((uint8_t*)&h, 2); h = (h >> 8) | (h << 8);
    uint8_t cs[4]; f.read(cs, 4);
    compSize = (cs[0]<<24)|(cs[1]<<16)|(cs[2]<<8)|cs[3];

    if (!imageBuffer) {
        imageBuffer = (uint16_t*)malloc(IMG_W * IMG_H * 2);
        if (!imageBuffer) { f.close(); return false; }
    }

    // Read compressed data
    uint8_t* compressed = (uint8_t*)malloc(compSize);
    if (!compressed) { f.close(); return false; }
    f.read(compressed, compSize);
    f.close();

    // RLE decompress
    int pixIdx = 0;
    int i = 0;
    while (i < (int)compSize && pixIdx < IMG_W * IMG_H) {
        uint8_t header = compressed[i++];
        if (header & 0x80) {
            // Run: repeat pixel
            int count = (header & 0x7F) + 1;
            uint16_t pixel = (compressed[i] << 8) | compressed[i+1]; i += 2;
            for (int j = 0; j < count && pixIdx < IMG_W * IMG_H; j++) {
                imageBuffer[pixIdx++] = pixel;
            }
        } else {
            // Literal: copy pixels
            int count = header + 1;
            for (int j = 0; j < count && pixIdx < IMG_W * IMG_H; j++) {
                imageBuffer[pixIdx] = (compressed[i] << 8) | compressed[i+1]; i += 2;
                pixIdx++;
            }
        }
    }

    free(compressed);
    return true;
}

void imageRenderer::drawPreloaded(int x, int y, int stripY) {
    if (!imageBuffer) return;

    TFT_eSprite& spr = display.getStrip();

    // Which rows of the image fall in this strip?
    int imgStartRow = stripY - y;
    int imgEndRow = imgStartRow + STRIP_H;

    if (imgStartRow >= IMG_H || imgEndRow <= 0) return;  // No overlap

    int rowStart = max(0, imgStartRow);
    int rowEnd = min(IMG_H, imgEndRow);

    for (int row = rowStart; row < rowEnd; row++) {
        int spriteY = (y + row) - stripY;
        for (int col = 0; col < IMG_W; col++) {
            uint16_t pixel = imageBuffer[row * IMG_W + col];
            if (pixel != 0x0000) {  // Skip black (transparent)
                spr.drawPixel(x + col, spriteY, pixel);
            }
        }
    }
}
```

**Step 3: Update main.cpp to init SPIFFS**

Add `SPIFFS.begin(true)` in setup() after display init.

**Step 4: Build to verify**

Run: `cd /Users/kevintomlinson/Coding/Osmosis && pio run`
Expected: BUILD SUCCESS

**Step 5: Commit**

```bash
git add src/image_renderer.h src/image_renderer.cpp src/main.cpp
git commit -m "feat: RLE image decoder with preload buffer for strip rendering"
```

---

## Task 10: Card Manager (Rotation Logic)

**Files:**
- Create: `src/card_manager.h`
- Create: `src/card_manager.cpp`
- Modify: `src/main.cpp`

**Step 1: Create src/card_manager.h**

```cpp
#pragma once
#include "word_data.h"
#include <cstdint>

class CardManager {
public:
    void init();                    // Load progress, build today's batch
    void update();                  // Check timer, advance card if needed
    void checkDayChange();          // Advance to next batch at midnight

    const WordEntry& currentWord() const;
    int currentCardIndex() const;   // 1-based position in today's batch
    int totalCardsToday() const;    // wordsPerDay

private:
    uint16_t _dailyBatch[25];       // Indices into WORD_LIST (max 25 words/day)
    uint8_t _batchSize = 0;
    uint8_t _currentPos = 0;        // Position within today's batch
    uint32_t _lastCardChangeMs = 0;
    bool _imageLoaded = false;

    void buildBatch();              // Select and shuffle today's words
    void loadCurrentImage();
    void shuffleBatch();
};

extern CardManager cardMgr;
```

**Step 2: Create src/card_manager.cpp**

- `init()`: Read progressIndex from settingsMgr. Call buildBatch(). Load first image.
- `buildBatch()`: Starting from progressIndex, take the next wordsPerDay words (wrapping around WORD_COUNT). Shuffle using Fisher-Yates with `random()`.
- `update()`: If `millis() - _lastCardChangeMs >= displaySecs * 1000`, advance to next card in batch (wrap to 0). Preload next image.
- `checkDayChange()`: If current day != lastDay, advance progressIndex by wordsPerDay, save progress, rebuild batch.
- `currentWord()`: Return `WORD_LIST[_dailyBatch[_currentPos]]`.

**Step 3: Update main.cpp**

Init cardMgr after settingsMgr. In loop(), call `cardMgr.update()` and `cardMgr.checkDayChange()`.

**Step 4: Build to verify**

Run: `cd /Users/kevintomlinson/Coding/Osmosis && pio run`
Expected: BUILD SUCCESS

**Step 5: Commit**

```bash
git add src/card_manager.h src/card_manager.cpp src/main.cpp
git commit -m "feat: card manager with daily batch rotation and progress tracking"
```

---

## Task 11: Flashcard Screen Rendering

**Files:**
- Create: `src/card_screen.h`
- Create: `src/card_screen.cpp`
- Modify: `src/main.cpp`

**Step 1: Create src/card_screen.h**

```cpp
#pragma once
#include <TFT_eSPI.h>

namespace cardScreen {
    void render();  // Full strip-based render of the flashcard screen
}
```

**Step 2: Create src/card_screen.cpp**

Strip-based rendering following Tide Clock pattern. For each strip (0 to NUM_STRIPS-1):

1. Fill strip with background (dark gradient or solid CLR_BG_CARD)
2. **Header band** (strips 0-2, y 0-29): Draw "OSMOSIS" title and "~ Spanish ~" subtitle
3. **Image area** (strips covering y 45-164): Call `imageRenderer::drawPreloaded(IMG_X, IMG_Y, stripY)`
4. **Word box** (strips covering y 180-249): Draw rounded rect border in CLR_WORD_BOX_BORDER, fill with CLR_WORD_BOX_BG. Spanish word centered in large white font. English translation below in muted color.
5. **Card counter** (strip at y 300): "Card X / Y" in dim text

```cpp
#include "card_screen.h"
#include "display_manager.h"
#include "card_manager.h"
#include "image_renderer.h"
#include "constants.h"

void cardScreen::render() {
    TFT_eSprite& spr = display.getStrip();
    const WordEntry& word = cardMgr.currentWord();

    for (int strip = 0; strip < NUM_STRIPS; strip++) {
        int stripY = strip * STRIP_H;
        spr.fillSprite(CLR_BG_DARK);

        // Header background (y 0 to HEADER_H)
        if (stripY < HEADER_H) {
            int drawH = min(STRIP_H, HEADER_H - stripY);
            spr.fillRect(0, 0, SCREEN_W, drawH, CLR_HEADER_BG);
        }

        // Header text: "OSMOSIS" at y=4
        {
            int y = 4 - stripY;
            if (y >= -16 && y < STRIP_H) {
                spr.setTextDatum(TC_DATUM);
                spr.setTextColor(CLR_ACCENT);
                spr.drawString("OSMOSIS", SCREEN_W / 2, y, 4);
            }
        }

        // Subtitle: "~ Spanish ~" at y=20
        {
            int y = 20 - stripY;
            if (y >= -10 && y < STRIP_H) {
                spr.setTextDatum(TC_DATUM);
                spr.setTextColor(CLR_TEXT_SECONDARY);
                spr.drawString("~ Spanish ~", SCREEN_W / 2, y, 1);
            }
        }

        // Image
        imageRenderer::drawPreloaded(IMG_X, IMG_Y, stripY);

        // Word box border and fill
        {
            int boxTop = WORD_BOX_Y - stripY;
            int boxBot = boxTop + WORD_BOX_H;
            if (boxBot > 0 && boxTop < STRIP_H) {
                int drawTop = max(0, boxTop);
                int drawBot = min(STRIP_H, boxBot);
                spr.fillRect(WORD_BOX_X, drawTop, WORD_BOX_W, drawBot - drawTop, CLR_WORD_BOX_BG);
                // Top border
                if (boxTop >= 0 && boxTop < STRIP_H) spr.drawFastHLine(WORD_BOX_X, boxTop, WORD_BOX_W, CLR_WORD_BOX_BORDER);
                // Bottom border
                if (boxBot - 1 >= 0 && boxBot - 1 < STRIP_H) spr.drawFastHLine(WORD_BOX_X, boxBot - 1, WORD_BOX_W, CLR_WORD_BOX_BORDER);
                // Left border
                if (boxTop < STRIP_H && boxBot > 0) spr.drawFastVLine(WORD_BOX_X, drawTop, drawBot - drawTop, CLR_WORD_BOX_BORDER);
                // Right border
                spr.drawFastVLine(WORD_BOX_X + WORD_BOX_W - 1, drawTop, drawBot - drawTop, CLR_WORD_BOX_BORDER);
            }
        }

        // Spanish word at WORD_BOX_Y + 12
        {
            int y = WORD_BOX_Y + 12 - stripY;
            if (y >= -26 && y < STRIP_H) {
                spr.setTextDatum(TC_DATUM);
                spr.setTextColor(CLR_TEXT_PRIMARY);
                spr.drawString(word.spanish, SCREEN_W / 2, y, 6);
            }
        }

        // English word at WORD_BOX_Y + 46
        {
            int y = WORD_BOX_Y + 46 - stripY;
            if (y >= -14 && y < STRIP_H) {
                spr.setTextDatum(TC_DATUM);
                spr.setTextColor(CLR_TEXT_SECONDARY);
                spr.drawString(word.english, SCREEN_W / 2, y, 2);
            }
        }

        // Card counter at COUNTER_Y
        {
            int y = COUNTER_Y - stripY;
            if (y >= -10 && y < STRIP_H) {
                char buf[20];
                snprintf(buf, sizeof(buf), "Card %d / %d", cardMgr.currentCardIndex(), cardMgr.totalCardsToday());
                spr.setTextDatum(TC_DATUM);
                spr.setTextColor(CLR_TEXT_DIM);
                spr.drawString(buf, SCREEN_W / 2, y, 1);
            }
        }

        display.pushStrip(stripY);
    }
}
```

**Step 3: Update main.cpp loop**

In loop(), call `cardScreen::render()` at 1Hz refresh rate (or when card changes).

```cpp
static uint32_t lastRender = 0;
static bool needsRender = true;

// In loop:
cardMgr.update();  // may set needsRender
if (needsRender || millis() - lastRender > 1000) {
    cardScreen::render();
    lastRender = millis();
    needsRender = false;
}
```

**Step 4: Build to verify**

Run: `cd /Users/kevintomlinson/Coding/Osmosis && pio run`
Expected: BUILD SUCCESS

**Step 5: Commit**

```bash
git add src/card_screen.h src/card_screen.cpp src/main.cpp
git commit -m "feat: flashcard screen with strip-based rendering"
```

---

## Task 12: Settings Menu UI

**Files:**
- Create: `src/ui_settings.h`
- Create: `src/ui_settings.cpp`
- Modify: `src/main.cpp`

**Step 1: Create src/ui_settings.h**

Follow Tide Clock's SettingsScreen pattern: isActive(), show(), hide(), draw(spr, stripY), handleTap(pt).

```cpp
#pragma once
#include <TFT_eSPI.h>
#include "touch_handler.h"

class SettingsScreen {
public:
    bool isActive() const { return _active; }
    void show();
    void hide();
    void draw(TFT_eSprite& spr, int stripY);
    bool handleTap(TouchPoint pt);

private:
    bool _active = false;

    struct Button { int x, y, w, h; };

    // Words per day row (y=50)
    static constexpr Button BTN_WPD_5  = {10, 55, 40, 30};
    static constexpr Button BTN_WPD_10 = {55, 55, 40, 30};
    static constexpr Button BTN_WPD_15 = {100, 55, 40, 30};
    static constexpr Button BTN_WPD_20 = {145, 55, 40, 30};
    static constexpr Button BTN_WPD_25 = {190, 55, 40, 30};

    // Display duration row (y=120)
    static constexpr Button BTN_DUR_30  = {10, 125, 50, 30};
    static constexpr Button BTN_DUR_60  = {65, 125, 50, 30};
    static constexpr Button BTN_DUR_120 = {120, 125, 50, 30};
    static constexpr Button BTN_DUR_300 = {175, 125, 50, 30};

    // Brightness row (y=190)
    static constexpr Button BTN_BRI_LOW  = {10, 195, 68, 30};
    static constexpr Button BTN_BRI_MED  = {86, 195, 68, 30};
    static constexpr Button BTN_BRI_HIGH = {162, 195, 68, 30};

    // Back button (y=270)
    static constexpr Button BTN_BACK = {70, 270, 100, 34};

    bool hitTest(const Button& btn, TouchPoint pt);
    void drawButton(TFT_eSprite& spr, const Button& btn, int stripY,
                    const char* label, uint16_t bg, uint16_t text, bool selected);
};

extern SettingsScreen settingsUI;
```

**Step 2: Create src/ui_settings.cpp**

- `draw()`: Strip-based rendering with sections for "Words Per Day", "Display Time", "Brightness", and a "Back" button. Highlight the currently selected option for each row.
- `handleTap()`: Check hit on each button. Update settingsMgr.settings(), call settingsMgr.save(), apply brightness immediately.
- `show()`: Set _active = true. `hide()`: Set _active = false.

**Step 3: Update main.cpp**

- On GESTURE_LONG_PRESS: call `settingsUI.show()`
- In loop render: if `settingsUI.isActive()`, render settings instead of card screen
- On GESTURE_TAP while settings active: call `settingsUI.handleTap(touch.getLastTap())`

**Step 4: Build to verify**

Run: `cd /Users/kevintomlinson/Coding/Osmosis && pio run`
Expected: BUILD SUCCESS

**Step 5: Commit**

```bash
git add src/ui_settings.h src/ui_settings.cpp src/main.cpp
git commit -m "feat: settings menu with words/day, duration, and brightness"
```

---

## Task 13: Main Loop Integration

**Files:**
- Modify: `src/main.cpp`

**Step 1: Wire up the complete main loop**

Final main.cpp bringing everything together:

```cpp
#include <Arduino.h>
#include <SPIFFS.h>
#include "constants.h"
#include "display_manager.h"
#include "splash_screen.h"
#include "touch_handler.h"
#include "settings_manager.h"
#include "card_manager.h"
#include "card_screen.h"
#include "ui_settings.h"
#include "image_renderer.h"

static uint32_t lastTouchPoll = 0;
static uint32_t lastRender = 0;
static bool needsRender = true;

void setup() {
    Serial.begin(115200);
    Serial.println("Osmosis v1.0.0 booting...");

    // 1. Init display and show splash
    display.init();
    splash::show();

    // 2. Init SPIFFS for images
    if (!SPIFFS.begin(true)) {
        Serial.println("SPIFFS mount failed!");
    }

    // 3. Init touch
    touch.init();

    // 4. Load settings
    settingsMgr.init();
    display.setBrightnessLevel(settingsMgr.settings().brightness);

    // 5. Init card manager (loads progress, builds batch, preloads first image)
    cardMgr.init();

    // 6. Initial render
    needsRender = true;
}

void loop() {
    uint32_t now = millis();

    // Touch polling at 50Hz
    if (now - lastTouchPoll >= TOUCH_POLL_MS) {
        lastTouchPoll = now;
        GestureType gesture = touch.update();

        if (gesture == GESTURE_LONG_PRESS && !settingsUI.isActive()) {
            settingsUI.show();
            needsRender = true;
        } else if (gesture == GESTURE_TAP && settingsUI.isActive()) {
            settingsUI.handleTap(touch.getLastTap());
            if (!settingsUI.isActive()) {
                // Returned from settings - may need new render with updated settings
                needsRender = true;
            }
        }
    }

    // Card rotation
    if (!settingsUI.isActive()) {
        bool cardChanged = cardMgr.update();
        if (cardChanged) needsRender = true;
        cardMgr.checkDayChange();
    }

    // Render at 1Hz or on demand
    if (needsRender || now - lastRender > 1000) {
        if (settingsUI.isActive()) {
            TFT_eSprite& spr = display.getStrip();
            for (int strip = 0; strip < NUM_STRIPS; strip++) {
                int stripY = strip * STRIP_H;
                spr.fillSprite(CLR_BG_DARK);
                settingsUI.draw(spr, stripY);
                display.pushStrip(stripY);
            }
        } else {
            cardScreen::render();
        }
        lastRender = now;
        needsRender = false;
    }
}
```

**Step 2: Build to verify**

Run: `cd /Users/kevintomlinson/Coding/Osmosis && pio run`
Expected: BUILD SUCCESS

**Step 3: Commit**

```bash
git add src/main.cpp
git commit -m "feat: complete main loop with card display and settings integration"
```

---

## Task 14: Upload and Test on Hardware

**Step 1: Upload SPIFFS data**

Run: `cd /Users/kevintomlinson/Coding/Osmosis && pio run -t uploadfs`
Expected: SPIFFS image uploaded with all .bin emoji files

**Step 2: Upload firmware**

Run: `cd /Users/kevintomlinson/Coding/Osmosis && pio run -t upload`
Expected: Firmware uploaded, device reboots

**Step 3: Verify**

1. Splash screen appears with "OSMOSIS", "LEARN BY IMMERSION", "by VCodeworks LLC"
2. After 3 seconds, first flashcard appears with emoji image and word
3. Card auto-rotates after configured duration (default 1 min)
4. Long press opens settings menu
5. Settings persist after reboot

**Step 4: Final commit**

```bash
git add -A
git commit -m "feat: Osmosis v1.0.0 - Spanish vocabulary flashcard device"
```

---

## Execution Notes

- **Task order is critical** — each task builds on the previous
- **Tasks 7 and 8** (image converter + emoji download) can run in parallel with Tasks 4-6
- **Task 6** (word data) should be done carefully — the word list quality drives the whole product
- The `cardMgr.update()` return value needs to signal when the card changes so the main loop knows to re-render
- Watch SPIFFS capacity closely during Task 8 — if over budget, reduce word count or increase compression
