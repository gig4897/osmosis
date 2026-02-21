#pragma once
#include <cstdint>

enum class WiFiState : uint8_t {
    NotConfigured,
    CaptivePortalActive,
    Connecting,
    Connected,
    Disconnected
};

namespace wifiMgr {
    void init();                  // Check NVS for saved creds, attempt connect
    void startCaptivePortal();    // Launch AP "Osmosis-Setup" with config page
    void stopCaptivePortal();
    void connect();               // Connect using saved credentials
    void disconnect();
    void update();                // Call in loop() — handles DNS, portal requests

    WiFiState state();
    bool isConnected();
    const char* ssid();           // Current/saved SSID
    int8_t rssi();                // Signal strength (when connected)
}
