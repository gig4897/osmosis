#pragma once
#include <cstdint>

namespace imageRenderer {
    bool preloadImage(const char* filename);  // Load + decompress from SPIFFS into RAM buffer
    void drawPreloaded(int x, int y, int stripY);  // Draw relevant rows into current strip
}
