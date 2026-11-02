#pragma once
#include <pgmspace.h>

// Einrichtungsseite (wird vom Gerät ausgeliefert, funktioniert ohne Internet)
static const char WEB_PAGE[] PROGMEM = R"rawliteral(<!doctype html>
<html lang="de">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>LongDeck</title>
<style>
:root{--bg:#0d0f13;--panel:#161a21;--panel2:#1d222b;--line:#2a303b;--text:#e8ebf1;--muted:#8a91a0;--accent:#5ec8ff;--danger:#ff5d65;--ok:#4ade80}
*{box-sizing:border-box}
body{margin:0;background:var(--bg);color:var(--text);font:15px/1.45 system-ui,-apple-system,Segoe UI,Roboto,sans-serif}
header{display:flex;align-items:center;gap:14px;padding:14px 20px;border-bottom:1px solid var(--line);position:sticky;top:0;background:var(--bg);z-index:5;flex-wrap:wrap}
header h1{font-size:19px;margin:0;letter-spacing:.3px}
header .status{color:var(--muted);font-size:13px;flex:1}
nav{display:flex;gap:4px}
nav button{background:none;border:0;color:var(--muted);padding:8px 12px;border-radius:8px;cursor:pointer;font:inherit}
nav button.on{background:var(--panel2);color:var(--text)}
main{max-width:1100px;margin:0 auto;padding:20px 16px 90px}
section{display:none}section.on{display:block}
.card{background:var(--panel);border:1px solid var(--line);border-radius:14px;padding:18px;margin-bottom:16px}
.card h2{font-size:15px;margin:0 0 12px;color:var(--muted);font-weight:600;text-transform:uppercase;letter-spacing:.6px}
.pages{display:flex;gap:6px;flex-wrap:wrap;align-items:center;margin-bottom:14px}
.chip{background:var(--panel2);border:1px solid var(--line);color:var(--text);padding:6px 12px;border-radius:999px;cursor:pointer;font:inherit}
.chip.on{border-color:var(--accent);color:var(--accent)}
.chip.add{color:var(--muted)}
.deck{background:#000;border-radius:16px;padding:8px;display:grid;gap:8px;aspect-ratio:640/180;max-width:100%;border:6px solid #23272f}
.key{border-radius:12px;display:flex;flex-direction:column;align-items:center;justify-content:center;gap:2px;cursor:pointer;text-align:center;font-size:12px;line-height:1.15;white-space:pre-line;overflow:hidden;border:2px solid transparent;user-select:none;padding:3px;background-image:linear-gradient(rgba(255,255,255,.06),rgba(0,0,0,.25))}
.key .ic{font-size:20px;line-height:1}
.key.empty{background:#0e1014!important;border:1px dashed #2a303b;color:#3a404b;background-image:none}
.key.sel{border-color:var(--accent);box-shadow:0 0 0 2px var(--accent)}
.key.drag{opacity:.4}
.grid2{display:grid;grid-template-columns:1fr 1fr;gap:12px}
@media(max-width:720px){.grid2{grid-template-columns:1fr}}
label{display:block;font-size:13px;color:var(--muted);margin:10px 0 4px}
input,select,textarea{width:100%;background:var(--panel2);border:1px solid var(--line);color:var(--text);border-radius:8px;padding:8px 10px;font:inherit}
textarea{min-height:64px;resize:vertical;font-family:ui-monospace,Consolas,monospace;font-size:13px}
input[type=color]{padding:2px;height:38px}
.row{display:flex;gap:8px;align-items:center;flex-wrap:wrap}
.row>*{flex:1}
.btn{background:var(--panel2);border:1px solid var(--line);color:var(--text);padding:8px 14px;border-radius:8px;cursor:pointer;font:inherit;flex:0 0 auto}
.btn:hover{border-color:#3d4452}
.btn.primary{background:var(--accent);border-color:var(--accent);color:#04121b;font-weight:600}
.btn.danger{color:var(--danger)}
.hint{color:var(--muted);font-size:12.5px;margin-top:6px}
.hint code{background:var(--panel2);padding:1px 5px;border-radius:4px}
.savebar{position:fixed;left:0;right:0;bottom:0;background:rgba(13,15,19,.95);border-top:1px solid var(--line);padding:12px 20px;display:flex;gap:10px;align-items:center;justify-content:flex-end;backdrop-filter:blur(6px)}
.savebar .msg{flex:1;color:var(--muted);font-size:13px}
.dirty{color:#ffc857!important}
.net{display:flex;justify-content:space-between;padding:8px 10px;border-radius:8px;cursor:pointer}
.net:hover{background:var(--panel2)}
.rec{border-color:#ffc857!important;color:#ffc857}
@media(max-width:720px){.deck{aspect-ratio:auto;padding:6px;gap:6px}.key{font-size:11px}.key .ic{font-size:16px}nav{width:100%;overflow-x:auto}header .status{order:3;width:100%;flex:none}}
#toast{position:fixed;top:16px;left:50%;transform:translateX(-50%);padding:10px 18px;border-radius:10px;display:none;z-index:20;font-weight:500}
</style>
</head>
<body>
<header>
  <h1>⌨️ LongDeck</h1>
  <div class="status" id="status">verbinde …</div>
  <nav>
    <button data-tab="keys" class="on">Tasten</button>
    <button data-tab="settings">Einstellungen</button>
    <button data-tab="wifi">WLAN</button>
    <button data-tab="system">System</button>
  </nav>
</header>
<main>
<section id="keys" class="on">
  <div class="card">
    <div class="pages" id="pages"></div>
    <div class="deck" id="deck"></div>
    <div class="hint">Taste anklicken zum Bearbeiten · Tasten per Drag &amp; Drop vertauschen · Auf dem Gerät wischt man zwischen den Seiten.</div>
  </div>
  <div class="card" id="pagecard">
    <h2>Seite</h2>
    <div class="row">
      <input id="pageName" placeholder="Seitenname">
      <button class="btn" id="pageLeft">◀</button>
      <button class="btn" id="pageRight">▶</button>
      <button class="btn danger" id="pageDel">Seite löschen</button>
    </div>
  </div>
  <div class="card" id="editor" style="display:none">
    <h2>Taste <span id="edIdx"></span></h2>
    <div class="grid2">
      <div>
        <label>Beschriftung (Enter = neue Zeile)</label>
        <textarea id="edLabel" style="min-height:44px;font-family:inherit"></textarea>
        <div class="row">
          <div><label>Symbol</label><select id="edIcon"></select></div>
          <div><label>Hintergrund</label><input type="color" id="edBg"></div>
          <div><label>Schrift</label><input type="color" id="edFg"></div>
        </div>
      </div>
      <div>
        <label>Aktion</label>
        <select id="edType">
          <option value="none">Keine</option>
          <option value="hotkey">Tastenkombination</option>
          <option value="media">Media-Taste</option>
          <option value="text">Text tippen</option>
          <option value="macro">Makro (mehrere Schritte)</option>
          <option value="http">HTTP / Webhook</option>
          <option value="page">Seite wechseln</option>
        </select>
        <div id="f-hotkey" class="fields">
          <label>Kombination</label>
          <div class="row"><input id="edHotkey" placeholder="z. B. CTRL+SHIFT+M"><button class="btn" id="recBtn">⏺ Aufnehmen</button></div>
          <div class="hint">Modifier: <code>CTRL</code> <code>SHIFT</code> <code>ALT</code> <code>ALTGR</code> <code>WIN</code> · Tasten: Buchstaben, Ziffern, <code>F1</code>–<code>F24</code>, <code>ENTER</code> <code>ESC</code> <code>TAB</code> <code>SPACE</code> <code>DELETE</code> <code>HOME</code> <code>END</code> <code>UP</code> <code>DOWN</code> <code>LEFT</code> <code>RIGHT</code> <code>PRINTSCREEN</code> … Folgen mit Komma: <code>CTRL+K, CTRL+C</code></div>
        </div>
        <div id="f-media" class="fields"><label>Media-Taste</label><select id="edMedia"></select></div>
        <div id="f-text" class="fields"><label>Text</label><textarea id="edText"></textarea><div class="hint">Zeilenumbruch = Enter. Umlaute funktionieren mit dem Layout „Deutsch“.</div></div>
        <div id="f-macro" class="fields"><label>Schritte (eine Zeile pro Schritt)</label><textarea id="edMacro" style="min-height:110px" placeholder="key:WIN+R&#10;delay:400&#10;text:notepad&#10;key:ENTER"></textarea>
          <div class="hint"><code>key:CTRL+C</code> · <code>text:Hallo</code> · <code>media:MUTE</code> · <code>delay:500</code> · <code>page:2</code></div></div>
        <div id="f-http" class="fields">
          <label>URL</label><input id="edUrl" placeholder="http://homeassistant.local:8123/api/webhook/licht">
          <div class="row">
            <div><label>Methode</label><select id="edMethod"><option>POST</option><option>GET</option><option>PUT</option></select></div>
            <div><label>Content-Type</label><input id="edCt" placeholder="application/json"></div>
          </div>
          <label>Body</label><textarea id="edBody" placeholder='{"entity_id":"light.wohnzimmer"}'></textarea>
          <label>Bearer-Token (optional, z. B. Home-Assistant-Token)</label><input id="edAuth" type="password" autocomplete="off">
          <div class="hint">Home Assistant: am einfachsten eine Automation mit Webhook-Auslöser anlegen und deren URL hier eintragen – dann braucht es keinen Token. Benötigt WLAN.</div>
        </div>
        <div id="f-page" class="fields"><label>Zielseite</label><select id="edPage"></select></div>
      </div>
    </div>
    <div class="row" style="margin-top:16px;justify-content:flex-end">
      <button class="btn danger" id="edClear">Leeren</button>
      <button class="btn" id="edTest">▶ Testen</button>
    </div>
  </div>
</section>

<section id="settings">
  <div class="card">
    <h2>Raster &amp; Anzeige</h2>
    <div class="grid2">
      <div><label>Spalten (1–8)</label><input type="number" id="sCols" min="1" max="8"></div>
      <div><label>Zeilen (1–3)</label><input type="number" id="sRows" min="1" max="3"></div>
      <div><label>Helligkeit (10–255)</label><input type="range" id="sBright" min="10" max="255"></div>
      <div><label>Dimmen nach (Sekunden, 0 = nie)</label><input type="number" id="sDim" min="0" max="3600"></div>
      <div><label>Helligkeit gedimmt (0 = aus)</label><input type="range" id="sDimB" min="0" max="255"></div>
      <div><label>Gerätename (USB)</label><input id="sName" maxlength="24"></div>
    </div>
    <div class="hint">Ändert man Spalten/Zeilen, bleiben die Tasten in Lesereihenfolge erhalten; überzählige fallen weg.</div>
  </div>
  <div class="card">
    <h2>Tastatur</h2>
    <div class="grid2">
      <div><label>Tastaturlayout des PCs</label><select id="sLayout"><option value="de">Deutsch (QWERTZ)</option><option value="us">Englisch (US)</option></select></div>
      <div><label>Tipp-Pause in ms (höher = zuverlässiger)</label><input type="number" id="sDelay" min="2" max="200"></div>
    </div>
  </div>
</section>

<section id="wifi">
  <div class="card">
    <h2>WLAN</h2>
    <div class="row"><button class="btn" id="scanBtn">Netzwerke suchen</button></div>
    <div id="nets" style="margin:10px 0"></div>
    <div class="grid2">
      <div><label>Netzwerk (SSID)</label><input id="wSsid"></div>
      <div><label>Passwort</label><input id="wPass" type="password"></div>
    </div>
    <div class="row" style="margin-top:14px;justify-content:flex-end"><button class="btn primary" id="wSave">Speichern &amp; neu starten</button></div>
    <div class="hint">Danach erreichst du diese Seite unter <code>http://longdeck.local</code> oder der IP, die auf der letzten Seite des Geräts steht.</div>
  </div>
</section>

<section id="system">
  <div class="card">
    <h2>Sicherung</h2>
    <div class="row">
      <button class="btn" id="exportBtn">⬇ Konfiguration exportieren</button>
      <label class="btn" style="margin:0;color:var(--text)">⬆ Importieren<input type="file" id="importFile" accept=".json" hidden></label>
      <button class="btn danger" id="resetBtn">Auf Standard zurücksetzen</button>
    </div>
  </div>
  <div class="card">
    <h2>Zugriffsschutz</h2>
    <div class="row"><input id="webPass" type="password" placeholder="Passwort für diese Seite (leer = kein Schutz)"><button class="btn" id="webPassBtn">Übernehmen</button></div>
    <div class="hint">Benutzername ist <code>admin</code>. Empfohlen, da jeder im WLAN sonst Tastendrücke auslösen kann.</div>
  </div>
  <div class="card">
    <h2>Firmware-Update</h2>
    <div class="row"><input type="file" id="fwFile" accept=".bin"><button class="btn" id="fwBtn">Hochladen</button></div>
    <div class="hint">Datei <code>longdeck-ota.bin</code> aus dem GitHub-Release verwenden (nicht die Web-Installer-Datei).</div>
    <div class="hint" id="fwProg"></div>
  </div>
  <div class="card">
    <h2>Gerät</h2>
    <div id="sysInfo" class="hint"></div>
    <div class="row" style="margin-top:10px"><button class="btn" id="restartBtn">Neu starten</button></div>
  </div>
</section>
</main>

<div class="savebar"><div class="msg" id="saveMsg">Alle Änderungen gespeichert</div><button class="btn primary" id="saveBtn">Speichern</button></div>
<div id="toast"></div>

<script>
const ICONS={audio:"🎵",video:"🎬",list:"☰",ok:"✔",close:"✖",power:"⏻",settings:"⚙",home:"⌂",download:"⬇",drive:"🖴",refresh:"↻",mute:"🔇",volume_mid:"🔉",volume_max:"🔊",image:"🖼",tint:"💧",prev:"⏮",play:"▶",pause:"⏸",stop:"⏹",next:"⏭",eject:"⏏",left:"◀",right:"▶",plus:"＋",minus:"－",eye_open:"👁",eye_close:"🙈",warning:"⚠",shuffle:"🔀",up:"▲",down:"▼",loop:"🔁",directory:"📁",upload:"⬆",call:"📞",cut:"✂",copy:"⧉",save:"💾",bars:"≡",envelope:"✉",charge:"⚡",paste:"📋",bell:"🔔",keyboard:"⌨",gps:"📍",file:"📄",wifi:"📶",battery:"🔋",usb:"🔌",bluetooth:"ᛒ",trash:"🗑",edit:"✎",backspace:"⌫",sd_card:"💳",new_line:"↵"};
const MEDIA_LABELS={VOL_UP:"Lauter",VOL_DOWN:"Leiser",MUTE:"Stumm",PLAY_PAUSE:"Play/Pause",NEXT:"Nächster Titel",PREV:"Vorheriger Titel",STOP:"Stopp",BRIGHT_UP:"Bildschirm heller",BRIGHT_DOWN:"Bildschirm dunkler",CALCULATOR:"Rechner",BROWSER:"Browser",MAIL:"Mail",MY_COMPUTER:"Arbeitsplatz",WWW_HOME:"Browser: Start",WWW_BACK:"Browser: Zurück",WWW_FORWARD:"Browser: Vor",WWW_REFRESH:"Browser: Neu laden",WWW_SEARCH:"Browser: Suche"};
let cfg=null,info=null,page=0,sel=-1,dirty=false,recording=false,dragFrom=-1;
const $=id=>document.getElementById(id);

function toast(msg,ok=true){const t=$('toast');t.textContent=msg;t.style.background=ok?'#1e7a3c':'#a3262e';t.style.display='block';clearTimeout(t._h);t._h=setTimeout(()=>t.style.display='none',2200)}
function setDirty(d=true){dirty=d;$('saveMsg').textContent=d?'Ungespeicherte Änderungen':'Alle Änderungen gespeichert';$('saveMsg').classList.toggle('dirty',d)}
async function api(path,opts={}){const r=await fetch(path,opts);let j={};try{j=await r.json()}catch(e){}if(!r.ok)throw new Error(j.error||('HTTP '+r.status));return j}
function blank(){return {label:"",icon:"",bg:"#2A2F3A",fg:"#FFFFFF",type:"none",value:""}}
function n(){return cfg.cols*cfg.rows}
function normalize(){cfg.pages.forEach(p=>{while(p.buttons.length<n())p.buttons.push(blank());p.buttons.length=n()})}

document.querySelectorAll('nav button').forEach(b=>b.onclick=()=>{document.querySelectorAll('nav button').forEach(x=>x.classList.toggle('on',x===b));document.querySelectorAll('section').forEach(s=>s.classList.toggle('on',s.id===b.dataset.tab))});

function renderPages(){
  const el=$('pages');el.innerHTML='';
  cfg.pages.forEach((p,i)=>{const c=document.createElement('button');c.className='chip'+(i===page?' on':'');c.textContent=(i+1)+' · '+p.name;c.onclick=()=>{page=i;sel=-1;render()};el.appendChild(c)});
  if(cfg.pages.length<12){const a=document.createElement('button');a.className='chip add';a.textContent='+ Seite';a.onclick=()=>{const p={name:'Seite '+(cfg.pages.length+1),buttons:[]};cfg.pages.push(p);normalize();page=cfg.pages.length-1;sel=-1;setDirty();render()};el.appendChild(a)}
  $('pageName').value=cfg.pages[page].name;
}
function renderDeck(){
  const d=$('deck');d.innerHTML='';d.style.gridTemplateColumns=`repeat(${cfg.cols},1fr)`;d.style.gridTemplateRows=`repeat(${cfg.rows},minmax(60px,1fr))`;
  cfg.pages[page].buttons.forEach((b,i)=>{
    const k=document.createElement('div');const empty=b.type==='none'&&!b.label&&!b.icon;
    k.className='key'+(empty?' empty':'')+(i===sel?' sel':'');k.style.background=b.bg;k.style.color=b.fg;k.draggable=true;
    if(b.icon&&ICONS[b.icon]){const s=document.createElement('div');s.className='ic';s.textContent=ICONS[b.icon];k.appendChild(s)}
    const l=document.createElement('div');l.textContent=b.label||(empty?'+':'');k.appendChild(l);
    k.onclick=()=>{sel=i;render()};
    k.ondragstart=e=>{dragFrom=i;k.classList.add('drag')};k.ondragend=()=>k.classList.remove('drag');
    k.ondragover=e=>e.preventDefault();
    k.ondrop=e=>{e.preventDefault();if(dragFrom<0||dragFrom===i)return;const bs=cfg.pages[page].buttons;[bs[dragFrom],bs[i]]=[bs[i],bs[dragFrom]];if(sel===dragFrom)sel=i;else if(sel===i)sel=dragFrom;dragFrom=-1;setDirty();render()};
    d.appendChild(k)});
}
function showFields(t){document.querySelectorAll('.fields').forEach(f=>f.style.display=f.id==='f-'+t?'block':'none')}
function renderEditor(){
  if(sel<0){$('editor').style.display='none';return}
  const b=cfg.pages[page].buttons[sel];$('editor').style.display='block';$('edIdx').textContent=(sel+1)+' (Zeile '+(Math.floor(sel/cfg.cols)+1)+', Spalte '+(sel%cfg.cols+1)+')';
  $('edLabel').value=b.label;$('edIcon').value=b.icon||'';$('edBg').value=b.bg.toLowerCase();$('edFg').value=b.fg.toLowerCase();$('edType').value=b.type;
  $('edHotkey').value=b.type==='hotkey'?b.value:'';$('edMedia').value=b.type==='media'?b.value:'VOL_UP';$('edText').value=b.type==='text'?b.value:'';$('edMacro').value=b.type==='macro'?b.value:'';
  $('edUrl').value=b.type==='http'?b.value:'';$('edMethod').value=b.method||'POST';$('edBody').value=b.body||'';$('edAuth').value=b.auth||'';$('edCt').value=b.contentType||'application/json';
  const ps=$('edPage');ps.innerHTML='';cfg.pages.forEach((p,i)=>{const o=document.createElement('option');o.value=i+1;o.textContent=(i+1)+' · '+p.name;ps.appendChild(o)});if(b.type==='page')ps.value=b.value;
  showFields(b.type);
}
function render(){renderPages();renderDeck();renderEditor()}

function readEditor(){
  if(sel<0)return;const b=cfg.pages[page].buttons[sel];
  b.label=$('edLabel').value;b.icon=$('edIcon').value;b.bg=$('edBg').value.toUpperCase();b.fg=$('edFg').value.toUpperCase();b.type=$('edType').value;
  delete b.method;delete b.body;delete b.auth;delete b.contentType;
  switch(b.type){case'hotkey':b.value=$('edHotkey').value;break;case'media':b.value=$('edMedia').value;break;case'text':b.value=$('edText').value;break;case'macro':b.value=$('edMacro').value;break;
    case'http':b.value=$('edUrl').value;b.method=$('edMethod').value;b.body=$('edBody').value;b.auth=$('edAuth').value;b.contentType=$('edCt').value;break;case'page':b.value=$('edPage').value;break;default:b.value=''}
  setDirty();renderDeck();
}
['edLabel','edIcon','edBg','edFg','edHotkey','edMedia','edText','edMacro','edUrl','edMethod','edBody','edAuth','edCt','edPage'].forEach(id=>{$(id).addEventListener('input',readEditor);$(id).addEventListener('change',readEditor)});
$('edType').onchange=()=>{showFields($('edType').value);readEditor()};
$('edClear').onclick=()=>{cfg.pages[page].buttons[sel]=blank();setDirty();render()};
$('edTest').onclick=async()=>{try{if(dirty)await save(true);await api('/api/press?page='+page+'&btn='+sel,{method:'POST'});toast('Ausgelöst')}catch(e){toast(e.message,false)}};
$('pageName').oninput=()=>{cfg.pages[page].name=$('pageName').value;setDirty();renderPages()};
$('pageDel').onclick=()=>{if(cfg.pages.length<2)return toast('Mindestens eine Seite nötig',false);if(!confirm('Seite „'+cfg.pages[page].name+'“ löschen?'))return;cfg.pages.splice(page,1);page=Math.max(0,page-1);sel=-1;setDirty();render()};
function movePage(d){const j=page+d;if(j<0||j>=cfg.pages.length)return;[cfg.pages[page],cfg.pages[j]]=[cfg.pages[j],cfg.pages[page]];page=j;setDirty();render()}
$('pageLeft').onclick=()=>movePage(-1);$('pageRight').onclick=()=>movePage(1);

// Kombination aufnehmen
const CODE_MAP={Enter:'ENTER',Escape:'ESC',Backspace:'BACKSPACE',Tab:'TAB',Space:'SPACE',Delete:'DELETE',Insert:'INSERT',Home:'HOME',End:'END',PageUp:'PGUP',PageDown:'PGDN',ArrowUp:'UP',ArrowDown:'DOWN',ArrowLeft:'LEFT',ArrowRight:'RIGHT',PrintScreen:'PRINTSCREEN',Pause:'PAUSE',ContextMenu:'MENU',NumpadAdd:'NUMPLUS',NumpadSubtract:'NUMMINUS',NumpadMultiply:'NUMMULT',NumpadDivide:'NUMDIV',NumpadEnter:'NUMENTER'};
$('recBtn').onclick=()=>{recording=!recording;$('recBtn').classList.toggle('rec',recording);$('recBtn').textContent=recording?'… Taste drücken':'⏺ Aufnehmen';if(recording)$('edHotkey').focus()};
$('edHotkey').addEventListener('keydown',e=>{if(!recording)return;e.preventDefault();
  if(['Control','Shift','Alt','Meta','AltGraph','OS'].includes(e.key))return;
  const m=[];if(e.ctrlKey)m.push('CTRL');if(e.shiftKey)m.push('SHIFT');if(e.altKey)m.push('ALT');if(e.metaKey)m.push('WIN');
  let k=CODE_MAP[e.code];
  if(!k&&/^F\d+$/.test(e.key))k=e.key;
  if(!k&&/^Key[A-Z]$/.test(e.code))k=(e.key.length===1&&/[a-zA-Z]/.test(e.key))?e.key.toUpperCase():e.code.slice(3);
  if(!k&&/^Digit\d$/.test(e.code))k=e.code.slice(5);
  if(!k&&/^Numpad\d$/.test(e.code))k='NUM'+e.code.slice(6);
  if(!k&&e.key.length===1)k=e.key==='+'?'PLUS':e.key.toUpperCase();
  if(!k)return;
  $('edHotkey').value=[...m,k].join('+');recording=false;$('recBtn').classList.remove('rec');$('recBtn').textContent='⏺ Aufnehmen';readEditor()});

// Einstellungen
function renderSettings(){$('sCols').value=cfg.cols;$('sRows').value=cfg.rows;$('sBright').value=cfg.brightness;$('sDim').value=cfg.dimAfter;$('sDimB').value=cfg.dimBrightness;$('sName').value=cfg.deviceName;$('sLayout').value=cfg.layout;$('sDelay').value=cfg.typeDelay}
function resizeGrid(cols,rows){
  cols=Math.min(8,Math.max(1,cols|0));rows=Math.min(3,Math.max(1,rows|0));if(cols===cfg.cols&&rows===cfg.rows)return;
  cfg.pages.forEach(p=>{const old=p.buttons,nb=[];for(let r=0;r<rows;r++)for(let c=0;c<cols;c++){nb.push(r<cfg.rows&&c<cfg.cols?old[r*cfg.cols+c]:blank())}p.buttons=nb});
  cfg.cols=cols;cfg.rows=rows;sel=-1;setDirty();render();
}
$('sCols').onchange=()=>resizeGrid(+$('sCols').value,cfg.rows);$('sRows').onchange=()=>resizeGrid(cfg.cols,+$('sRows').value);
[['sBright','brightness',1],['sDim','dimAfter',1],['sDimB','dimBrightness',1],['sName','deviceName',0],['sLayout','layout',0],['sDelay','typeDelay',1]].forEach(([id,key,num])=>{$(id).addEventListener('input',()=>{cfg[key]=num?+$(id).value:$(id).value;setDirty()})});

async function save(silent){normalize();await api('/api/config',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(cfg)});setDirty(false);if(!silent)toast('Gespeichert – Gerät aktualisiert')}
$('saveBtn').onclick=()=>save().catch(e=>toast(e.message,false));
window.addEventListener('keydown',e=>{if((e.ctrlKey||e.metaKey)&&e.key==='s'){e.preventDefault();save().catch(er=>toast(er.message,false))}});
window.addEventListener('beforeunload',e=>{if(dirty){e.preventDefault();e.returnValue=''}});

// WLAN
$('scanBtn').onclick=async()=>{$('nets').textContent='Suche …';try{const j=await api('/api/scan');$('nets').innerHTML='';j.networks.sort((a,b)=>b.rssi-a.rssi).forEach(w=>{const d=document.createElement('div');d.className='net';d.innerHTML=`<span></span><span class="hint">${w.rssi} dBm ${w.secure?'🔒':''}</span>`;d.firstChild.textContent=w.ssid;d.onclick=()=>{$('wSsid').value=w.ssid;$('wPass').focus()};$('nets').appendChild(d)});if(!j.networks.length)$('nets').textContent='Keine Netzwerke gefunden'}catch(e){$('nets').textContent=e.message}};
$('wSave').onclick=async()=>{if(!$('wSsid').value)return toast('SSID fehlt',false);try{await api('/api/wifi',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({ssid:$('wSsid').value,pass:$('wPass').value})});toast('Gespeichert – Gerät startet neu')}catch(e){toast(e.message,false)}};

// System
$('exportBtn').onclick=()=>{const a=document.createElement('a');a.href=URL.createObjectURL(new Blob([JSON.stringify(cfg,null,2)],{type:'application/json'}));a.download='longdeck-config.json';a.click()};
$('importFile').onchange=async e=>{const f=e.target.files[0];if(!f)return;try{const j=JSON.parse(await f.text());if(!Array.isArray(j.pages))throw new Error('Keine LongDeck-Konfiguration');cfg=j;page=0;sel=-1;normalize();render();renderSettings();await save()}catch(er){toast(er.message,false)}e.target.value=''};
$('resetBtn').onclick=async()=>{if(!confirm('Alle Tasten auf Standard zurücksetzen?'))return;try{await api('/api/reset',{method:'POST'});await load();toast('Zurückgesetzt')}catch(e){toast(e.message,false)}};
$('webPassBtn').onclick=async()=>{try{await api('/api/security',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({webPass:$('webPass').value})});toast($('webPass').value?'Passwort gesetzt (Benutzer: admin)':'Zugriffsschutz entfernt')}catch(e){toast(e.message,false)}};
$('restartBtn').onclick=async()=>{try{await api('/api/restart',{method:'POST'});toast('Startet neu …')}catch(e){toast(e.message,false)}};
$('fwBtn').onclick=()=>{const f=$('fwFile').files[0];if(!f)return toast('Datei wählen',false);const fd=new FormData();fd.append('firmware',f,f.name);const x=new XMLHttpRequest();x.open('POST','/update');x.upload.onprogress=e=>{if(e.lengthComputable)$('fwProg').textContent='Hochladen: '+Math.round(e.loaded/e.total*100)+' %'};x.onload=()=>{$('fwProg').textContent=x.status===200?'Fertig – Gerät startet neu':'Fehler: '+x.responseText};x.onerror=()=>$('fwProg').textContent='Verbindungsfehler';x.send(fd)};

async function refreshInfo(){try{info=await api('/api/info');$('status').textContent=(info.connected?('WLAN '+info.ssid+' · '+info.ip):(info.ap?'Einrichtungs-WLAN aktiv':'nicht verbunden'))+' · v'+info.version;$('sysInfo').textContent=`Version ${info.version} · freier Speicher ${Math.round(info.heap/1024)} KB · PSRAM ${Math.round(info.psram/1024)} KB${info.connected?' · Signal '+info.rssi+' dBm':''}`;if(info.ssid&&!$('wSsid').value)$('wSsid').value=info.ssid}catch(e){$('status').textContent='keine Verbindung zum Gerät'}}
async function load(){
  info=await api('/api/info');cfg=await api('/api/config');
  const ic=$('edIcon');ic.innerHTML='<option value="">— keins —</option>';info.icons.forEach(i=>{const o=document.createElement('option');o.value=i;o.textContent=(ICONS[i]||'')+'  '+i;ic.appendChild(o)});
  const md=$('edMedia');md.innerHTML='';info.media.forEach(m=>{const o=document.createElement('option');o.value=m;o.textContent=MEDIA_LABELS[m]||m;md.appendChild(o)});
  page=Math.min(page,cfg.pages.length-1);normalize();render();renderSettings();setDirty(false);refreshInfo();
}
load().catch(e=>toast('Laden fehlgeschlagen: '+e.message,false));
setInterval(refreshInfo,10000);
</script>
</body>
</html>
)rawliteral";
