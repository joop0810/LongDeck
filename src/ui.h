#pragma once
#include <Arduino.h>

void uiBuild();                 // (Neu-)Aufbau aller Seiten aus g_config
void uiGotoPage(int page);
void uiToast(const char *msg, bool ok);
void uiFlashButton(int page, int button, bool ok);
void uiUpdateInfo();            // Status-Seite aktualisieren (1x pro Sekunde)
