#pragma once
#include <TFT_eSPI.h>

namespace cardScreen {
    void init();        // Load smooth font data from SPIFFS (call after SPIFFS.begin + vocab load)
    void reloadFont();  // Reload font after pack switch
    void freeFont();    // Free font buffer to reclaim heap (e.g. before TLS)
    void render();      // Full strip-based render of the flashcard screen
}
