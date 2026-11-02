#pragma once
#include <Arduino.h>
#include "config.h"

// Ergebnis einer ausgeführten Aktion (für die Anzeige auf dem Display)
struct ActionResult {
    int8_t page;
    int8_t button;
    bool ok;
    int16_t gotoPage;  // >= 0: UI soll auf diese Seite springen
    char msg[48];
};

void actionsBegin();
// Stellt die Aktion einer Taste in die Warteschlange (Kopie der Taste wird gemacht)
void actionsTrigger(int page, int button);
// Holt ein Ergebnis ab (nicht blockierend)
bool actionsPollResult(ActionResult &out);
