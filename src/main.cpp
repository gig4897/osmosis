#include <Arduino.h>
#include "constants.h"
#include "display_manager.h"
#include "splash_screen.h"

void setup() {
    Serial.begin(115200);
    Serial.println("Osmosis v1.0.0 booting...");

    display.init();
    splash::show();
}

void loop() {
}
