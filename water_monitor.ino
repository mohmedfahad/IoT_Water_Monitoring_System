/*
 * ESP32 Water Monitor — WaterWise v2
 * Sensors: AJ-SR04M (TRIG=GPIO5, ECHO=GPIO18)
 *          pH  analog (GPIO 34)
 *          TDS analog (GPIO 35)
 *
 * Routes:
 *   GET  /           -> login page
 *   POST /login      -> validate, set cookie, redirect /dashboard
 *   GET  /dashboard  -> dashboard (cookie required)
 *   GET  /data       -> JSON (cookie required)
 *   GET  /logout     -> clear cookie, redirect /
 */

#include <WebServer.h>
#include <WiFi.h>
#include <Preferences.h>

// ─── WiFi ───────────────────────────────────────────────────
const char *ssid     = "GNXS-2.4G-BE7E20";
const char *password = "abcd1234";

// ─── Auth ───────────────────────────────────────────────────
const char *AUTH_USER     = "user";
const char *AUTH_PASS     = "password";
const char *SESSION_TOKEN = "ww_auth_ok_esp32";

// ─── Pins ───────────────────────────────────────────────────
#define TRIG_PIN 5
#define ECHO_PIN 18
#define PH_PIN   34
#define TDS_PIN  35
#define TURB_PIN 32
#define PUMP_PIN 23

// ─── Relay Logic ────────────────────────────────────────────
// Standard relay modules trigger on LOW for ESP32
#define PUMP_ON  LOW
#define PUMP_OFF HIGH

// ─── ADC / Sampling ─────────────────────────────────────────
#define VREF    3.3f
#define ADC_RES 4095.0f
#define SCOUNT  30

// ─── Tank Calibration ───────────────────────────────────────
const float EMPTY_DISTANCE_CM = 16.5f;
const float FULL_DISTANCE_CM  = 5.0f;
const float MIN_SENSOR_CM     = 5.0f;

// ─── Speed of Sound ─────────────────────────────────────────
const float AIR_TEMP_C   = 28.0f;
float soundSpeed_cm_us   = 0.0f;

// ─── pH Calibration (hardcoded defaults) ────────────────────
float voltageAtPH7 = 1.65f;
float voltageAtPH4 = 2.20f;

// ─── TDS Calibration ────────────────────────────────────────
float tdsCalFactor = 0.5f;

// ─── Sample Buffers ─────────────────────────────────────────
int phBuffer[SCOUNT], tdsBuffer[SCOUNT], turbBuffer[SCOUNT];
int bufIndex = 0;

// ─── Globals ────────────────────────────────────────────────
float distance_cm    = 0.0f;
float waterLevel_cm  = 0.0f;
float levelPercent   = 0.0f;
float currentPH      = 7.0f;
float currentTDS     = 0.0f;
float currentTurb    = 0.0f;  // NTU
float lastPhVoltage  = 0.0f;
float lastTdsVoltage = 0.0f;
float lastTurbVoltage= 0.0f;
float desiredLevel   = 75.0f;
bool  pumpActive     = false;
bool  pumpAuto       = true;  // New: Auto vs Manual mode
float lowTh          = 20.0f; // ON at 20%
float highTh         = 80.0f; // OFF at 80%

Preferences prefs;
WebServer server(80);

// ─── Median filter ──────────────────────────────────────────
int getMedianNum(int arr[], int n) {
  int b[SCOUNT];
  for (int i = 0; i < n; i++) b[i] = arr[i];
  for (int j = 0; j < n - 1; j++)
    for (int i = 0; i < n - j - 1; i++)
      if (b[i] > b[i + 1]) { int t = b[i]; b[i] = b[i+1]; b[i+1] = t; }
  return (n & 1) ? b[(n-1)/2] : (b[n/2] + b[n/2-1]) / 2;
}

// ════════════════════════════════════════════════════════════
//  LOGIN PAGE
// ════════════════════════════════════════════════════════════
const char LOGIN_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8"/>
<meta name="viewport" content="width=device-width,initial-scale=1.0"/>
<title>WaterWise — Sign In</title>
<link rel="preconnect" href="https://fonts.googleapis.com">
<link href="https://fonts.googleapis.com/css2?family=Inter:wght@400;600;700;800&display=swap" rel="stylesheet">
<style>
*,*::before,*::after{box-sizing:border-box;margin:0;padding:0}
:root{--bg:#F3F6F9;--surface:#fff;--border:#E0E7EF;
  --blue-d:#1976D2;--text:#2C3E50;--muted:#7F8C8D;--red:#EF4444}
body{font-family:'Inter',sans-serif;background:var(--bg);color:var(--text);
  min-height:100vh;display:flex;align-items:center;justify-content:center;padding:24px}
.card{background:var(--surface);border-radius:28px;padding:40px 36px;
  width:100%;max-width:380px;box-shadow:0 8px 32px rgba(25,118,210,.10);text-align:center}
.icon{width:60px;height:60px;margin:0 auto 20px;
  background:linear-gradient(135deg,var(--blue-d),#6366f1);
  border-radius:18px;display:flex;align-items:center;justify-content:center;
  font-size:28px;box-shadow:0 6px 20px rgba(25,118,210,.35)}
h2{font-size:1.45rem;font-weight:800;margin-bottom:6px}
.sub{font-size:.82rem;color:var(--muted);margin-bottom:28px}
.fg{margin-bottom:16px;text-align:left}
.fg label{display:block;font-size:.75rem;font-weight:600;color:var(--muted);
  margin-bottom:6px;text-transform:uppercase;letter-spacing:.06em}
.iw{position:relative}
.iw .ic{position:absolute;left:13px;top:50%;transform:translateY(-50%);font-size:15px;color:var(--muted)}
.iw input{width:100%;padding:11px 14px 11px 38px;border:1.5px solid var(--border);
  border-radius:12px;font-family:inherit;font-size:.9rem;color:var(--text);
  background:#FAFBFC;transition:border-color .2s,box-shadow .2s;outline:none}
.iw input:focus{border-color:var(--blue-d);box-shadow:0 0 0 3px rgba(25,118,210,.12);background:#fff}
.err{background:rgba(239,68,68,.08);border:1px solid rgba(239,68,68,.25);
  color:var(--red);font-size:.8rem;font-weight:500;padding:9px 14px;
  border-radius:10px;margin-bottom:16px;display:none;text-align:left}
.err.show{display:block}
.btn{width:100%;padding:13px;margin-top:8px;
  background:linear-gradient(135deg,var(--blue-d),#6366f1);
  border:none;border-radius:14px;color:#fff;
  font-family:inherit;font-size:.95rem;font-weight:700;
  cursor:pointer;transition:opacity .2s,transform .1s}
.btn:hover{opacity:.9}.btn:active{transform:scale(.98)}
.foot{margin-top:24px;font-size:.72rem;color:var(--muted)}
</style>
</head>
<body>
<div class="card">
  <div class="icon">&#128167;</div>
  <h2>Welcome back</h2>
  <p class="sub">Sign in to WaterWise Monitor</p>
  <div class="err" id="eb">&#10006;&nbsp;Invalid username or password.</div>
  <form method="POST" action="/login">
    <div class="fg">
      <label>Username</label>
      <div class="iw"><span class="ic">&#128100;</span>
        <input type="text" name="username" placeholder="Enter username" required/></div>
    </div>
    <div class="fg">
      <label>Password</label>
      <div class="iw"><span class="ic">&#128274;</span>
        <input type="password" name="passwd" placeholder="Enter password" required/></div>
    </div>
    <button class="btn" type="submit">Sign In</button>
  </form>
  <p class="foot">AJ-SR04M &bull; pH &bull; TDS &bull; ESP32</p>
</div>
<script>
if(window.location.search.indexOf('err=1')!==-1)
  document.getElementById('eb').classList.add('show');
</script>
</body></html>
)rawliteral";

// ════════════════════════════════════════════════════════════
//  DASHBOARD
// ════════════════════════════════════════════════════════════
const char DASH_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8"/>
<meta name="viewport" content="width=device-width,initial-scale=1.0"/>
<title>WaterWise Monitor</title>
<link rel="preconnect" href="https://fonts.googleapis.com">
<link href="https://fonts.googleapis.com/css2?family=Inter:wght@300;400;500;600;700;800&display=swap" rel="stylesheet">
<script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
<style>
*,*::before,*::after{box-sizing:border-box;margin:0;padding:0}
:root{
  --bg:#F3F6F9;--surface:#FFFFFF;--border:#E5E7EB;
  --blue:#64B5F6;--blue-d:#1976D2;
  --green:#22c55e;--red:#EF4444;--yellow:#F59E0B;
  --purple:#8B5CF6;
  --text:#2C3E50;--muted:#7F8C8D;
  --shadow:0 4px 20px rgba(0,0,0,.06);
  --r-lg:24px;--r-md:16px;
}
body{font-family:'Inter',sans-serif;background:var(--bg);color:var(--text);
  min-height:100vh;display:flex;flex-direction:column;align-items:center;
  padding:24px 16px 48px}

/* ── Header ── */
.header{width:100%;max-width:500px;display:flex;align-items:center;
  justify-content:space-between;margin-bottom:24px}
.hl{display:flex;align-items:center;gap:10px}
.hicon{width:44px;height:44px;background:linear-gradient(135deg,var(--blue-d),#6366f1);
  border-radius:12px;display:flex;align-items:center;justify-content:center;
  font-size:22px;box-shadow:0 4px 14px rgba(25,118,210,.35)}
.ht{font-size:1.2rem;font-weight:700}
.hs{font-size:.72rem;color:var(--muted);margin-top:1px}
.hr{display:flex;align-items:center;gap:10px}
.cdot{width:9px;height:9px;border-radius:50%;background:#9CA3AF;
  animation:blink 2s ease-in-out infinite}
.cdot.on{background:var(--green)}
@keyframes blink{0%,100%{opacity:1}50%{opacity:.3}}
.lout{border:1px solid var(--border);background:var(--surface);color:var(--muted);
  font-family:inherit;font-size:.73rem;font-weight:600;padding:5px 13px;
  border-radius:50px;cursor:pointer;text-decoration:none;transition:all .2s}
.lout:hover{border-color:#cbd5e1;color:var(--text)}

/* ── Stack ── */
.stack{display:flex;flex-direction:column;gap:16px;width:100%;max-width:500px}

/* ── Card ── */
.card{background:var(--surface);border-radius:var(--r-lg);padding:22px;
  box-shadow:var(--shadow)}
.ctitle{font-size:.75rem;color:var(--muted);font-weight:600;margin-bottom:14px;
  text-transform:uppercase;letter-spacing:.07em}

/* ── Water Tank ── */
.twrap{display:flex;align-items:center;gap:20px}
.tglass{width:72px;height:140px;border-radius:0 0 28px 28px;
  border:2px solid rgba(100,181,246,.5);background:rgba(100,181,246,.06);
  position:relative;overflow:hidden;box-shadow:inset 0 0 12px rgba(100,181,246,.1)}
.tliq{position:absolute;bottom:0;left:0;right:0;
  background:linear-gradient(180deg,var(--blue) 0%,var(--blue-d) 100%);
  transition:height 1s cubic-bezier(.4,0,.2,1);z-index:2}
.tsurf{position:absolute;top:-8px;left:0;width:100%;height:16px}
.twave{position:absolute;top:-4px;left:0;width:200%;height:200%;
  border-radius:40%;background:rgba(255,255,255,.35);
  animation:wr 6s linear infinite;transform-origin:50% 50%}
.twave:nth-child(2){top:-6px;background:rgba(255,255,255,.18);
  animation:wr 8s linear infinite;border-radius:35%}
@keyframes wr{0%{transform:translateX(-25%) rotate(0deg)}100%{transform:translateX(-25%) rotate(360deg)}}
.tliq::before{content:'';position:absolute;width:200%;height:200%;top:-50%;left:-50%;
  background:radial-gradient(rgba(255,255,255,.15) 10%,transparent 20%);
  background-size:16px 16px;opacity:.6;animation:bb 4s linear infinite}
@keyframes bb{0%{transform:translateY(0)}100%{transform:translateY(-16px)}}
.tinfo{flex:1;display:flex;flex-direction:column;gap:8px}
.pv{font-size:3.2rem;font-weight:800;line-height:1;color:var(--blue-d)}
.pl{font-size:.78rem;color:var(--muted)}
.pb{background:var(--bg);border-radius:100px;height:8px;
  overflow:hidden;border:1px solid var(--border);margin:6px 0 0}
.pf{height:100%;border-radius:100px;
  background:linear-gradient(90deg,var(--blue),#6366f1);
  transition:width .8s cubic-bezier(.4,0,.2,1)}
.spill{display:inline-flex;align-items:center;gap:5px;font-size:.72rem;
  font-weight:600;padding:4px 10px;border-radius:50px;width:fit-content}
.spill .pip{width:6px;height:6px;border-radius:50%;animation:blink 1.6s infinite}
.p-ok{background:rgba(34,197,94,.12);color:#15803d}.p-ok .pip{background:#22c55e}
.p-hi{background:rgba(239,68,68,.12);color:#dc2626}.p-hi .pip{background:#ef4444}
.p-lo{background:rgba(245,158,11,.12);color:#b45309}.p-lo .pip{background:#f59e0b}
.p-er{background:rgba(100,116,139,.12);color:#64748b}.p-er .pip{background:#94a3b8}

/* ── Purity donut ── */
.pur-wrap{display:flex;align-items:center;gap:20px}
.pur-donut{position:relative;flex-shrink:0}
.pur-svg{width:130px;height:130px;transform:rotate(-90deg)}
.pur-bg{fill:none;stroke:#E5E7EB;stroke-width:7}
.pur-fg{fill:none;stroke-width:7;stroke-linecap:round;
  transition:stroke-dasharray .9s cubic-bezier(.4,0,.2,1),stroke .4s}
.pur-inner{position:absolute;inset:0;display:flex;flex-direction:column;
  align-items:center;justify-content:center}
.pur-num{font-size:2rem;font-weight:800;line-height:1}
.pur-sub{font-size:.62rem;color:var(--muted);font-weight:500;margin-top:2px}
.pur-info{flex:1;display:flex;flex-direction:column;gap:6px}
.pur-badge{display:inline-flex;align-items:center;gap:5px;font-size:.72rem;
  font-weight:700;padding:4px 11px;border-radius:50px;width:fit-content}
.pb-great{background:rgba(34,197,94,.12);color:#15803d}
.pb-good{background:rgba(100,181,246,.12);color:#1976D2}
.pb-fair{background:rgba(245,158,11,.12);color:#b45309}
.pb-poor{background:rgba(239,68,68,.12);color:#dc2626}
.pur-label{font-size:.72rem;color:var(--muted)}

/* Sensor mini tiles */
.sen-row{display:grid;grid-template-columns:1fr 1fr 1fr;gap:10px;margin-top:16px}
.sen-tile{background:var(--bg);border:1px solid var(--border);
  border-radius:var(--r-md);padding:12px 14px}
.sen-lbl{font-size:.65rem;color:var(--muted);text-transform:uppercase;
  letter-spacing:.07em;margin-bottom:4px}
.sen-val{font-size:1.25rem;font-weight:700;color:var(--text)}
.sen-unit{font-size:.65rem;color:var(--muted);font-weight:400}
.sen-bar{height:4px;border-radius:4px;margin-top:6px;background:var(--border);overflow:hidden}
.sen-bar-fill{height:100%;border-radius:4px;transition:width .7s cubic-bezier(.4,0,.2,1)}
.ph-fill{background:linear-gradient(90deg,#22d3ee,#6366f1)}
.tds-fill{background:linear-gradient(90deg,#a78bfa,#ec4899)}
.turb-fill{background:linear-gradient(90deg,#f97316,#ef4444)}

/* ── Slider card ── */
.slider-wrap{display:flex;align-items:center;gap:16px}
.slider-info{flex:1}
.slider-lbl{font-size:.78rem;font-weight:600;color:var(--text);margin-bottom:2px}
.slider-sub{font-size:.68rem;color:var(--muted)}
.slider-val{font-size:2rem;font-weight:800;color:var(--blue-d);text-align:center;min-width:60px}
.slider-pct{font-size:.7rem;color:var(--muted);font-weight:400}
input[type=range]{-webkit-appearance:none;width:100%;height:8px;
  border-radius:8px;background:linear-gradient(90deg,var(--blue),#6366f1);
  outline:none;margin:10px 0 4px}
input[type=range]::-webkit-slider-thumb{-webkit-appearance:none;width:22px;height:22px;
  border-radius:50%;background:#fff;border:3px solid var(--blue-d);
  box-shadow:0 2px 8px rgba(0,0,0,.15);cursor:pointer;transition:transform .15s}
input[type=range]::-webkit-slider-thumb:hover{transform:scale(1.15)}
.slider-ticks{display:flex;justify-content:space-between;font-size:.6rem;color:var(--muted)}

/* ── Chart ── */
.tabs{display:flex;background:#EDF2F7;border-radius:12px;padding:4px;margin-bottom:14px}
.tab{flex:1;text-align:center;padding:7px 0;font-size:.75rem;font-weight:700;
  border-radius:9px;color:var(--blue-d);background:var(--surface);
  box-shadow:0 2px 6px rgba(0,0,0,.07);user-select:none}
.chwrap{position:relative;height:180px;width:100%}

/* Footer */
hr{border:none;border-top:1px solid var(--border);margin:14px 0}
.frow{display:flex;align-items:center;justify-content:space-between;margin-top:8px}
.ftime{font-size:.7rem;color:var(--muted)}
.rbtn{border:none;background:linear-gradient(135deg,var(--blue-d),#6366f1);
  color:#fff;font-family:inherit;font-size:.75rem;font-weight:600;
  padding:6px 14px;border-radius:50px;cursor:pointer;transition:opacity .2s}
.rbtn:hover{opacity:.85}
.btn-warn{background:linear-gradient(135deg,var(--red),#f87171)}
.btn-succ{background:linear-gradient(135deg,var(--green),#4ade80)}

/* ── Notification Dropdown ── */
.n-btn{position:relative;background:none;border:none;cursor:pointer;color:var(--muted);
  display:flex;align-items:center;justify-content:center;padding:8px;border-radius:50%;
  transition:background .2s}
.n-btn:hover{background:rgba(0,0,0,.04);color:var(--text)}
.n-dot{position:absolute;top:7px;right:7px;width:9px;height:9px;background:var(--red);
  border:2px solid var(--surface);border-radius:50%;display:none}
.n-dot.show{display:block}
.n-drop{position:absolute;top:60px;right:0;width:320px;background:var(--surface);
  border-radius:18px;box-shadow:0 10px 40px rgba(0,0,0,.12);border:1px solid var(--border);
  z-index:200;display:none;flex-direction:column;overflow:hidden;text-align:left}
.n-drop.show{display:flex}
.n-hdr{padding:16px 20px;font-size:.7rem;font-weight:700;color:var(--muted);
  text-transform:uppercase;letter-spacing:.08em;border-bottom:1px solid var(--border)}
.n-list{max-height:360px;overflow-y:auto}
.n-item{padding:16px 20px;display:flex;gap:15px;border-bottom:1px solid var(--border);
  transition:background .2s;cursor:default}
.n-item:hover{background:#F9FAFB}
.n-item:last-child{border-bottom:none}
.n-icon{flex-shrink:0;width:34px;height:34px;border-radius:50%;display:flex;
  align-items:center;justify-content:center;font-size:16px}
.ni-info{background:rgba(25,118,210,.1);color:var(--blue-d)}
.ni-warn{background:rgba(239,68,68,.1);color:var(--red)}
.ni-succ{background:rgba(34,197,94,.1);color:var(--green)}
.n-body{flex:1}
.n-title{font-size:.85rem;font-weight:700;color:var(--text);margin-bottom:3px}
.n-text{font-size:.78rem;color:var(--muted);line-height:1.4}
.n-time{font-size:.65rem;color:#9CA3AF;margin-top:6px}
.n-empty{padding:40px 20px;text-align:center;color:var(--muted);font-size:.8rem}
</style>
</head>
<body>

<!-- Header -->
<div class="header">
  <div class="hl">
    <div class="hicon">&#128167;</div>
    <div>
      <div class="ht">WaterWise Monitor</div>
      <div class="hs">AJ-SR04M &bull; pH &bull; TDS &bull; ESP32</div>
    </div>
  </div>
  <div class="hr">
    <div style="position:relative">
      <button class="n-btn" onclick="toggleN(event)">
        <svg fill="none" stroke="currentColor" stroke-width="2" viewBox="0 0 24 24" width="22" height="22">
          <path d="M15 17h5l-1.405-1.405A2.032 2.032 0 0118 14.158V11a6.002 6.002 0 00-4-5.659V5a2 2 0 10-4 0v.341C7.67 6.165 6 8.388 6 11v3.159c0 .538-.214 1.055-.595 1.436L4 17h5m6 0v1a3 3 0 11-6 0v-1m6 0H9"></path>
        </svg>
        <div class="n-dot" id="nDot"></div>
      </button>
      <div class="n-drop" id="nDrop">
        <div class="n-hdr">Notifications</div>
        <div class="n-list" id="nList">
          <div class="n-empty">No new notifications</div>
        </div>
      </div>
    </div>
    <div class="cdot" id="cdot"></div>
    <a class="lout" href="/logout">Sign out</a>
  </div>
</div>

<div class="stack">

  <!-- ① Water Level -->
  <div class="card">
    <div class="ctitle">Water Level</div>
    <div class="twrap">
      <div class="tglass">
        <div class="tliq" id="tliq" style="height:0%">
          <div class="tsurf">
            <div class="twave"></div>
            <div class="twave"></div>
          </div>
        </div>
      </div>
      <div class="tinfo">
        <div>
          <div class="pv" id="pv">--</div>
          <div class="pl">Water Level</div>
        </div>
        <div class="pb"><div class="pf" id="pf" style="width:0%"></div></div>
        <div class="spill p-er" id="spill">
          <span class="pip"></span>
          <span id="stxt">Connecting&hellip;</span>
        </div>
      </div>
    </div>
  </div>

  <!-- ② Water Purity (donut) -->
  <div class="card">
    <div class="ctitle">Water Purity</div>
    <div class="pur-wrap">
      <!-- Donut -->
      <div class="pur-donut" style="width:130px;height:130px">
        <svg class="pur-svg" viewBox="0 0 36 36">
          <path class="pur-bg" d="M18 2.0845 a 15.9155 15.9155 0 0 1 0 31.831 a 15.9155 15.9155 0 0 1 0 -31.831"/>
          <path class="pur-fg" id="purFg" stroke="#1976D2" stroke-dasharray="0,100"
            d="M18 2.0845 a 15.9155 15.9155 0 0 1 0 31.831 a 15.9155 15.9155 0 0 1 0 -31.831"/>
        </svg>
        <div class="pur-inner">
          <div class="pur-num" id="purNum" style="color:#1976D2">--</div>
          <div class="pur-sub">PURITY</div>
        </div>
      </div>
      <!-- Badge + label -->
      <div class="pur-info">
        <div class="pur-badge pb-good" id="purBadge">Analyzing&hellip;</div>
        <div class="pur-label">Score based on pH + TDS</div>
      </div>
    </div>
    <!-- pH + TDS + Turbidity tiles -->
    <div class="sen-row">
      <div class="sen-tile">
        <div class="sen-lbl">pH Level</div>
        <div class="sen-val" id="phVal">--&nbsp;<span class="sen-unit">pH</span></div>
        <div class="sen-bar"><div class="sen-bar-fill ph-fill" id="phBar" style="width:50%"></div></div>
      </div>
      <div class="sen-tile">
        <div class="sen-lbl">TDS</div>
        <div class="sen-val" id="tdsVal">--&nbsp;<span class="sen-unit">ppm</span></div>
        <div class="sen-bar"><div class="sen-bar-fill tds-fill" id="tdsBar" style="width:0%"></div></div>
      </div>
      <div class="sen-tile">
        <div class="sen-lbl">Turbidity</div>
        <div class="sen-val" id="turbVal">--&nbsp;<span class="sen-unit">NTU</span></div>
        <div class="sen-bar"><div class="sen-bar-fill turb-fill" id="turbBar" style="width:0%"></div></div>
      </div>
    </div>
  </div>

  <!-- ③ Desired Water Level Slider -->
  <div class="card">
    <div class="ctitle">Desired Water Level</div>
    <div class="slider-wrap">
      <div class="slider-info">
        <div class="slider-lbl">Set Target Level</div>
        <div class="slider-sub">Adjust the desired water level for your tank</div>
      </div>
      <div>
        <div class="slider-val" id="sliderVal">75<span class="slider-pct">%</span></div>
      </div>
    </div>
    <input type="range" id="levelSlider" min="0" max="100" value="75"/>
    <div class="slider-ticks">
      <span>0%</span><span>25%</span><span>50%</span><span>75%</span><span>100%</span>
    </div>
  </div>

  <!-- ④ Level History -->
  <div class="card">
    <div class="ctitle">Level History</div>
    <div class="tabs">
      <div class="tab">Live &mdash; last 2 min</div>
    </div>
    <div class="chwrap">
      <canvas id="lvlChart"></canvas>
    </div>
    <hr/>
    <div class="frow">
      <span class="ftime" id="lastUpd">Last update: --</span>
      <button class="rbtn" onclick="fetchData()">&#8635; Refresh</button>
    </div>
  </div>

</div><!-- /stack -->

<script>
// ── Notifications ──
var nList = [];
function toggleN(e){
  e.stopPropagation();
  document.getElementById('nDrop').classList.toggle('show');
  document.getElementById('nDot').classList.remove('show');
}
window.onclick = function() { document.getElementById('nDrop').classList.remove('show'); };

function notify(title, text, type="info"){
  if(nList.length > 0 && nList[0].title === title) return; // Prevent spam
  var id = Date.now();
  nList.unshift({title, text, type, time: "Just now"});
  if(nList.length > 10) nList.pop();
  updateNUI();
  document.getElementById('nDot').classList.add('show');
}

function updateNUI(){
  var l = document.getElementById('nList');
  if(nList.length === 0){ l.innerHTML = '<div class="n-empty">No notifications</div>'; return; }
  l.innerHTML = nList.map(n => `
    <div class="n-item">
      <div class="n-icon ${n.type==='warn'?'ni-warn':n.type==='succ'?'ni-succ':'ni-info'}">
        ${n.type==='warn'?'&#9888;':n.type==='succ'?'&#10003;':'&#8505;'}
      </div>
      <div class="n-body">
        <div class="n-title">${n.title}</div>
        <div class="n-text">${n.text}</div>
        <div class="n-time">${n.time}</div>
      </div>
    </div>
  `).join('');
}
// ── Rolling buffers ──
var MAX=60;
var hTime=[],hLvl=[];

// ── Level chart ──
var lvlCtx=document.getElementById('lvlChart').getContext('2d');
var lvlGrad=lvlCtx.createLinearGradient(0,0,0,180);
lvlGrad.addColorStop(0,'rgba(25,118,210,0.22)');
lvlGrad.addColorStop(1,'rgba(25,118,210,0.0)');
var lvlChart=new Chart(lvlCtx,{
  type:'line',
  data:{labels:hTime,datasets:[{
    label:'Level %',data:hLvl,
    borderColor:'#1976D2',backgroundColor:lvlGrad,
    borderWidth:2.5,tension:0.38,fill:true,
    pointRadius:0,pointHoverRadius:5,
    pointHoverBackgroundColor:'#1976D2'
  }]},
  options:{
    responsive:true,maintainAspectRatio:false,
    plugins:{legend:{display:false},
      tooltip:{backgroundColor:'#fff',titleColor:'#2C3E50',bodyColor:'#7F8C8D',
        borderColor:'#E5E7EB',borderWidth:1,padding:10,displayColors:false,
        callbacks:{label:function(c){return c.parsed.y.toFixed(1)+' %';}}}},
    scales:{
      y:{min:0,max:100,grid:{color:'#F3F4F6'},
        ticks:{color:'#9CA3AF',font:{size:10},callback:function(v){return v+'%';}}},
      x:{grid:{display:false},
        ticks:{color:'#9CA3AF',font:{size:9},maxTicksLimit:8}}},
    animation:{duration:400}
  }
});

// ── Status pill ──
function setStat(pct,err){
  var s=document.getElementById('spill');
  var t=document.getElementById('stxt');
  s.className='spill';
  if(err){s.classList.add('p-er');t.textContent='Sensor Error';return;}
  if(pct>=90){s.classList.add('p-hi');t.textContent='FULL - Critical High';return;}
  if(pct>=70){s.classList.add('p-ok');t.textContent='High';return;}
  if(pct>=30){s.classList.add('p-ok');t.textContent='Normal';return;}
  if(pct>=10){s.classList.add('p-lo');t.textContent='Low';return;}
  s.classList.add('p-hi');t.textContent='EMPTY - Critically Low';
}

// ── Purity helpers ──
function calcPurity(ph,tds,turb){
  var s=100;
  if(ph<6.5||ph>8.5) s-=20; else if(ph<6.8||ph>7.5) s-=5;
  if(tds>900) s-=25; else if(tds>600) s-=15; else if(tds>300) s-=5;
  if(turb>3000) s-=25; else if(turb>1000) s-=15; else if(turb>500) s-=5;
  return Math.max(0,Math.min(100,s));
}

function setPurDonut(score){
  var fg=document.getElementById('purFg');
  var num=document.getElementById('purNum');
  var badge=document.getElementById('purBadge');
  fg.setAttribute('stroke-dasharray',score.toFixed(1)+',100');
  var col=score>=70?'#22c55e':score>=50?'#F59E0B':'#EF4444';
  fg.setAttribute('stroke',col);
  num.style.color=col;
  num.textContent=score.toFixed(0)+'%';
  badge.className='pur-badge';
  if(score>=85){badge.classList.add('pb-great');badge.innerHTML='\u2705 Excellent';}
  else if(score>=70){badge.classList.add('pb-good');badge.innerHTML='\ud83d\udfe2 Good';}
  else if(score>=50){badge.classList.add('pb-fair');badge.innerHTML='\u26a0\ufe0f Fair';}
  else{badge.classList.add('pb-poor');badge.innerHTML='\u274c Poor';}
}

// ── Slider ──
var slider=document.getElementById('levelSlider');
var sliderDisp=document.getElementById('sliderVal');
var sliderTimer=null;
slider.addEventListener('input',function(){
  sliderDisp.innerHTML=this.value+'<span class="slider-pct">%</span>';
  clearTimeout(sliderTimer);
  sliderTimer=setTimeout(function(){
    fetch('/setlevel?v='+slider.value);
  },400);
});

// ── Fetch ──
function fetchData(){
  fetch('/data')
    .then(function(r){
      if(r.status===401){window.location.href='/';return null;}
      return r.json();
    })
    .then(function(d){if(d)updateUI(d);})
    .catch(function(){
      document.getElementById('cdot').className='cdot';
      document.getElementById('stxt').textContent='Connection lost';
    });
}

function updateUI(d){
  var pct=Math.max(0,Math.min(100,d.levelPercent));
  var ph=d.ph||7.0, tds=d.tds||0, turb=d.turb||0;
  var pur=calcPurity(ph,tds,turb);

  document.getElementById('cdot').className='cdot on';

  // Tank
  document.getElementById('tliq').style.height=pct+'%';
  document.getElementById('pv').textContent=pct.toFixed(1)+'%';
  document.getElementById('pf').style.width=pct+'%';
  setStat(pct,d.sensorError);

  // Logic notifications
  if(pct >= 99) notify("Tank is Full", "The tank is completely full (100%).", "succ");
  else if(pct <= 1) notify("Tank is Empty", "The tank is empty. Please fill it immediately.", "warn");
  else if(pct >= d.desiredLevel && d.pumpActive === false) 
    notify("Desired Level Reached", "Water level has successfully reached your desired threshold of " + d.desiredLevel.toFixed(0) + "%.", "info");

  // Purity donut
  setPurDonut(pur);

  // pH tile
  document.getElementById('phVal').innerHTML=ph.toFixed(2)+'&nbsp;<span class="sen-unit">pH</span>';
  document.getElementById('phBar').style.width=(ph/14*100).toFixed(1)+'%';

  // TDS tile
  document.getElementById('tdsVal').innerHTML=tds.toFixed(0)+'&nbsp;<span class="sen-unit">ppm</span>';
  document.getElementById('tdsBar').style.width=Math.min(tds/1000*100,100).toFixed(1)+'%';

  // Turbidity tile
  document.getElementById('turbVal').innerHTML=turb.toFixed(0)+'&nbsp;<span class="sen-unit">NTU</span>';
  document.getElementById('turbBar').style.width=Math.min(turb/3000*100,100).toFixed(1)+'%';

  // Sync slider if server has a different value
  if(d.desiredLevel!==undefined && !sliderTimer){
    slider.value=d.desiredLevel;
    sliderDisp.innerHTML=d.desiredLevel.toFixed(0)+'<span class="slider-pct">%</span>';
  }

  // Rolling history
  var now=new Date().toLocaleTimeString([],{hour:'2-digit',minute:'2-digit',second:'2-digit'});
  hTime.push(now);
  hLvl.push(parseFloat(pct.toFixed(2)));
  if(hTime.length>MAX){hTime.shift();hLvl.shift();}
  lvlChart.update();

  document.getElementById('lastUpd').textContent='Last update: '+now;
}

fetchData();
setInterval(fetchData,2000);
</script>
</body></html>
)rawliteral";

// ════════════════════════════════════════════════════════════
//  AUTH HELPERS
// ════════════════════════════════════════════════════════════
bool isAuthenticated() {
  if (server.hasHeader("Cookie")) {
    String c = server.header("Cookie");
    return c.indexOf(String("ww_session=") + SESSION_TOKEN) != -1;
  }
  return false;
}
void redirectToLogin() {
  server.sendHeader("Location", "/");
  server.send(302, "text/plain", "");
}

// ════════════════════════════════════════════════════════════
//  ROUTE HANDLERS
// ════════════════════════════════════════════════════════════
void handleLoginPage() {
  server.send_P(200, "text/html", LOGIN_HTML);
}

void handleLoginPost() {
  String u = server.arg("username");
  String p = server.arg("passwd");
  if (u == AUTH_USER && p == AUTH_PASS) {
    server.sendHeader("Set-Cookie",
      String("ww_session=") + SESSION_TOKEN + "; Path=/; HttpOnly");
    server.sendHeader("Location", "/dashboard");
    server.send(302, "text/plain", "");
  } else {
    server.sendHeader("Location", "/?err=1");
    server.send(302, "text/plain", "");
  }
}

void handleDashboard() {
  if (!isAuthenticated()) { redirectToLogin(); return; }
  server.send_P(200, "text/html", DASH_HTML);
}

void handleData() {
  if (!isAuthenticated()) {
    server.send(401, "application/json", "{\"error\":\"unauthorized\"}");
    return;
  }
  bool  sensorError  = (distance_cm < 0);
  float emptySpace   = max(0.0f, distance_cm - FULL_DISTANCE_CM);
  float usableTankCm = EMPTY_DISTANCE_CM - FULL_DISTANCE_CM;

  String json = "{";
  json += "\"levelPercent\":"   + String(levelPercent, 2)             + ",";
  json += "\"distance_cm\":"    + String(distance_cm, 2)              + ",";
  json += "\"waterLevel_cm\":"  + String(waterLevel_cm, 2)            + ",";
  json += "\"tankDepth_cm\":"   + String(usableTankCm, 1)             + ",";
  json += "\"emptySpace_cm\":"  + String(emptySpace, 2)               + ",";
  json += "\"ph\":"             + String(currentPH, 2)                + ",";
  json += "\"tds\":"            + String(currentTDS, 0)               + ",";
  json += "\"turb\":"           + String(currentTurb, 0)              + ",";
  json += "\"desiredLevel\":"   + String(desiredLevel, 1)             + ",";
  json += "\"pumpActive\":"     + String(pumpActive ? "true" : "false") + ",";
  json += "\"sensorError\":"    + String(sensorError ? "true" : "false");
  json += "}";
  server.send(200, "application/json", json);
}

void handleLogout() {
  server.sendHeader("Set-Cookie",
    "ww_session=deleted; Path=/; Expires=Thu, 01 Jan 1970 00:00:00 GMT; HttpOnly");
  server.sendHeader("Location", "/");
  server.send(302, "text/plain", "");
}

void handleSetLevel() {
  if (!isAuthenticated()) { server.send(401, "text/plain", "unauthorized"); return; }
  if (server.hasArg("v")) {
    desiredLevel = constrain(server.arg("v").toFloat(), 0.0f, 100.0f);
    Serial.printf("[SYSTEM] Desired level updated: %.1f%%\n", desiredLevel);
  }
  server.send(200, "text/plain", "ok");
}

void handlePump() {
  if (!isAuthenticated()) { server.send(401, "text/plain", "unauthorized"); return; }
  if (server.hasArg("mode")) {
    pumpAuto = (server.arg("mode") == "auto");
    Serial.printf("[PUMP] Mode: %s\n", pumpAuto ? "AUTO" : "MANUAL");
    if (pumpAuto) pumpActive = false; // Reset to safe state on mode change
  }
  if (server.hasArg("state") && !pumpAuto) {
    pumpActive = (server.arg("state") == "on");
    Serial.printf("[PUMP] Manual State: %s\n", pumpActive ? "ON" : "OFF");
  }
  server.send(200, "application/json", "{\"success\":true}");
}

// ════════════════════════════════════════════════════════════
//  DISTANCE MEASUREMENT
// ════════════════════════════════════════════════════════════
float measureDistance() {
  float sum = 0; int valid = 0;
  for (int i = 0; i < 3; i++) {
    digitalWrite(TRIG_PIN, LOW);  delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH); delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);
    long dur = pulseIn(ECHO_PIN, HIGH, 30000);
    if (dur > 0) { sum += (dur * soundSpeed_cm_us) / 2.0; valid++; }
    delay(60);
  }
  return valid == 0 ? -1.0f : sum / valid;
}

// ════════════════════════════════════════════════════════════
//  SETUP
// ════════════════════════════════════════════════════════════
void setup() {
  Serial.begin(115200);
  analogReadResolution(12);

  float v_air_ms   = 331.5f + 0.606f * AIR_TEMP_C;
  soundSpeed_cm_us = v_air_ms / 10000.0f;
  Serial.printf("Sound speed: %.2f m/s\n", v_air_ms);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(PH_PIN,   INPUT);
  pinMode(TDS_PIN,  INPUT);
  pinMode(TURB_PIN, INPUT);
  pinMode(PUMP_PIN, OUTPUT);
  digitalWrite(PUMP_PIN, PUMP_OFF); // Power-up state: OFF

  // Init buffers
  for (int i = 0; i < SCOUNT; i++) { phBuffer[i] = 0; tdsBuffer[i] = 0; turbBuffer[i] = 0; }

  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println();
  Serial.print("IP: "); Serial.println(WiFi.localIP());

  const char *hdrs[] = {"Cookie"};
  server.collectHeaders(hdrs, 1);

  server.on("/",          HTTP_GET,  handleLoginPage);
  server.on("/login",     HTTP_POST, handleLoginPost);
  server.on("/dashboard", HTTP_GET,  handleDashboard);
  server.on("/data",      HTTP_GET,  handleData);
  server.on("/setlevel",  HTTP_GET,  handleSetLevel);
  server.on("/pump",      HTTP_POST, handlePump);
  server.on("/logout",    HTTP_GET,  handleLogout);

  server.begin();
  Serial.println("Server started — login: user / password");
}

// ════════════════════════════════════════════════════════════
//  LOOP
// ════════════════════════════════════════════════════════════
void loop() {
  server.handleClient();

  // ── Fast sampling (every 40 ms) ──
  static unsigned long sampleT = 0;
  if (millis() - sampleT > 40U) {
    sampleT = millis();
    phBuffer[bufIndex]   = analogRead(PH_PIN);
    tdsBuffer[bufIndex]  = analogRead(TDS_PIN);
    turbBuffer[bufIndex] = analogRead(TURB_PIN);
    bufIndex = (bufIndex + 1) % SCOUNT;
  }

  // ── Compute + ultrasonic (every 1 s) ──
  static unsigned long compT = 0;
  if (millis() - compT >= 1000U) {
    compT = millis();

    // pH
    int pb[SCOUNT]; for (int i = 0; i < SCOUNT; i++) pb[i] = phBuffer[i];
    lastPhVoltage = getMedianNum(pb, SCOUNT) * VREF / ADC_RES;
    float slope   = (7.0f - 4.0f) / (voltageAtPH7 - voltageAtPH4);
    currentPH     = constrain(7.0f + (lastPhVoltage - voltageAtPH7) * slope, 0.0f, 14.0f);

    // TDS
    int tb[SCOUNT]; for (int i = 0; i < SCOUNT; i++) tb[i] = tdsBuffer[i];
    lastTdsVoltage = getMedianNum(tb, SCOUNT) * VREF / ADC_RES;
    float cv = lastTdsVoltage;  // no temperature compensation
    currentTDS = (133.42f * pow(cv,3) - 255.86f * pow(cv,2) + 857.39f * cv) * tdsCalFactor;

    // Turbidity
    int trb[SCOUNT]; for (int i = 0; i < SCOUNT; i++) trb[i] = turbBuffer[i];
    lastTurbVoltage = getMedianNum(trb, SCOUNT) * VREF / ADC_RES;
    currentTurb = (3.3f - lastTurbVoltage) * 1000.0f; // Simplified NTU proxy
    if (currentTurb < 0) currentTurb = 0;

    // Ultrasonic
    float raw = measureDistance();
    if (raw < 0) {
      distance_cm = -1; waterLevel_cm = 0; levelPercent = 0;
      Serial.println("[ERROR] No echo.");
    } else if (raw <= MIN_SENSOR_CM) {
      distance_cm = raw;
      waterLevel_cm = EMPTY_DISTANCE_CM - FULL_DISTANCE_CM;
      levelPercent  = 100.0f;
      Serial.printf("[FULL] %.1f cm\n", raw);
    } else if (raw >= EMPTY_DISTANCE_CM) {
      distance_cm = raw; waterLevel_cm = 0; levelPercent = 0;
      Serial.printf("[EMPTY] %.1f cm\n", raw);
    } else {
      distance_cm = raw;
      float range = EMPTY_DISTANCE_CM - FULL_DISTANCE_CM;
      waterLevel_cm = EMPTY_DISTANCE_CM - raw;
      levelPercent  = constrain((waterLevel_cm / range) * 100.0f, 0.0f, 100.0f);
    }

    Serial.printf("Dist:%.1fcm Lvl:%.1f%% pH:%.2f TDS:%.0fppm\n",
                  distance_cm, levelPercent, currentPH, currentTDS);

    // ── Pump Control Logic ──
    static bool lastP = false;
    
    if (pumpAuto) {
      // Auto Mode: ON below lowTh, OFF above highTh (Desired Level)
      if (levelPercent <= lowTh) {
        pumpActive = true;
      } else if (levelPercent >= desiredLevel) {
        pumpActive = false;
      }
    }

    if (pumpActive != lastP) {
      Serial.printf("[PUMP] %s | Mode: %s | Level: %.1f%%\n",
                    pumpActive ? "STARTED" : "STOPPED", 
                    pumpAuto ? "AUTO" : "MANUAL", levelPercent);
      lastP = pumpActive;
    }

    digitalWrite(PUMP_PIN, pumpActive ? PUMP_ON : PUMP_OFF);
  }
}
