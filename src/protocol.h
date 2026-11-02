#pragma once
#include <Arduino.h>

// Konfigurations-Protokoll über die serielle USB-Schnittstelle (für die Web-App via Web Serial).
// Eine Anfrage pro Zeile als JSON: {"id":1,"cmd":"get"}  ->  {"id":1,"ok":true,"config":{...}}
void protocolBegin();
void protocolLoop();
bool protocolEditorConnected();  // Web-App hat sich in den letzten Sekunden gemeldet
