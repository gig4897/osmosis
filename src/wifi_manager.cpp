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

// --- WiFi scan results stored as pre-built HTML <option> tags + JSON ---
static String _scanOptionsHtml;   // "<option>...</option>..." for dropdown
static String _scanJson;          // JSON array for /scan endpoint

static void doWifiScan() {
    Serial.println("[wifi] Scanning for networks...");
    WiFi.mode(WIFI_STA);
    delay(100);
    int n = WiFi.scanNetworks();
    Serial.printf("[wifi] Found %d networks\n", n);

    _scanOptionsHtml = "";
    _scanJson = "[";

    // Deduplicate and sort by RSSI (strongest first).
    // WiFi.scanNetworks() already returns sorted, but there may be duplicates.
    struct Net { String ssid; int32_t rssi; };
    Net nets[16];
    int count = 0;

    for (int i = 0; i < n && count < 16; i++) {
        String s = WiFi.SSID(i);
        if (s.length() == 0) continue;  // skip hidden networks

        // Check for duplicate
        bool dup = false;
        for (int j = 0; j < count; j++) {
            if (nets[j].ssid == s) { dup = true; break; }
        }
        if (dup) continue;

        nets[count].ssid = s;
        nets[count].rssi = WiFi.RSSI(i);
        count++;
    }

    for (int i = 0; i < count; i++) {
        // Build HTML options
        _scanOptionsHtml += "<option value=\"";
        // Escape quotes in SSID for HTML attribute safety
        String escaped = nets[i].ssid;
        escaped.replace("\"", "&quot;");
        escaped.replace("<", "&lt;");
        _scanOptionsHtml += escaped;
        _scanOptionsHtml += "\">";
        _scanOptionsHtml += escaped;

        // Signal strength indicator
        int bars = 1;
        if (nets[i].rssi > -50) bars = 4;
        else if (nets[i].rssi > -60) bars = 3;
        else if (nets[i].rssi > -70) bars = 2;
        char sig[16];
        snprintf(sig, sizeof(sig), " (%ddBm)", (int)nets[i].rssi);
        _scanOptionsHtml += sig;
        _scanOptionsHtml += "</option>";

        // Build JSON
        if (i > 0) _scanJson += ",";
        _scanJson += "{\"ssid\":\"";
        // Escape quotes for JSON
        String jsonEsc = nets[i].ssid;
        jsonEsc.replace("\\", "\\\\");
        jsonEsc.replace("\"", "\\\"");
        _scanJson += jsonEsc;
        _scanJson += "\",\"rssi\":";
        _scanJson += String((int)nets[i].rssi);
        _scanJson += "}";
    }

    _scanJson += "]";
    WiFi.scanDelete();
}

// --- Build portal HTML dynamically (embeds scan results) ---
static String buildPortalHtml() {
    String html;
    html.reserve(3500);

    html += F("<!DOCTYPE html><html><head>"
        "<meta name='viewport' content='width=device-width,initial-scale=1'>"
        "<style>"
        "*{box-sizing:border-box;margin:0;padding:0}"
        "body{font-family:-apple-system,sans-serif;background:#0a0a1a;color:#e0e0e0;"
        "min-height:100vh;display:flex;flex-direction:column;align-items:center;padding:24px 16px}"
        "h1{font-size:32px;font-weight:800;letter-spacing:6px;margin:28px 0 4px;"
        "background:linear-gradient(135deg,#4e7fff,#7b9fff);-webkit-background-clip:text;"
        "-webkit-text-fill-color:transparent;text-shadow:0 0 40px rgba(78,127,255,.4)}"
        ".sub{color:#808080;font-size:13px;margin-bottom:24px}"
        ".card{background:#1a1a2e;border:1px solid #2a2a4e;border-radius:12px;"
        "padding:20px;width:100%;max-width:320px}"
        "label{display:block;font-size:13px;color:#808080;margin:12px 0 4px}"
        "label:first-child{margin-top:0}"
        "select,input[type=text],input[type=password]{width:100%;padding:10px 12px;"
        "background:#0a0a1a;border:1px solid #2a2a4e;border-radius:8px;color:#e0e0e0;"
        "font-size:15px;outline:none;transition:border .2s}"
        "select:focus,input:focus{border-color:#4e7fff}"
        ".or{text-align:center;color:#808080;font-size:12px;margin:8px 0}"
        "button{width:100%;padding:12px;margin-top:16px;background:#4e7fff;color:#fff;"
        "border:none;border-radius:8px;font-size:16px;font-weight:600;cursor:pointer;"
        "transition:background .2s}"
        "button:active{background:#3a6ae0}"
        ".foot{margin-top:auto;padding-top:24px;text-align:center;font-size:12px;color:#808080}"
        ".foot a{color:#4e7fff;text-decoration:none}"
        "</style></head><body>"
        "<h1>OSMOSIS</h1>"
        "<p class='sub'>WiFi Setup</p>"
        "<div class='card'>"
        "<form action='/save' method='POST'>"
        "<label>Select a network</label>"
        "<select name='ssid_sel' id='sel'>"
        "<option value=''>-- Choose --</option>");

    html += _scanOptionsHtml;

    html += F("</select>"
        "<div class='or'>or enter manually</div>"
        "<input type='text' name='ssid_manual' id='man' placeholder='Network name (SSID)'>"
        "<label>Password</label>"
        "<input type='password' name='pass' placeholder='WiFi password'>"
        "<input type='hidden' name='ssid' id='ssid_val'>"
        "<button type='submit'>Connect</button>"
        "</form></div>"
        "<div class='foot'>Visit <a href='https://vcodeworks.dev'>vcodeworks.dev</a> for more info</div>"
        "<script>"
        "document.querySelector('form').onsubmit=function(){"
        "var s=document.getElementById('sel').value;"
        "var m=document.getElementById('man').value.trim();"
        "document.getElementById('ssid_val').value=m||s;"
        "if(!m&&!s){alert('Select or enter a network');return false;}"
        "return true;};"
        "</script>"
        "</body></html>");

    return html;
}

static void handleRoot() {
    String html = buildPortalHtml();
    _server->send(200, "text/html", html);
}

static void handleScan() {
    // Re-scan networks and return JSON
    // Use AP_STA so we can scan while AP is running
    WiFi.mode(WIFI_AP_STA);
    delay(50);
    int n = WiFi.scanNetworks();
    String json = "[";
    int count = 0;
    for (int i = 0; i < n && count < 16; i++) {
        String s = WiFi.SSID(i);
        if (s.length() == 0) continue;
        // Deduplicate
        bool dup = false;
        // Simple check: scan earlier entries in this loop
        for (int j = 0; j < i; j++) {
            if (WiFi.SSID(j) == s) { dup = true; break; }
        }
        if (dup) continue;

        if (count > 0) json += ",";
        json += "{\"ssid\":\"";
        String esc = s;
        esc.replace("\\", "\\\\");
        esc.replace("\"", "\\\"");
        json += esc;
        json += "\",\"rssi\":";
        json += String((int)WiFi.RSSI(i));
        json += "}";
        count++;
    }
    json += "]";
    WiFi.scanDelete();
    _server->send(200, "application/json", json);
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
        "<!DOCTYPE html><html><head>"
        "<meta name='viewport' content='width=device-width,initial-scale=1'>"
        "<style>"
        "body{font-family:-apple-system,sans-serif;background:#0a0a1a;color:#e0e0e0;"
        "text-align:center;padding:60px 20px}"
        "h1{font-size:28px;color:#4e7fff;margin-bottom:12px}"
        "p{color:#808080;font-size:14px}"
        "</style></head><body>"
        "<h1>Saved!</h1>"
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

    // Scan for networks in STA mode first (best results)
    doWifiScan();

    // Now switch to AP mode
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
    _server->on("/scan", HTTP_GET, handleScan);
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

    // Free scan result memory
    _scanOptionsHtml = String();
    _scanJson = String();

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
