#pragma once
#include <cstdint>

namespace imageRenderer {
    void init();                              // Pre-allocate image buffer (call early before heap fragments)
    void freeBuffer();                        // Free image buffer to reclaim heap (e.g. before TLS)
    bool preloadImage(const char* filename);  // Load + decompress from SPIFFS into RAM buffer
    void drawPreloaded(int x, int y, int stripY);  // Draw relevant rows into current strip
}
