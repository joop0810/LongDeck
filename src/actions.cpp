#include "actions.h"
#include "keys.h"

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

// Max. Länge, die das Windows-Ausführen-Fenster (Win+R) annimmt
static const size_t RUN_MAX = 259;

// Öffnet Win+R, tippt den Befehl und bestätigt mit Enter
static bool runDialog(const String &cmd, const ActionJob &job, String &msg) {
    if (cmd.length() == 0) { msg = "Nichts zum Ausfuehren"; return false; }
    if (cmd.length() > RUN_MAX) { msg = "Befehl zu lang (" + String(cmd.length()) + ">259)"; return false; }
    String err;
    if (!keysSendCombo("GUI+R", job.layout, err)) { msg = err; return false; }
    delay(550);  // Fenster öffnen lassen
    keysType(cmd, job.layout, job.typeDelay);
    delay(60);
    keysSendCombo("ENTER", job.layout, err);
    return true;
}

// Für PowerShell-Strings in einfachen Anführungszeichen: ' -> ''  und  " -> \" (Kommandozeile)
static String psQuote(const String &in) {
    String s = in;
    s.replace("'", "''");
    s.replace("\"", "\\\"");
    s.replace("\r", "");
    s.replace("\n", " ");
    return "'" + s + "'";
}

// Baut den PowerShell-Befehl für einen Webhook. Muss identisch zur Web-App sein (buildWebhookCmd).
String buildWebhookCommand(const DeckButton &b) {
    String method = b.method;
    method.toUpperCase();
    if (method.length() == 0) method = "POST";
    String c = "powershell -w hidden -nop -c \"iwr -useb -Me " + method + " " + psQuote(b.value);
    if (b.body.length()) {
        c += " -ContentType " + psQuote(b.contentType.length() ? b.contentType : String("application/json"));
        c += " -Body " + psQuote(b.body);
    }
    if (b.auth.length()) c += " -H @{Authorization=" + psQuote("Bearer " + b.auth) + "}";
    c += "\"";
    return c;
}

// Makro: eine Zeile pro Schritt
//   key:CTRL+C       Tastenkombination(en)
//   text:Hallo       Text tippen (\n am Ende über eigene Zeile "key:ENTER")
//   media:VOL_UP     Media-Taste
//   delay:500        Warten in ms
//   page:2           Seite wechseln
//   open:notepad     Programm/Datei/Ordner/URL öffnen (Win+R)
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
        } else if (cmd == "open") {
            arg.trim();
            if (!runDialog(arg, job, msg)) return false;
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
        bool needsUsb = b.type != ACT_PAGE;
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
                if (b.value.length() == 0) { ok = false; msg = "Keine URL"; break; }
                ok = runDialog(buildWebhookCommand(b), *job, msg);
                break;
            case ACT_OPEN:
                ok = runDialog(b.value, *job, msg);
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
