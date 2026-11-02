#!/usr/bin/env python3
"""
Erzeugt die Icon-Schriften für LongDeck aus den Tabler Icons (MIT, https://tabler.io/icons).

Voraussetzungen (einmalig, im Ordner tools/icons):
    npm i @tabler/icons-webfont@3.50.0 lv_font_conv@1.5.2

Aufruf (aus dem Projektordner):
    python3 tools/icons/gen_icons.py

Ergebnis:
    src/icon_o_XX.c / icon_f_XX.c        Schriften für das Display (Outline / Filled; 24, 32, 48 px)
    src/icons_gen.h                      Tabelle Name -> UTF-8
    web/icons.js                         Name -> Codepoint für die Web-App
"""
import os
import re
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
PKG = os.path.join(HERE, "node_modules", "@tabler", "icons-webfont", "dist")
if not os.path.isdir(PKG):
    alt = os.environ.get("TABLER_DIST")
    if alt and os.path.isdir(alt):
        PKG = alt
    else:
        sys.exit("Tabler-Paket fehlt: cd tools/icons && npm i @tabler/icons-webfont@3.50.0 lv_font_conv@1.5.2")

SIZES = [24, 32, 48]
CP_RE = re.compile(r'\.ti-([a-z0-9-]+):before\s*\{\s*content:\s*"\\([0-9a-f]+)"')


def load(cssname):
    return {n: int(cp, 16) for n, cp in CP_RE.findall(open(os.path.join(PKG, cssname)).read())}


outline = load("tabler-icons.css")
filled = load("tabler-icons-filled.css")

names = []
for line in open(os.path.join(HERE, "icons.txt"), encoding="utf-8"):
    if line.strip().startswith("#"):
        continue
    for n in line.split():
        if n not in names:
            names.append(n)

missing = [n for n in names if n not in outline]
if missing:
    sys.exit("Unbekannte Icons: " + ", ".join(missing))

# Outline- und Filled-Icons teilen sich teilweise Codepoints. Deshalb gibt es zwei
# getrennte Schriften (icon_o_XX / icon_f_XX); die Firmware wählt je Icon die passende.
# Diese gefüllten Varianten bringen lv_font_conv 1.5.2 zum Absturz (FT_Load_Glyph) – Outline-Version nutzen
FILLED_SKIP = {"home", "mouse", "calculator", "calendar"}
filled_names = [n for n in names if n in filled and n not in FILLED_SKIP]
entries = [(n, outline[n], False) for n in names]
entries += [(n + "-filled", filled[n], True) for n in filled_names]

o_cps = sorted({outline[n] for n in names})
f_cps = sorted({filled[n] for n in filled_names})

lv_font_conv = os.path.join(HERE, "node_modules", ".bin", "lv_font_conv")
if not os.path.exists(lv_font_conv):
    lv_font_conv = "lv_font_conv"

for size in SIZES:
    for kind, ttf, cps in (("o", "tabler-icons.ttf", o_cps), ("f", "tabler-icons-filled.ttf", f_cps)):
        rng = ",".join(hex(c) for c in cps)
        out = os.path.join(ROOT, "src", "icon_%s_%d.c" % (kind, size))
        cmd = [lv_font_conv, "--no-compress", "--bpp", "4", "--size", str(size),
               "--font", os.path.join(PKG, "fonts", ttf), "-r", rng,
               "--format", "lvgl", "--lv-include", "lvgl.h", "--no-kerning", "-o", out]
        subprocess.run(cmd, check=True)
        src = open(out, encoding="utf-8").read()
        src = src.replace(PKG + "/", "tabler/").replace(" -r " + rng, " -r <siehe icons.txt>")
        src = src.replace(out, os.path.relpath(out, ROOT))
        open(out, "w", encoding="utf-8").write(src)
        print("geschrieben:", os.path.relpath(out, ROOT))


def utf8_literal(cp):
    return "".join("\\x%02X" % b for b in chr(cp).encode("utf-8"))


with open(os.path.join(ROOT, "src", "icons_gen.h"), "w", encoding="utf-8") as h:
    h.write("// Automatisch erzeugt von tools/icons/gen_icons.py – nicht von Hand bearbeiten.\n")
    h.write("// Tabler Icons (MIT) – https://tabler.io/icons\n#pragma once\n\n")
    h.write("struct DeckIcon { const char *name; const char *glyph; bool filled; };\n\n")
    h.write("static const DeckIcon DECK_ICONS[] = {\n")
    for n, cp, f in entries:
        h.write('    {"%s", "%s", %s},\n' % (n, utf8_literal(cp), "true" if f else "false"))
    h.write("};\n")
    h.write("static const size_t DECK_ICON_COUNT = sizeof(DECK_ICONS) / sizeof(DECK_ICONS[0]);\n")
print("geschrieben: src/icons_gen.h (%d Icons)" % len(entries))

with open(os.path.join(ROOT, "web", "icons.js"), "w", encoding="utf-8") as js:
    js.write("// Automatisch erzeugt von tools/icons/gen_icons.py\n")
    # Web-App: Original-Codepoint + Schrift (o = Outline, f = Filled)
    js.write("const ICON_CP={%s};\n" % ",".join('"%s":[%d,"%s"]' % (n, wcp, "f" if f else "o") for n, wcp, f in entries))
print("geschrieben: web/icons.js")
