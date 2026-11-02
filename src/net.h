#pragma once
#include <Arduino.h>

#define LONGDECK_VERSION "1.0.0"

struct NetStatus {
    bool staConnected;
    bool apActive;
    String ssid;     // WLAN, mit dem verbunden wird
    String ip;       // IP im Heimnetz
    String apName;   // Name des Einrichtungs-WLANs
    String apPass;
    String apIp;
};

void netBegin();
void netLoop();
NetStatus netStatus();
void netStartAp();  // Einrichtungs-WLAN manuell starten

// Wird gesetzt, wenn die Konfiguration über das Web geändert wurde -> UI neu aufbauen
extern volatile bool g_configChanged;
