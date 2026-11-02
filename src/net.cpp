#include "net.h"
#include "config.h"
#include "actions.h"
#include "icons.h"
#include "log.h"
#include "web_page.h"
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include <Update.h>

volatile bool g_configChanged = false;

static WebServer server(80);
static DNSServer dns;
static Preferences prefs;

static String s_ssid, s_pass, s_webPass;
static String s_apName;
static const char *AP_PASS = "longdeck";
static bool s_apActive = false;
static uint32_t s_staStart = 0;
static bool s_mdnsStarted = false;
static bool s_wasConnected = false;
static uint32_t s_restartAt = 0;

static const char *MEDIA_NAMES[] = {
    "VOL_UP", "VOL_DOWN", "MUTE", "PLAY_PAUSE", "NEXT", "PREV", "STOP", "BRIGHT_UP",
    "BRIGHT_DOWN", "CALCULATOR", "BROWSER", "MAIL", "MY_COMPUTER", "WWW_HOME", "WWW_BACK",
    "WWW_FORWARD", "WWW_REFRESH", "WWW_SEARCH",
};

// ---------------------------------------------------------------------------
void netStartAp() {
    if (s_apActive) return;
    WiFi.mode(s_ssid.length() ? WIFI_AP_STA : WIFI_AP);
    WiFi.softAP(s_apName.c_str(), AP_PASS);
    dns.start(53, "*", WiFi.softAPIP());
    s_apActive = true;
    LOGF("[net] Einrichtungs-WLAN %s aktiv (%s)\n", s_apName.c_str(), WiFi.softAPIP().toString().c_str());
}

static void stopAp() {
    if (!s_apActive) return;
    dns.stop();
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_STA);
    s_apActive = false;
}

NetStatus netStatus() {
    NetStatus s;
    s.staConnected = WiFi.status() == WL_CONNECTED;
    s.apActive = s_apActive;
    s.ssid = s_ssid;
    s.ip = s.staConnected ? WiFi.localIP().toString() : String("");
    s.apName = s_apName;
    s.apPass = AP_PASS;
    s.apIp = s_apActive ? WiFi.softAPIP().toString() : String("");
    return s;
}

// ---------------------------------------------------------------------------
static bool authorized() {
    if (s_webPass.length() == 0) return true;
    if (server.authenticate("admin", s_webPass.c_str())) return true;
    server.requestAuthentication(BASIC_AUTH, "LongDeck");
    return false;
}

static void sendJson(int code, JsonDocument &doc) {
    String out;
    serializeJson(doc, out);
    server.send(code, "application/json", out);
}

static void sendError(int code, const String &msg) {
    JsonDocument d;
    d["error"] = msg;
    sendJson(code, d);
}

static void handleRoot() {
    if (!authorized()) return;
    server.sendHeader("Cache-Control", "no-cache");
    server.send_P(200, "text/html; charset=utf-8", WEB_PAGE);
}

static void handleGetConfig() {
    if (!authorized()) return;
    JsonDocument doc;
    xSemaphoreTakeRecursive(g_configMutex, portMAX_DELAY);
    configToJson(doc);
    xSemaphoreGiveRecursive(g_configMutex);
    sendJson(200, doc);
}

static void handlePostConfig() {
    if (!authorized()) return;
    JsonDocument doc;
    DeserializationError e = deserializeJson(doc, server.arg("plain"));
    if (e) return sendError(400, String("JSON: ") + e.c_str());
    xSemaphoreTakeRecursive(g_configMutex, portMAX_DELAY);
    String err;
    bool ok = configFromJson(doc, err);
    if (ok) ok = configSave();
    xSemaphoreGiveRecursive(g_configMutex);
    if (!ok) return sendError(400, err.length() ? err : String("Speichern fehlgeschlagen"));
    g_configChanged = true;
    JsonDocument r;
    r["ok"] = true;
    sendJson(200, r);
}

static void handleInfo() {
    if (!authorized()) return;
    JsonDocument d;
    NetStatus s = netStatus();
    d["version"] = LONGDECK_VERSION;
    d["ssid"] = s.ssid;
    d["ip"] = s.ip;
    d["connected"] = s.staConnected;
    d["ap"] = s.apActive;
    d["rssi"] = s.staConnected ? WiFi.RSSI() : 0;
    d["heap"] = ESP.getFreeHeap();
    d["psram"] = ESP.getFreePsram();
    d["webPassSet"] = s_webPass.length() > 0;
    JsonArray icons = d["icons"].to<JsonArray>();
    for (const DeckIcon &i : DECK_ICONS) icons.add(i.name);
    JsonArray media = d["media"].to<JsonArray>();
    for (const char *m : MEDIA_NAMES) media.add(m);
    sendJson(200, d);
}

static void handlePress() {
    if (!authorized()) return;
    int page = server.arg("page").toInt();
    int btn = server.arg("btn").toInt();
    actionsTrigger(page, btn);
    JsonDocument r;
    r["ok"] = true;
    sendJson(200, r);
}

static void handleScan() {
    if (!authorized()) return;
    int n = WiFi.scanNetworks(false, false);
    JsonDocument d;
    JsonArray arr = d["networks"].to<JsonArray>();
    for (int i = 0; i < n; i++) {
        String ssid = WiFi.SSID(i);
        if (ssid.length() == 0) continue;
        bool dup = false;
        for (JsonObject o : arr) {
            if (ssid == (const char *)o["ssid"]) { dup = true; break; }
        }
        if (dup) continue;
        JsonObject o = arr.add<JsonObject>();
        o["ssid"] = ssid;
        o["rssi"] = WiFi.RSSI(i);
        o["secure"] = WiFi.encryptionType(i) != WIFI_AUTH_OPEN;
    }
    WiFi.scanDelete();
    sendJson(200, d);
}

static void handleWifi() {
    if (!authorized()) return;
    JsonDocument doc;
    if (deserializeJson(doc, server.arg("plain"))) return sendError(400, "JSON ungueltig");
    String ssid = doc["ssid"] | "";
    String pass = doc["pass"] | "";
    prefs.putString("ssid", ssid);
    prefs.putString("pass", pass);
    JsonDocument r;
    r["ok"] = true;
    r["restart"] = true;
    sendJson(200, r);
    s_restartAt = millis() + 1500;
}

static void handleSecurity() {
    if (!authorized()) return;
    JsonDocument doc;
    if (deserializeJson(doc, server.arg("plain"))) return sendError(400, "JSON ungueltig");
    s_webPass = (const char *)(doc["webPass"] | "");
    prefs.putString("webpass", s_webPass);
    JsonDocument r;
    r["ok"] = true;
    sendJson(200, r);
}

static void handleReset() {
    if (!authorized()) return;
    xSemaphoreTakeRecursive(g_configMutex, portMAX_DELAY);
    configDefaults();
    configSave();
    xSemaphoreGiveRecursive(g_configMutex);
    g_configChanged = true;
    JsonDocument r;
    r["ok"] = true;
    sendJson(200, r);
}

static void handleRestart() {
    if (!authorized()) return;
    server.send(200, "application/json", "{\"ok\":true}");
    s_restartAt = millis() + 500;
}

static void handleUpdateDone() {
    if (!authorized()) return;
    bool ok = !Update.hasError();
    server.send(ok ? 200 : 500, "application/json", ok ? "{\"ok\":true}" : "{\"error\":\"Update fehlgeschlagen\"}");
    if (ok) s_restartAt = millis() + 1000;
}

static void handleUpdateUpload() {
    // Authentifizierung wird beim Start des Uploads geprüft
    HTTPUpload &up = server.upload();
    if (up.status == UPLOAD_FILE_START) {
        if (s_webPass.length() && !server.authenticate("admin", s_webPass.c_str())) return;
        LOGF("[ota] %s\n", up.filename.c_str());
        if (!Update.begin(UPDATE_SIZE_UNKNOWN, U_FLASH)) LOGF("[ota] begin fehlgeschlagen\n");
    } else if (up.status == UPLOAD_FILE_WRITE) {
        if (Update.isRunning() && Update.write(up.buf, up.currentSize) != up.currentSize) {
            LOGF("[ota] Schreibfehler\n");
        }
    } else if (up.status == UPLOAD_FILE_END) {
        if (Update.isRunning()) {
            if (Update.end(true)) LOGF("[ota] fertig: %u Bytes\n", up.totalSize);
            else LOGF("[ota] Fehler beim Abschluss\n");
        }
    } else if (up.status == UPLOAD_FILE_ABORTED) {
        Update.abort();
    }
}

static void handleNotFound() {
    if (s_apActive) {
        // Captive Portal: alle Anfragen auf die Einrichtungsseite umleiten
        server.sendHeader("Location", "http://" + WiFi.softAPIP().toString() + "/", true);
        server.send(302, "text/plain", "");
        return;
    }
    server.send(404, "text/plain", "Nicht gefunden");
}

// ---------------------------------------------------------------------------
void netBegin() {
    prefs.begin("longdeck", false);
    s_ssid = prefs.getString("ssid", "");
    s_pass = prefs.getString("pass", "");
    s_webPass = prefs.getString("webpass", "");

    uint8_t mac[6];
    WiFi.macAddress(mac);
    char apName[32];
    snprintf(apName, sizeof(apName), "LongDeck-%02X%02X", mac[4], mac[5]);
    s_apName = apName;

    WiFi.persistent(false);
    WiFi.setHostname("longdeck");
    if (s_ssid.length()) {
        WiFi.mode(WIFI_STA);
        WiFi.setAutoReconnect(true);
        WiFi.begin(s_ssid.c_str(), s_pass.c_str());
        s_staStart = millis();
        LOGF("[net] Verbinde mit %s\n", s_ssid.c_str());
    } else {
        netStartAp();
    }

    const char *headers[] = {"Authorization"};
    server.collectHeaders(headers, 1);
    server.on("/", HTTP_GET, handleRoot);
    server.on("/api/config", HTTP_GET, handleGetConfig);
    server.on("/api/config", HTTP_POST, handlePostConfig);
    server.on("/api/info", HTTP_GET, handleInfo);
    server.on("/api/press", HTTP_POST, handlePress);
    server.on("/api/scan", HTTP_GET, handleScan);
    server.on("/api/wifi", HTTP_POST, handleWifi);
    server.on("/api/security", HTTP_POST, handleSecurity);
    server.on("/api/reset", HTTP_POST, handleReset);
    server.on("/api/restart", HTTP_POST, handleRestart);
    server.on("/update", HTTP_POST, handleUpdateDone, handleUpdateUpload);
    server.onNotFound(handleNotFound);
    server.begin();
}

void netLoop() {
    server.handleClient();
    if (s_apActive) dns.processNextRequest();

    bool connected = WiFi.status() == WL_CONNECTED;
    if (connected && !s_wasConnected) {
        s_wasConnected = true;
        if (!s_mdnsStarted) {
            s_mdnsStarted = MDNS.begin("longdeck");
            if (s_mdnsStarted) MDNS.addService("http", "tcp", 80);
        }
        LOGF("[net] Verbunden, IP %s\n", WiFi.localIP().toString().c_str());
        // Einrichtungs-WLAN wird nicht mehr gebraucht
        if (s_apActive && s_ssid.length()) stopAp();
    }
    if (!connected && s_wasConnected) {
        s_wasConnected = false;
        s_staStart = millis();  // Verbindung verloren -> erneut 30 s warten
    }
    // Nach 30 s ohne Verbindung zusätzlich das Einrichtungs-WLAN öffnen
    if (!connected && !s_apActive && s_ssid.length() && millis() - s_staStart > 30000) {
        netStartAp();
    }
    if (s_restartAt && millis() > s_restartAt) {
        delay(100);
        ESP.restart();
    }
}
