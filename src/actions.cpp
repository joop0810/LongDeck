#include "actions.h"
#include "keys.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

struct ActionJob {
    int8_t page;
    int8_t button;
    DeckButton btn;
    String layout;
    uint16_t typeDelay;
};

static QueueHandle_t s_jobs = nullptr;
static QueueHandle_t s_results = nullptr;

static void postResult(const ActionJob &job, bool ok, const String &msg, int gotoPage = -1) {
    ActionResult r;
    r.page = job.page;
    r.button = job.button;
    r.ok = ok;
    r.gotoPage = gotoPage;
    strlcpy(r.msg, msg.c_str(), sizeof(r.msg));
    xQueueSend(s_results, &r, 0);
}

static bool runHttp(const DeckButton &b, String &msg) {
    if (WiFi.status() != WL_CONNECTED) { msg = "Kein WLAN"; return false; }
    if (b.value.length() == 0) { msg = "Keine URL"; return false; }

    HTTPClient http;
    WiFiClientSecure secure;
    WiFiClient plain;
    bool ok;
    if (b.value.startsWith("https://")) {
        secure.setInsecure();  // kein Zertifikats-Pinning auf dem Mikrocontroller
        ok = http.begin(secure, b.value);
    } else {
        ok = http.begin(plain, b.value);
    }
    if (!ok) { msg = "URL ungueltig"; return false; }
    http.setTimeout(8000);
    http.setConnectTimeout(5000);
    if (b.auth.length()) http.addHeader("Authorization", "Bearer " + b.auth);

    String method = b.method;
    method.toUpperCase();
    if (method.length() == 0) method = "POST";
    int code;
    if (method == "GET") {
        code = http.GET();
    } else {
        http.addHeader("Content-Type", b.contentType.length() ? b.contentType : String("application/json"));
        code = http.sendRequest(method.c_str(), b.body);
    }
    http.end();
    if (code <= 0) { msg = "HTTP-Fehler " + String(code); return false; }
    msg = "HTTP " + String(code);
    return code >= 200 && code < 300;
}

// Makro: eine Zeile pro Schritt
//   key:CTRL+C       Tastenkombination(en)
//   text:Hallo       Text tippen (\n am Ende über eigene Zeile "key:ENTER")
//   media:VOL_UP     Media-Taste
//   delay:500        Warten in ms
//   page:2           Seite wechseln
static bool runMacro(const ActionJob &job, String &msg, int &gotoPage) {
    const String &src = job.btn.value;
    int start = 0;
    while (start < (int)src.length()) {
        int nl = src.indexOf('\n', start);
        String line = nl < 0 ? src.substring(start) : src.substring(start, nl);
        start = nl < 0 ? src.length() : nl + 1;
        line.replace("\r", "");
        if (line.length() == 0) continue;
        int colon = line.indexOf(':');
        if (colon < 0) { msg = "Zeile ohne ':'"; return false; }
        String cmd = line.substring(0, colon);
        String arg = line.substring(colon + 1);
        cmd.trim();
        cmd.toLowerCase();
        String err;
        if (cmd == "key") {
            if (!keysSendSequence(arg, job.layout, err)) { msg = err; return false; }
        } else if (cmd == "text") {
            keysType(arg, job.layout, job.typeDelay);
        } else if (cmd == "media") {
            if (!keysMedia(arg)) { msg = "Media? " + arg; return false; }
        } else if (cmd == "delay") {
            delay(constrain(arg.toInt(), 0, 10000));
        } else if (cmd == "page") {
            gotoPage = arg.toInt() - 1;
        } else {
            msg = "Unbekannt: " + cmd;
            return false;
        }
        delay(20);
    }
    msg = "OK";
    return true;
}

static void worker(void *) {
    ActionJob *job;
    for (;;) {
        if (xQueueReceive(s_jobs, &job, portMAX_DELAY) != pdTRUE) continue;
        const DeckButton &b = job->btn;
        String msg, err;
        bool ok = true;
        int gotoPage = -1;
        bool needsUsb = b.type == ACT_HOTKEY || b.type == ACT_MEDIA || b.type == ACT_TEXT || b.type == ACT_MACRO;
        if (needsUsb && !keysUsbReady()) {
            postResult(*job, false, "USB nicht verbunden");
            delete job;
            continue;
        }
        switch (b.type) {
            case ACT_HOTKEY:
                ok = keysSendSequence(b.value, job->layout, err);
                msg = ok ? "" : err;
                break;
            case ACT_MEDIA:
                ok = keysMedia(b.value);
                if (!ok) msg = "Unbekannte Media-Taste";
                break;
            case ACT_TEXT:
                keysType(b.value, job->layout, job->typeDelay);
                break;
            case ACT_HTTP:
                ok = runHttp(b, msg);
                break;
            case ACT_PAGE:
                gotoPage = b.value.toInt() - 1;
                break;
            case ACT_MACRO:
                ok = runMacro(*job, msg, gotoPage);
                if (ok) msg = "";
                break;
            default:
                break;
        }
        postResult(*job, ok, msg, gotoPage);
        delete job;
    }
}

void actionsBegin() {
    s_jobs = xQueueCreate(8, sizeof(ActionJob *));
    s_results = xQueueCreate(8, sizeof(ActionResult));
    xTaskCreatePinnedToCore(worker, "actions", 8192, nullptr, 2, nullptr, 0);
}

void actionsTrigger(int page, int button) {
    ActionJob *job = new ActionJob();
    xSemaphoreTakeRecursive(g_configMutex, portMAX_DELAY);
    bool valid = page >= 0 && page < (int)g_config.pages.size() &&
                 button >= 0 && button < (int)g_config.pages[page].buttons.size();
    if (valid) {
        job->btn = g_config.pages[page].buttons[button];
        job->layout = g_config.layout;
        job->typeDelay = g_config.typeDelay;
    }
    xSemaphoreGiveRecursive(g_configMutex);
    if (!valid || job->btn.type == ACT_NONE) {
        delete job;
        return;
    }
    job->page = page;
    job->button = button;
    if (xQueueSend(s_jobs, &job, 0) != pdTRUE) delete job;  // Warteschlange voll -> verwerfen
}

bool actionsPollResult(ActionResult &out) {
    return s_results && xQueueReceive(s_results, &out, 0) == pdTRUE;
}
