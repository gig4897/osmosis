#include "wifi_manager.h"
#include "settings_manager.h"
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>

static WiFiState _state = WiFiState::NotConfigured;
static WebServer* _server = nullptr;
static DNSServer* _dns = nullptr;
static uint32_t _connectStartMs = 0;
static const uint32_t CONNECT_TIMEOUT_MS = 15000;

// Captive portal HTML — minimal form for SSID + password
static const char PORTAL_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html><head>
<meta name="viewport" content="width=device-width,initial-scale=1">
<style>
body{font-family:sans-serif;background:#1a1a2e;color:#e0e0e0;margin:0;padding:20px;text-align:center}
h1{color:#4fc3f7;margin-top:40px}
input{width:80%;padding:12px;margin:8px 0;border:1px solid #4fc3f7;border-radius:6px;background:#0d0d1a;color:#fff;font-size:16px}
button{width:84%;padding:14px;margin-top:16px;background:#4fc3f7;color:#000;border:none;border-radius:6px;font-size:18px;font-weight:bold;cursor:pointer}
.sub{color:#80b0cc;font-size:14px;margin-top:4px}
</style>
</head><body>
<h1>OSMOSIS</h1>
<p class="sub">WiFi Setup</p>
<form action="/save" method="POST">
<input name="ssid" placeholder="WiFi Network Name" required><br>
<input name="pass" type="password" placeholder="Password" required><br>
<button type="submit">Connect</button>
</form>
</body></html>
)rawliteral";

static void handleRoot() {
    _server->send(200, "text/html", PORTAL_HTML);
}

static void handleSave() {
    String ssid = _server->arg("ssid");
    String pass = _server->arg("pass");

    if (ssid.length() == 0) {
        _server->send(400, "text/html", "<h1>SSID required</h1>");
        return;
    }

    // Save to settings
    OsmosisSettings& s = settingsMgr.settings();
    strlcpy(s.wifiSSID, ssid.c_str(), sizeof(s.wifiSSID));
    strlcpy(s.wifiPass, pass.c_str(), sizeof(s.wifiPass));
    s.wifiConfigured = true;
    settingsMgr.save();

    _server->send(200, "text/html",
        "<html><body style='font-family:sans-serif;background:#1a1a2e;color:#e0e0e0;text-align:center;padding:40px'>"
        "<h1 style='color:#4fc3f7'>Saved!</h1>"
        "<p>Osmosis will now connect to your WiFi.</p>"
        "<p>You can close this page.</p>"
        "</body></html>");

    // Stop portal after short delay to let response send
    delay(1000);
    wifiMgr::stopCaptivePortal();
    wifiMgr::connect();
}

static void handleNotFound() {
    // Redirect all requests to the portal (captive portal behavior)
    _server->sendHeader("Location", "http://192.168.4.1/");
    _server->send(302, "text/plain", "");
}

namespace wifiMgr {

void init() {
    const OsmosisSettings& s = settingsMgr.settings();
    if (s.wifiConfigured && strlen(s.wifiSSID) > 0) {
        connect();
    } else {
        _state = WiFiState::NotConfigured;
        Serial.println("[wifi] No saved credentials");
    }
}

void startCaptivePortal() {
    Serial.println("[wifi] Starting captive portal...");

    WiFi.mode(WIFI_AP);
    WiFi.softAP("Osmosis-Setup");
    delay(100);

    Serial.printf("[wifi] AP IP: %s\n", WiFi.softAPIP().toString().c_str());

    // DNS server redirects all domains to our IP
    if (!_dns) _dns = new DNSServer();
    _dns->start(53, "*", WiFi.softAPIP());

    // Web server
    if (!_server) _server = new WebServer(80);
    _server->on("/", handleRoot);
    _server->on("/save", HTTP_POST, handleSave);
    _server->onNotFound(handleNotFound);
    _server->begin();

    _state = WiFiState::CaptivePortalActive;
    Serial.println("[wifi] Captive portal active at http://192.168.4.1");
}

void stopCaptivePortal() {
    if (_server) {
        _server->stop();
        delete _server;
        _server = nullptr;
    }
    if (_dns) {
        _dns->stop();
        delete _dns;
        _dns = nullptr;
    }
    WiFi.softAPdisconnect(true);
    Serial.println("[wifi] Captive portal stopped");
}

void connect() {
    const OsmosisSettings& s = settingsMgr.settings();
    if (strlen(s.wifiSSID) == 0) {
        _state = WiFiState::NotConfigured;
        return;
    }

    WiFi.mode(WIFI_STA);
    WiFi.begin(s.wifiSSID, s.wifiPass);
    _state = WiFiState::Connecting;
    _connectStartMs = millis();
    Serial.printf("[wifi] Connecting to '%s'...\n", s.wifiSSID);
}

void disconnect() {
    WiFi.disconnect(true);
    _state = WiFiState::Disconnected;
}

void update() {
    // Handle captive portal DNS + HTTP
    if (_state == WiFiState::CaptivePortalActive) {
        if (_dns) _dns->processNextRequest();
        if (_server) _server->handleClient();
        return;
    }

    // Handle connection state
    if (_state == WiFiState::Connecting) {
        if (WiFi.status() == WL_CONNECTED) {
            _state = WiFiState::Connected;
            Serial.printf("[wifi] Connected! IP: %s\n", WiFi.localIP().toString().c_str());
        } else if (millis() - _connectStartMs > CONNECT_TIMEOUT_MS) {
            _state = WiFiState::Disconnected;
            Serial.println("[wifi] Connection timeout");
        }
    }

    // Detect disconnect while connected
    if (_state == WiFiState::Connected && WiFi.status() != WL_CONNECTED) {
        _state = WiFiState::Disconnected;
        Serial.println("[wifi] Connection lost");
    }
}

WiFiState state() { return _state; }
bool isConnected() { return _state == WiFiState::Connected; }
const char* ssid() { return settingsMgr.settings().wifiSSID; }
int8_t rssi() { return WiFi.RSSI(); }

}  // namespace wifiMgr
