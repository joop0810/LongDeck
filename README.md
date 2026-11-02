# ⌨️ LongDeck

**Stream-Deck-Firmware für den [LilyGO T-Display-S3-Long](https://lilygo.cc/products/t-display-s3-long)** (ESP32-S3, 3,4″ Touch, 640 × 180).

Das Board meldet sich per USB als ganz normale Tastatur am PC an – kein Treiber, keine Zusatzsoftware. Die Tasten werden bequem im Browser eingerichtet.

![Layout](https://img.shields.io/badge/Display-640×180_quer-5ec8ff) ![USB HID](https://img.shields.io/badge/USB-HID_Tastatur-4ade80)

## Funktionen

- **Touch-Kacheln** im Raster (Standard 6 × 2, einstellbar bis 8 × 3), beliebig viele Seiten (max. 12) – Wischen wechselt die Seite
- **Aktionen pro Taste**
  - **Tastenkombination** – z. B. `CTRL+SHIFT+M`, `WIN+L`, Folgen wie `CTRL+K, CTRL+C`, Aufnahme direkt im Browser
  - **Media-Taste** – Lautstärke, Stumm, Play/Pause, Titel vor/zurück, Rechner, Browser …
  - **Text tippen** – inkl. Umlauten, Zeilenumbruch = Enter
  - **Makro** – mehrere Schritte (`key:`, `text:`, `media:`, `delay:`, `page:`)
  - **HTTP / Webhook** – z. B. Home-Assistant-Automationen auslösen (GET/POST/PUT, Body, Bearer-Token)
  - **Seite wechseln**
- **Tastaturlayout Deutsch (QWERTZ) oder US** – damit `Z`, `Y`, `@`, `ß`, `ä` usw. richtig ankommen
- **Web-Oberfläche** (läuft auf dem Gerät, auch ohne Internet): Beschriftung, Symbol, Farben, Aktion, Drag & Drop, Test-Knopf, Export/Import, Passwortschutz
- **Einrichtungs-WLAN** mit Captive Portal, danach erreichbar unter `http://longdeck.local`
- **Automatisches Dimmen**, Helligkeitsregler direkt am Gerät
- **Firmware-Update über WLAN** (OTA) oder per Web-Installer

## Installation

### Variante A: Web-Installer (empfohlen)

1. **https://joop0810.github.io/LongDeck/** in Chrome oder Edge öffnen
2. Board per USB-C anschließen → **Installieren**
3. Wird kein Gerät gefunden: **BOOT**-Taste gedrückt halten, USB einstecken, loslassen, erneut versuchen

> „Gerät löschen“ nur anhaken, wenn die Tastenbelegung zurückgesetzt werden soll.

### Variante B: Selbst bauen (PlatformIO)

```bash
git clone https://github.com/joop0810/LongDeck.git
cd LongDeck
pio run -t upload
```

Nach dem Build liegt zusätzlich `firmware/longdeck.bin` (komplettes Image ab Adresse `0x0`) im Projektordner.

## Einrichtung

1. Auf dem Display **ganz nach rechts wischen** – die Status-Seite zeigt WLAN, Adresse und USB-Status.
2. Beim ersten Start öffnet das Gerät das WLAN **`LongDeck-XXXX`** (Passwort **`longdeck`**). Verbinden → die Einrichtungsseite öffnet sich (sonst `http://192.168.4.1`).
3. Im Reiter **WLAN** das Heimnetz eintragen. Das Gerät startet neu und ist danach unter **`http://longdeck.local`** bzw. der angezeigten IP erreichbar.
4. Im Reiter **Tasten** die Belegung anpassen und **Speichern** – das Display aktualisiert sich sofort.
5. Empfohlen: im Reiter **System** ein Passwort setzen (Benutzer `admin`), sonst kann jeder im WLAN Tastendrücke auslösen.

Klappt die WLAN-Verbindung 30 Sekunden lang nicht, öffnet sich das Einrichtungs-WLAN automatisch zusätzlich. Auf der Status-Seite lässt es sich auch per Knopf starten.

## Beispiele

| Zweck | Typ | Wert |
|---|---|---|
| Mikrofon in Teams/Discord stumm | Tastenkombination | `CTRL+SHIFT+M` |
| PC sperren | Tastenkombination | `WIN+L` |
| Screenshot-Tool | Tastenkombination | `WIN+SHIFT+S` |
| VS Code: Zeile kommentieren | Tastenkombination | `CTRL+K, CTRL+C` |
| E-Mail-Signatur | Text | `Viele Grüße\nDennis` |
| Editor öffnen und schreiben | Makro | `key:WIN+R` ↵ `delay:400` ↵ `text:notepad` ↵ `key:ENTER` |
| Licht toggeln (Home Assistant) | HTTP | `POST http://homeassistant.local:8123/api/webhook/<id>` |

### Home Assistant

Am einfachsten: In Home Assistant eine Automation mit Auslöser **Webhook** anlegen, die Webhook-ID kopieren und als URL `http://<HA-IP>:8123/api/webhook/<id>` mit Methode **POST** eintragen. Ein Token ist dafür nicht nötig.

Alternativ die REST-API: URL `http://<HA-IP>:8123/api/services/light/toggle`, Body `{"entity_id":"light.wohnzimmer"}` und einen langlebigen Zugriffstoken als Bearer-Token.

### Verfügbare Tastennamen

Modifier: `CTRL` `SHIFT` `ALT` `ALTGR` `WIN` (auch `GUI`, `CMD`)
Tasten: `A`–`Z`, `0`–`9`, `F1`–`F24`, `ENTER` `ESC` `TAB` `SPACE` `BACKSPACE` `DELETE` `INSERT` `HOME` `END` `PGUP` `PGDN` `UP` `DOWN` `LEFT` `RIGHT` `PRINTSCREEN` `PAUSE` `MENU` `NUM0`–`NUM9` `NUMPLUS` `NUMMINUS` `NUMENTER` … sowie einzelne Zeichen wie `-`, `#`, `PLUS`

## Firmware-Update

- **Über WLAN:** `longdeck-ota.bin` aus dem [neuesten Release](https://github.com/joop0810/LongDeck/releases) laden → Einrichtungsseite → **System → Firmware-Update**
- **Per USB:** einfach erneut den Web-Installer verwenden (Belegung bleibt erhalten)

## Technik

- PlatformIO, Arduino-ESP32 2.0.14 (`espressif32 @ 6.5.0`), LVGL 8.3
- USB im OTG/TinyUSB-Modus: HID-Tastatur + Consumer-Control + serielle Konsole
- Display-Treiber (AXS15231B, QSPI) und Touch-Treiber (Hynitron) stammen aus den offiziellen [LilyGO-Beispielen](https://github.com/Xinyuan-LilyGO/T-Display-S3-Long); die mitgelieferte LVGL-Version ist von LilyGO für die Software-Rotation angepasst und sollte nicht aktualisiert werden
- Konfiguration als JSON in LittleFS, WLAN-Daten in NVS

```
src/
  main.cpp       Display, Touch, LVGL, Hauptschleife, Dimmen
  ui.cpp         Kachel-Oberfläche, Seiten, Status-Seite
  keys.cpp       USB-HID, Kombinationen, DE/US-Layout, Tippen
  actions.cpp    Aktions-Warteschlange (eigener Task), HTTP, Makros
  net.cpp        WLAN, Captive Portal, Web-API, OTA
  web_page.h     Einrichtungsseite
  config.cpp     Datenmodell + JSON
```

### Web-API

| Methode | Pfad | Zweck |
|---|---|---|
| GET | `/api/config` | Konfiguration lesen |
| POST | `/api/config` | Konfiguration schreiben (JSON) |
| POST | `/api/press?page=0&btn=3` | Taste fernauslösen |
| GET | `/api/info` | Status |

## Lizenz

MIT – siehe [LICENSE](LICENSE). LVGL steht unter MIT-Lizenz; Display- und Touch-Treiber stammen von LilyGO bzw. Hynitron.
