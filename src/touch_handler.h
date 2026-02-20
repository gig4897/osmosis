#pragma once
#include <cstdint>

enum GestureType : uint8_t {
    GESTURE_NONE,
    GESTURE_TAP,
    GESTURE_LONG_PRESS
};

struct TouchPoint {
    int16_t x;
    int16_t y;
};

class TouchHandler {
public:
    void init();
    GestureType update();  // Call at 50Hz
    TouchPoint getLastTap() const { return _lastTap; }

private:
    enum State : uint8_t { IDLE, TOUCH_DOWN };

    void* _hspi = nullptr;   // SPIClass*
    void* _ts = nullptr;     // XPT2046_Touchscreen*

    State    _state = IDLE;
    int16_t  _startX = 0, _startY = 0;
    int16_t  _lastX = 0, _lastY = 0;
    uint32_t _startTime = 0;
    bool     _longPressFired = false;
    TouchPoint _lastTap = {0, 0};

    int16_t mapX(int16_t raw);
    int16_t mapY(int16_t raw);
    bool readTouch(int16_t& x, int16_t& y);
};

extern TouchHandler touch;
