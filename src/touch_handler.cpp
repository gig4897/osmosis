#include "touch_handler.h"
#include <Arduino.h>
#include <SPI.h>
#include <XPT2046_Touchscreen.h>
#include "constants.h"

TouchHandler touch;

void TouchHandler::init() {
    SPIClass* hspi = new SPIClass(HSPI);
    hspi->begin(PIN_TOUCH_CLK, PIN_TOUCH_MISO, PIN_TOUCH_MOSI, PIN_TOUCH_CS);
    _hspi = hspi;

    XPT2046_Touchscreen* ts = new XPT2046_Touchscreen(PIN_TOUCH_CS);
    ts->begin(*hspi);
    ts->setRotation(0);  // Portrait orientation
    _ts = ts;
}

int16_t TouchHandler::mapX(int16_t raw) {
    int16_t mapped = map(raw, TOUCH_X_MIN, TOUCH_X_MAX, 0, SCREEN_W);
    if (mapped < 0) mapped = 0;
    if (mapped >= SCREEN_W) mapped = SCREEN_W - 1;
    return mapped;
}

int16_t TouchHandler::mapY(int16_t raw) {
    int16_t mapped = map(raw, TOUCH_Y_MIN, TOUCH_Y_MAX, 0, SCREEN_H);
    if (mapped < 0) mapped = 0;
    if (mapped >= SCREEN_H) mapped = SCREEN_H - 1;
    return mapped;
}

bool TouchHandler::readTouch(int16_t& x, int16_t& y) {
    XPT2046_Touchscreen* ts = static_cast<XPT2046_Touchscreen*>(_ts);
    if (!ts->touched()) return false;

    // Average 3 readings to reduce jitter
    int32_t sumX = 0, sumY = 0;
    int count = 0;
    for (int i = 0; i < 3; i++) {
        if (ts->touched()) {
            TS_Point p = ts->getPoint();
            sumX += p.x;
            sumY += p.y;
            count++;
        }
    }
    if (count == 0) return false;

    x = mapX(sumX / count);
    y = mapY(sumY / count);
    return true;
}

GestureType TouchHandler::update() {
    int16_t sx, sy;
    bool touching = readTouch(sx, sy);

    switch (_state) {
    case IDLE:
        if (touching) {
            _state = TOUCH_DOWN;
            _startX = _lastX = sx;
            _startY = _lastY = sy;
            _startTime = millis();
            _longPressFired = false;
        }
        return GESTURE_NONE;

    case TOUCH_DOWN:
        if (!touching) {
            // Released -- determine gesture
            _state = IDLE;
            if (_longPressFired) return GESTURE_NONE;

            uint32_t duration = millis() - _startTime;
            int16_t absDx = abs(_lastX - _startX);
            int16_t absDy = abs(_lastY - _startY);

            // Tap: short duration, small movement
            if (duration < LONG_PRESS_MS && absDx < 40 && absDy < 40) {
                _lastTap = {_startX, _startY};
                return GESTURE_TAP;
            }

            return GESTURE_NONE;
        }

        // Still touching -- update position
        _lastX = sx;
        _lastY = sy;

        // Check for long press: held > 2s with < 20px movement
        if (!_longPressFired
            && (millis() - _startTime) > LONG_PRESS_MS
            && abs(_lastX - _startX) < 20
            && abs(_lastY - _startY) < 20) {
            _longPressFired = true;
            return GESTURE_LONG_PRESS;
        }
        return GESTURE_NONE;
    }

    return GESTURE_NONE;
}
