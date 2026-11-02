# ⌨️ LongDeck

**Stream-Deck-Firmware für den [LilyGO T-Display-S3-Long](https://lilygo.cc/products/t-display-s3-long)** (ESP32-S3, 3,4″ Touch, 640 × 180).

Nur USB – kein WLAN, keine Hintergrund-App. Das Board meldet sich am PC als ganz normale Tastatur an. Belegt werden die Tasten über eine Web-App, die per USB direkt mit dem Gerät spricht (Web Serial). Die Belegung wird auf dem Gerät gespeichert; danach funktioniert es an jedem PC ohne Software.

**👉 Web-App & Installer: https://joop0810.github.io/LongDeck/**

## Funktionen

- **Touch-Kacheln** im Raster (Standard 6 × 2, einstellbar bis 8 × 3), bis zu 12 Seiten – Wischen wechselt die Seite
- **Aktionen pro Taste**
  - **Tastenkombination** – z. B. `CTRL+SHIFT+M`, `WIN+L`, Folgen wie `CTRL+K, CTRL+C`; Aufnahme direkt in der Web-App
  - **Media-Taste** – Lautstärke, Stumm, Play/Pause, Titel vor/zurück …
  - **Text tippen** – inkl. Umlauten, Zeilenumbruch = Enter
  - **Programm / Ordner / Webseite öffnen** – mit Vorlagen (Rechner, Explorer, Einstellungen …)
  - **Webhook / URL aufrufen** – z. B. Home-Assistant-Automationen
  - **Makro** – mehrere Schritte (`key:`, `text:`, `media:`, `open:`, `delay:`, `page:`)
  - **Seite wechseln**
- **Tastaturlayout Deutsch (QWERTZ) oder US** – damit `Z`, `Y`, `@`, `\`, `ä` usw. richtig ankommen
- **Web-App**: Beschriftung, Symbol, Farben, Aktion, Drag & Drop, Kopieren/Einfügen, „Am PC testen“, Export/Import, Demo-Modus ohne Gerät
- **Automatisches Dimmen**, Helligkeitsregler direkt am Gerät

## Installation

1. **https://joop0810.github.io/LongDeck/** in **Chrome oder Edge** öffnen
2. Board per USB-C anschließen → Reiter **Firmware** → **Installieren**
3. Wird kein Gerät gefunden: **BOOT**-Taste gedrückt halten, USB einstecken, loslassen, erneut versuchen
4. Danach einmal ab- und wieder anstecken

> „Gerät löschen“ nur anhaken, wenn die Tastenbelegung zurückgesetzt werden soll.

**Selbst bauen (PlatformIO):** `pio run -t upload` – nach dem Build liegt zusätzlich `firmware/longdeck.bin` (komplettes Image ab `0x0`) im Projektordner.

## Tasten belegen

1. Web-App öffnen → **Verbinden** → Gerät auswählen
2. Taste anklicken, Aktion einstellen
3. **Aufs Gerät speichern** (oder `Strg+S`) – das Display aktualisiert sich sofort

Die letzte Seite auf dem Display (ganz nach rechts wischen) zeigt Version, USB-Status, Helligkeitsregler und die Adresse der Web-App.

## Wie funktionieren „Öffnen“ und „Webhook“ ohne App?

Das Gerät drückt `Win+R`, tippt den Befehl ins Ausführen-Fenster und bestätigt mit Enter. Dadurch geht alles, was im Ausführen-Fenster funktioniert – Programme, Ordner, `https://`-Adressen, `steam://`-Links, `ms-settings:` usw.

Webhooks werden dabei als unsichtbarer PowerShell-Aufruf ausgeführt (`iwr …`). Das Ausführen-Fenster blitzt kurz auf, ansonsten passiert alles im Hintergrund. Einschränkungen:

- Funktioniert unter **Windows**, wenn der PC entsperrt ist
- Das Ausführen-Fenster nimmt max. **259 Zeichen** – die Web-App zeigt die Länge live an
- Bei aktivem Programm mit Admin-Rechten kann Windows Eingaben ins Ausführen-Fenster blockieren (selten)

### Home Assistant

Am einfachsten: In Home Assistant eine Automation mit Auslöser **Webhook** anlegen, die Webhook-ID kopieren und als URL `http://<HA-IP>:8123/api/webhook/<id>` mit Methode **POST** eintragen. Kein Token nötig – das hält den Befehl kurz.

## Beispiele

| Zweck | Typ | Wert |
|---|---|---|
| Mikrofon in Teams/Discord stumm | Tastenkombination | `CTRL+SHIFT+M` |
| PC sperren | Tastenkombination | `WIN+L` |
| Screenshot-Tool | Tastenkombination | `WIN+SHIFT+S` |
| VS Code: Zeile kommentieren | Tastenkombination | `CTRL+K, CTRL+C` |
| OBS starten | Öffnen | `C:\Program Files\obs-studio\bin\64bit\obs64.exe` |
| Sound-Einstellungen | Öffnen | `ms-settings:sound` |
| Licht toggeln | Webhook | `POST http://homeassistant.local:8123/api/webhook/licht` |
| Markieren, kopieren, in Editor einfügen | Makro | `key:CTRL+A` ↵ `key:CTRL+C` ↵ `open:notepad` ↵ `delay:600` ↵ `key:CTRL+V` |

### Tastennamen

Modifier: `CTRL` `SHIFT` `ALT` `ALTGR` `WIN` (auch `GUI`, `CMD`)
Tasten: `A`–`Z`, `0`–`9`, `F1`–`F24`, `ENTER` `ESC` `TAB` `SPACE` `BACKSPACE` `DELETE` `INSERT` `HOME` `END` `PGUP` `PGDN` `UP` `DOWN` `LEFT` `RIGHT` `PRINTSCREEN` `PAUSE` `MENU` `NUM0`–`NUM9` `NUMPLUS` `NUMMINUS` `NUMENTER` … sowie einzelne Zeichen wie `-`, `#`, `PLUS`

## Technik

- PlatformIO, Arduino-ESP32 2.0.14 (`espressif32 @ 6.5.0`), LVGL 8.3
- USB im OTG/TinyUSB-Modus: HID-Tastatur + Consumer-Control + serielle Schnittstelle (Konfiguration)
- WLAN und Bluetooth werden nie eingeschaltet
- Display-Treiber (AXS15231B, QSPI) und Touch-Treiber (Hynitron) aus den offiziellen [LilyGO-Beispielen](https://github.com/Xinyuan-LilyGO/T-Display-S3-Long); die mitgelieferte LVGL-Version ist von LilyGO für die Software-Rotation angepasst und sollte nicht aktualisiert werden
- Konfiguration als JSON in LittleFS

```
src/
  main.cpp       Display, Touch, LVGL, Hauptschleife, Dimmen
  ui.cpp         Kachel-Oberfläche, Seiten, Status-Seite
  keys.cpp       USB-HID, Kombinationen, DE/US-Layout, Tippen
  actions.cpp    Aktions-Warteschlange (eigener Task), Öffnen/Webhook über Win+R, Makros
  protocol.cpp   Konfiguration über USB-Seriell (JSON-Zeilen)
  config.cpp     Datenmodell + JSON
web/
  index.html     Web-App (Editor + Installer), wird per GitHub Pages veröffentlicht
```

### USB-Protokoll

115200 Baud, eine JSON-Zeile pro Nachricht, Antworten mit derselben `id`:

| Anfrage | Antwort |
|---|---|
| `{"id":1,"cmd":"hello"}` | Version, Symbol- und Media-Namen |
| `{"id":2,"cmd":"get"}` | `{"config":{…}}` |
| `{"id":3,"cmd":"set","config":{…}}` | speichert und aktualisiert das Display |
| `{"id":4,"cmd":"press","page":0,"btn":3}` | löst eine Taste aus |
| `reset` / `reboot` / `bootloader` / `ping` | |

## Lizenz

MIT – siehe [LICENSE](LICENSE). LVGL steht unter MIT-Lizenz; Display- und Touch-Treiber stammen von LilyGO bzw. Hynitron.
