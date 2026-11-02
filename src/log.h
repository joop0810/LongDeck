#pragma once
#include "USBCDC.h"

// Serielle Konsole über USB (eigene CDC-Schnittstelle, angelegt in keys.cpp)
extern USBCDC USBSerial;
#define LOGF(...) USBSerial.printf(__VA_ARGS__)
