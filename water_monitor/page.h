// page.h — Complete Water Quality Dashboard
#ifndef PAGE_H
#define PAGE_H

const char MAIN_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html lang="en"><head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1.0">
<title>Water Quality Monitor</title>
<link rel="preconnect" href="https://fonts.googleapis.com">
<link href="https://fonts.googleapis.com/css2?family=Inter:wght@300;400;500;600;700;800&display=swap" rel="stylesheet">
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:'Inter',system-ui,sans-serif;background:#080c1a;min-height:100vh;color:#e2e8f0;overflow-x:hidden}
.bg{position:fixed;inset:0;z-index:0;pointer-events:none}
.bg .orb{position:absolute;border-radius:50%;filter:blur(100px);animation:drift 25s infinite ease-in-out}
.orb.o1{width:420px;height:420px;background:rgba(96,165,250,.12);top:-5%;left:15%}
.orb.o2{width:500px;height:500px;background:rgba(167,139,250,.10);bottom:-10%;right:5%;animation-delay:8s}
.orb.o3{width:300px;height:300px;background:rgba(34,211,238,.08);top:45%;left:55%;animation-delay:16s}
@keyframes drift{0%,100%{transform:translateY(0) scale(1)}50%{transform:translateY(-40px) scale(1.08)}}
.wrap{position:relative;z-index:1;max-width:1060px;margin:0 auto;padding:20px 16px}
.hdr{text-align:center;padding:22px 0 6px}
.hdr h1{font-size:1.8rem;font-weight:800;background:linear-gradient(135deg,#60a5fa,#c084fc);-webkit-background-clip:text;-webkit-text-fill-color:transparent}
.hdr .sub{margin-top:4px;font-size:.78rem;color:#64748b}
.sbar{display:flex;align-items:center;justify-content:center;gap:8px;margin:8px 0 22px;font-size:.78rem;color:#94a3b8}
.dot{width:7px;height:7px;border-radius:50%;background:#4ade80;animation:pulse 2s infinite}
.dot.off{background:#ef4444;animation:none}
@keyframes pulse{0%,100%{opacity:1}50%{opacity:.4}}

/* cards */
.cards{display:grid;grid-template-columns:repeat(4,1fr);gap:16px}
.card{background:rgba(255,255,255,.025);border:1px solid rgba(255,255,255,.06);border-radius:20px;padding:24px 16px 20px;text-align:center;position:relative;overflow:hidden;transition:transform .3s,box-shadow .3s}
.card::before{content:'';position:absolute;top:0;left:0;right:0;height:2px;background:var(--ac);opacity:.5}
.card:hover{transform:translateY(-4px);box-shadow:0 16px 32px rgba(0,0,0,.3)}
.ct{--ac:#fb923c}.cp{--ac:#22d3ee}.cd{--ac:#a78bfa}.cl{--ac:#3b82f6}
.gc{position:relative;width:120px;height:120px;margin:0 auto 12px}
.gc svg{width:120px;height:120px;transform:rotate(-90deg)}
.gbg{fill:none;stroke:var(--ac);stroke-width:5;opacity:.10}
.gf{fill:none;stroke:var(--ac);stroke-width:5;stroke-linecap:round;transition:stroke-dashoffset .9s cubic-bezier(.4,0,.2,1)}
.gv{position:absolute;top:50%;left:50%;transform:translate(-50%,-50%);text-align:center}
.gv .n{font-size:1.6rem;font-weight:700;color:#fff;line-height:1}
.gv .u{font-size:.68rem;color:#94a3b8;margin-top:2px}
.lbl{font-size:.72rem;font-weight:600;color:#cbd5e1;text-transform:uppercase;letter-spacing:1.5px;margin-bottom:6px}
.badge{display:inline-block;padding:2px 12px;border-radius:16px;font-size:.65rem;font-weight:700;text-transform:uppercase;letter-spacing:.5px}
.bg-g{background:rgba(74,222,128,.12);color:#4ade80}
.bg-w{background:rgba(250,204,21,.12);color:#facc15}
.bg-b{background:rgba(239,68,68,.12);color:#ef4444}
.bg-i{background:rgba(96,165,250,.12);color:#60a5fa}
.rv{font-size:.65rem;color:#475569;margin-top:4px}

/* info row: pump + analysis */
.irow{display:grid;grid-template-columns:1fr 1fr;gap:16px;margin-top:16px}
.icard{background:rgba(255,255,255,.025);border:1px solid rgba(255,255,255,.06);border-radius:20px;padding:22px 20px}
.icard h3{font-size:.82rem;font-weight:700;color:#cbd5e1;text-transform:uppercase;letter-spacing:1px;margin-bottom:14px}

/* pump */
.pump-ind{display:flex;align-items:center;gap:10px;margin-bottom:14px}
.pump-dot{width:12px;height:12px;border-radius:50%;transition:all .3s}
.pump-dot.on{background:#4ade80;box-shadow:0 0 12px rgba(74,222,128,.5)}
.pump-dot.off{background:#ef4444;box-shadow:0 0 12px rgba(239,68,68,.3)}
.pump-st{font-size:1rem;font-weight:700}
.pump-btns{display:flex;gap:8px;flex-wrap:wrap}
.pbtn{padding:7px 16px;border-radius:10px;border:1px solid rgba(255,255,255,.1);background:rgba(255,255,255,.04);color:#94a3b8;font-size:.78rem;font-weight:600;cursor:pointer;transition:all .3s;font-family:inherit}
.pbtn:hover{border-color:rgba(255,255,255,.2);color:#e2e8f0}
.pbtn.act{background:rgba(96,165,250,.15);border-color:rgba(96,165,250,.3);color:#60a5fa}
.pump-info{font-size:.72rem;color:#475569;margin-top:10px}

/* analysis */
.score-wrap{display:flex;align-items:center;gap:16px;margin-bottom:14px}
.score-ring{position:relative;width:64px;height:64px;flex-shrink:0}
.score-ring svg{width:64px;height:64px;transform:rotate(-90deg)}
.score-ring .sbg{fill:none;stroke:rgba(255,255,255,.06);stroke-width:5}
.score-ring .sf{fill:none;stroke-width:5;stroke-linecap:round;transition:stroke .4s,stroke-dashoffset .9s}
.score-num{position:absolute;top:50%;left:50%;transform:translate(-50%,-50%);font-size:1.2rem;font-weight:800;color:#fff}
.score-label{font-size:.85rem;font-weight:600;color:#e2e8f0}
.score-sub{font-size:.72rem;color:#64748b;margin-top:2px}
.sug{display:flex;align-items:flex-start;gap:8px;padding:6px 0;font-size:.78rem;border-bottom:1px solid rgba(255,255,255,.04)}
.sug:last-child{border:none}
.sug-icon{flex-shrink:0;font-size:.85rem;margin-top:1px}
.sug-t{color:#94a3b8}

/* chart */
.chcard{background:rgba(255,255,255,.025);border:1px solid rgba(255,255,255,.06);border-radius:20px;padding:20px;margin-top:16px}
.chtabs{display:flex;gap:6px;margin-bottom:14px;flex-wrap:wrap}
.chtab{padding:6px 16px;border-radius:10px;border:1px solid rgba(255,255,255,.08);background:transparent;color:#64748b;font-size:.75rem;font-weight:600;cursor:pointer;transition:all .3s;font-family:inherit}
.chtab.act{background:rgba(96,165,250,.12);border-color:rgba(96,165,250,.25);color:#60a5fa}
.chwrap{position:relative;width:100%;height:200px}
.chwrap canvas{position:absolute;top:0;left:0}

/* calibration */
.caltog{display:block;margin:20px auto 0;background:none;border:1px solid rgba(255,255,255,.08);color:#64748b;padding:9px 22px;border-radius:12px;cursor:pointer;font-size:.8rem;font-weight:500;transition:all .3s;font-family:inherit}
.caltog:hover{border-color:rgba(255,255,255,.2);color:#e2e8f0}
.calpan{max-height:0;overflow:hidden;transition:max-height .5s}
.calpan.open{max-height:1200px}
.calinn{background:rgba(255,255,255,.02);border:1px solid rgba(255,255,255,.05);border-radius:18px;padding:22px 20px;margin-top:16px}
.calinn h3{font-size:.88rem;font-weight:700;text-align:center;margin-bottom:16px}
.calsec{margin-bottom:16px;padding:14px;border-radius:12px;background:rgba(255,255,255,.02);border-left:3px solid var(--sc)}
.calst{font-size:.72rem;font-weight:700;color:#cbd5e1;text-transform:uppercase;letter-spacing:.8px;margin-bottom:10px}
.calrow{display:grid;grid-template-columns:1fr 1fr;gap:10px}
.calf{display:flex;flex-direction:column;gap:3px;margin-bottom:8px}
.calf label{font-size:.68rem;color:#94a3b8;text-transform:uppercase;letter-spacing:.6px;font-weight:600}
.calf input{background:rgba(255,255,255,.05);border:1px solid rgba(255,255,255,.08);border-radius:8px;padding:8px 12px;color:#e2e8f0;font-size:.82rem;font-family:inherit;outline:none;width:100%}
.calf input:focus{border-color:#60a5fa}
.calh{font-size:.62rem;color:#475569}
.rlive{font-size:1rem;font-weight:700;color:#60a5fa;padding:4px 0}
.calbtns{display:flex;gap:10px;justify-content:center;margin-top:16px}
.cbtn{padding:9px 24px;border-radius:10px;border:none;font-weight:600;font-size:.82rem;cursor:pointer;transition:all .3s;font-family:inherit}
.cbtn-s{background:#4ade80;color:#0f172a}.cbtn-s:hover{background:#22c55e}
.cbtn-r{background:rgba(255,255,255,.05);color:#94a3b8;border:1px solid rgba(255,255,255,.08)}.cbtn-r:hover{color:#e2e8f0}
.toast{position:fixed;bottom:20px;left:50%;transform:translateX(-50%) translateY(60px);background:#1e293b;border:1px solid rgba(74,222,128,.3);color:#4ade80;padding:10px 24px;border-radius:12px;font-size:.82rem;font-weight:600;opacity:0;transition:all .4s;pointer-events:none;z-index:99}
.toast.show{opacity:1;transform:translateX(-50%) translateY(0)}
.foot{margin-top:20px;text-align:center;font-size:.7rem;color:#334155}
.foot span{color:#94a3b8;font-weight:600}

@media(max-width:900px){.cards{grid-template-columns:repeat(2,1fr)}}
@media(max-width:600px){
  .cards{grid-template-columns:1fr 1fr;gap:10px}
  .irow{grid-template-columns:1fr}
  .hdr h1{font-size:1.4rem}
  .gc{width:100px;height:100px}.gc svg{width:100px;height:100px}
  .gv .n{font-size:1.3rem}.card{padding:18px 12px 16px}
  .calrow{grid-template-columns:1fr}
}
</style></head><body>
<div class="bg"><div class="orb o1"></div><div class="orb o2"></div><div class="orb o3"></div></div>
<div class="wrap">
<div class="hdr"><h1>&#x1F4A7; Water Quality Monitor</h1><div class="sub">ESP32 &bull; Real-time Sensor Dashboard</div></div>
<div class="sbar"><div class="dot" id="dot"></div><span id="sts">Connecting&hellip;</span><span>&bull;</span><span id="upd">--</span></div>

<div class="cards">
<div class="card ct"><div class="gc"><svg viewBox="0 0 120 120"><circle class="gbg" cx="60" cy="60" r="54"/><circle class="gf" id="tG" cx="60" cy="60" r="54" stroke-dasharray="339.3" stroke-dashoffset="339.3"/></svg><div class="gv"><div class="n" id="tV">--</div><div class="u">&deg;C</div></div></div><div class="lbl">Temperature</div><span class="badge bg-i" id="tB">--</span></div>
<div class="card cp"><div class="gc"><svg viewBox="0 0 120 120"><circle class="gbg" cx="60" cy="60" r="54"/><circle class="gf" id="pG" cx="60" cy="60" r="54" stroke-dasharray="339.3" stroke-dashoffset="339.3"/></svg><div class="gv"><div class="n" id="pV">--</div><div class="u">pH</div></div></div><div class="lbl">pH Level</div><span class="badge bg-i" id="pB">--</span><div class="rv" id="phRV"></div></div>
<div class="card cd"><div class="gc"><svg viewBox="0 0 120 120"><circle class="gbg" cx="60" cy="60" r="54"/><circle class="gf" id="dG" cx="60" cy="60" r="54" stroke-dasharray="339.3" stroke-dashoffset="339.3"/></svg><div class="gv"><div class="n" id="dV">--</div><div class="u">ppm</div></div></div><div class="lbl">TDS</div><span class="badge bg-i" id="dB">--</span><div class="rv" id="tdsRV"></div></div>
<div class="card cl"><div class="gc"><svg viewBox="0 0 120 120"><circle class="gbg" cx="60" cy="60" r="54"/><circle class="gf" id="lG" cx="60" cy="60" r="54" stroke-dasharray="339.3" stroke-dashoffset="339.3"/></svg><div class="gv"><div class="n" id="lV">--</div><div class="u">%</div></div></div><div class="lbl">Water Level</div><span class="badge bg-i" id="lB">--</span></div>
</div>

<!-- Pump + Analysis -->
<div class="irow">
<div class="icard">
<h3>&#x1F50C; Pump Control</h3>
<div class="pump-ind"><div class="pump-dot off" id="pDot"></div><span class="pump-st" id="pSt">OFF</span></div>
<div class="pump-btns">
<button class="pbtn act" id="btnAuto" onclick="setMode('auto')">&#9881; Auto</button>
<button class="pbtn" id="btnMan" onclick="setMode('manual')">&#9997; Manual</button>
<button class="pbtn" id="btnTog" onclick="togPump()" style="display:none">&#9654; Turn ON</button>
</div>
<div class="pump-info" id="pInfo">Auto mode: pump starts when level &lt; low threshold and stops when &gt; high threshold.</div>
</div>
<div class="icard">
<h3>&#x1F4CA; Water Quality Analysis</h3>
<div class="score-wrap">
<div class="score-ring"><svg viewBox="0 0 64 64"><circle class="sbg" cx="32" cy="32" r="28"/><circle class="sf" id="sRing" cx="32" cy="32" r="28" stroke-dasharray="175.9" stroke-dashoffset="175.9"/></svg><div class="score-num" id="sNum">--</div></div>
<div><div class="score-label" id="sLbl">Analyzing...</div><div class="score-sub" id="sSub">Waiting for data</div></div>
</div>
<div id="sugList"></div>
</div>
</div>

<!-- Chart -->
<div class="chcard">
<div class="chtabs">
<button class="chtab act" onclick="stab(0,this)">pH</button>
<button class="chtab" onclick="stab(1,this)">TDS</button>
<button class="chtab" onclick="stab(2,this)">Temp</button>
<button class="chtab" onclick="stab(3,this)">Level</button>
</div>
<div class="chwrap" id="chWrap"><canvas id="chart"></canvas></div>
</div>

<!-- Calibration -->
<button class="caltog" onclick="togCal()">&#9881; Calibration &amp; Tank Settings</button>
<div class="calpan" id="calP">
<div class="calinn"><h3>&#128295; Settings</h3>
<div class="calsec" style="--sc:#22d3ee"><div class="calst">&#9879; pH Sensor</div>
<div class="calf"><label>Live Raw Voltage</label><div class="rlive" id="cpV">--</div></div>
<div class="calrow"><div class="calf"><label>Voltage at pH 7.0</label><input type="number" id="cP7" step="0.001"><div class="calh">Distilled water / pH 7 buffer</div></div>
<div class="calf"><label>Voltage at pH 4.0</label><input type="number" id="cP4" step="0.001"><div class="calh">pH 4 buffer solution</div></div></div></div>
<div class="calsec" style="--sc:#a78bfa"><div class="calst">&#128142; TDS Sensor</div>
<div class="calf"><label>Live Raw Voltage</label><div class="rlive" id="ctV">--</div></div>
<div class="calf"><label>Calibration Factor</label><input type="number" id="cTF" step="0.01"><div class="calh">Default 0.50</div></div></div>
<div class="calsec" style="--sc:#fb923c"><div class="calst">&#127777; Temperature</div>
<div class="calf"><label>Manual Override (&deg;C)</label><input type="number" id="cTe" step="0.1"><div class="calh">Used if DS18B20 not detected</div></div></div>
<div class="calsec" style="--sc:#3b82f6"><div class="calst">&#128166; Tank &amp; Pump</div>
<div class="calf"><label>Live Sensor Distance</label><div class="rlive" id="cLvlD">--</div><div class="calh">Mount JSN-SR04T &gt;25cm above max water!</div></div>
<div class="calrow"><div class="calf"><label>Empty Dist (cm)</label><input type="number" id="cEmD" step="0.1"><div class="calh">Sensor to bottom</div></div>
<div class="calf"><label>Full Dist (cm)</label><input type="number" id="cFuD" step="0.1"><div class="calh">Sensor to full level</div></div></div>
<div class="calrow"><div class="calf"><label>Low Thresh (%)</label><input type="number" id="cLo" step="1"><div class="calh">Pump ON below</div></div>
<div class="calf"><label>High Thresh (%)</label><input type="number" id="cHi" step="1"><div class="calh">Pump OFF above</div></div></div></div>
<div class="calbtns"><button class="cbtn cbtn-s" onclick="saveCal()">&#128190; Save</button><button class="cbtn cbtn-r" onclick="resetCal()">&#8635; Reset</button></div>
</div></div>
<div class="foot">ESP32 Water Quality Monitor &bull; <span id="ipAddr">--</span></div>
</div>
<div class="toast" id="toast"></div>

<script>
var C=2*Math.PI*54,SC=2*Math.PI*28,curTab=0,hist=[];

var sg=function(id,p){document.getElementById(id).style.strokeDashoffset=C*(1-Math.min(Math.max(p,0),1))};
var bc=function(el,c){el.className='badge '+c};
var toastMsg=function(m){var t=document.getElementById('toast');t.textContent=m;t.className='toast show';setTimeout(function(){t.className='toast'},3000)};

/* ---- Quality Score ---- */
var calcScore=function(ph,tds,tmp,lvl){
  var s=100;
  if(ph<6.5||ph>8.5)s-=25;else if(ph<6.8||ph>7.5)s-=10;
  if(tds>900)s-=30;else if(tds>600)s-=20;else if(tds>300)s-=10;
  if(tmp<10||tmp>40)s-=15;else if(tmp<15||tmp>35)s-=5;
  if(lvl<20)s-=15;else if(lvl<40)s-=5;
  return Math.max(0,Math.min(100,s));
};

var updateAnalysis=function(d){
  var s=calcScore(d.ph,d.tds,d.temp,d.lvl);
  document.getElementById('sNum').textContent=s;
  var ring=document.getElementById('sRing');
  ring.style.strokeDashoffset=SC*(1-s/100);
  var col=s>=80?'#4ade80':s>=60?'#facc15':s>=40?'#fb923c':'#ef4444';
  ring.style.stroke=col;
  var lbl=s>=80?'Good':s>=60?'Fair':s>=40?'Poor':'Critical';
  document.getElementById('sLbl').textContent=lbl+' Quality';
  document.getElementById('sSub').textContent='Score: '+s+'/100';

  var sug='';
  // pH
  if(d.ph>=6.5&&d.ph<=7.5)sug+='<div class="sug"><span class="sug-icon">&#9989;</span><span class="sug-t">pH '+d.ph.toFixed(2)+' — Neutral, ideal for drinking</span></div>';
  else if(d.ph>=6.5&&d.ph<=8.5)sug+='<div class="sug"><span class="sug-icon">&#9989;</span><span class="sug-t">pH '+d.ph.toFixed(2)+' — Acceptable range</span></div>';
  else if(d.ph<6.5)sug+='<div class="sug"><span class="sug-icon">&#9888;</span><span class="sug-t">pH '+d.ph.toFixed(2)+' — Acidic, may corrode pipes</span></div>';
  else sug+='<div class="sug"><span class="sug-icon">&#9888;</span><span class="sug-t">pH '+d.ph.toFixed(2)+' — Alkaline, may taste bitter</span></div>';
  // TDS
  if(d.tds<300)sug+='<div class="sug"><span class="sug-icon">&#9989;</span><span class="sug-t">TDS '+Math.round(d.tds)+' ppm — Good purity</span></div>';
  else if(d.tds<600)sug+='<div class="sug"><span class="sug-icon">&#9888;</span><span class="sug-t">TDS '+Math.round(d.tds)+' ppm — Moderate, consider filter</span></div>';
  else sug+='<div class="sug"><span class="sug-icon">&#10060;</span><span class="sug-t">TDS '+Math.round(d.tds)+' ppm — High, filtration recommended</span></div>';
  // Temp
  if(d.temp>=15&&d.temp<=35)sug+='<div class="sug"><span class="sug-icon">&#9989;</span><span class="sug-t">Temp '+d.temp.toFixed(1)+'&deg;C — Normal range</span></div>';
  else sug+='<div class="sug"><span class="sug-icon">&#9888;</span><span class="sug-t">Temp '+d.temp.toFixed(1)+'&deg;C — Outside ideal range</span></div>';
  // Level
  if(d.lvl>40)sug+='<div class="sug"><span class="sug-icon">&#9989;</span><span class="sug-t">Water level '+d.lvl.toFixed(0)+'% — Sufficient</span></div>';
  else if(d.lvl>20)sug+='<div class="sug"><span class="sug-icon">&#9888;</span><span class="sug-t">Water level '+d.lvl.toFixed(0)+'% — Getting low</span></div>';
  else sug+='<div class="sug"><span class="sug-icon">&#10060;</span><span class="sug-t">Water level '+d.lvl.toFixed(0)+'% — Critical! Pump active</span></div>';
  document.getElementById('sugList').innerHTML=sug;
};

/* ---- Fetch Data ---- */
var fetchData=function(){
  fetch('/data').then(function(r){return r.json()}).then(function(d){
    document.getElementById('tV').textContent=d.temp.toFixed(1);sg('tG',d.temp/50);
    var te=document.getElementById('tB');
    if(d.temp<15){te.textContent='Cold';bc(te,'bg-w')}else if(d.temp>40){te.textContent='Hot';bc(te,'bg-b')}else{te.textContent='Normal';bc(te,'bg-g')}

    document.getElementById('pV').textContent=d.ph.toFixed(2);sg('pG',d.ph/14);
    var pe=document.getElementById('pB');
    if(d.ph<6.5){pe.textContent='Acidic';bc(pe,'bg-w')}else if(d.ph>8.5){pe.textContent='Alkaline';bc(pe,'bg-w')}else if(d.ph<=7.5){pe.textContent='Neutral';bc(pe,'bg-g')}else{pe.textContent='Normal';bc(pe,'bg-g')}

    document.getElementById('dV').textContent=Math.round(d.tds);sg('dG',Math.min(d.tds/1000,1));
    var de=document.getElementById('dB');
    if(d.tds<300){de.textContent='Good';bc(de,'bg-g')}else if(d.tds<600){de.textContent='Fair';bc(de,'bg-w')}else{de.textContent='Poor';bc(de,'bg-b')}

    document.getElementById('lV').textContent=d.lvl.toFixed(0);sg('lG',d.lvl/100);
    var le=document.getElementById('lB');
    if(d.lvl>60){le.textContent='Full';bc(le,'bg-g')}else if(d.lvl>30){le.textContent='OK';bc(le,'bg-g')}else if(d.lvl>15){le.textContent='Low';bc(le,'bg-w')}else{le.textContent='Critical';bc(le,'bg-b')}

    document.getElementById('phRV').textContent=d.phV.toFixed(3)+' V';
    document.getElementById('tdsRV').textContent=d.tdsV.toFixed(3)+' V';
    document.getElementById('cpV').textContent=d.phV.toFixed(3)+' V';
    document.getElementById('ctV').textContent=d.tdsV.toFixed(3)+' V';
    var eL=document.getElementById('cLvlD');if(eL)eL.textContent=d.dist.toFixed(1)+' cm';

    // Pump
    document.getElementById('pDot').className='pump-dot '+(d.pump?'on':'off');
    document.getElementById('pSt').textContent=d.pump?'RUNNING':'STOPPED';
    document.getElementById('pSt').style.color=d.pump?'#4ade80':'#ef4444';
    var ba=document.getElementById('btnAuto'),bm=document.getElementById('btnMan'),bt=document.getElementById('btnTog');
    if(d.pAuto){ba.className='pbtn act';bm.className='pbtn';bt.style.display='none';document.getElementById('pInfo').textContent='Auto: pump ON when level < low, OFF when > high threshold.'}
    else{ba.className='pbtn';bm.className='pbtn act';bt.style.display='';bt.textContent=d.pump?'Stop Pump':'Start Pump';document.getElementById('pInfo').textContent='Manual mode: you control the pump.'}

    document.getElementById('dot').className='dot';
    document.getElementById('sts').textContent='Connected';
    document.getElementById('upd').textContent=new Date().toLocaleTimeString();
    if(d.ip)document.getElementById('ipAddr').textContent=d.ip;
    updateAnalysis(d);
  }).catch(function(){document.getElementById('dot').className='dot off';document.getElementById('sts').textContent='Offline'});
};

/* ---- Chart ---- */
var fetchHist=function(){
  fetch('/history').then(function(r){return r.json()}).then(function(d){hist=d.d||[];drawChart()});
};
var stab=function(i,el){
  curTab=i;
  var tabs=document.querySelectorAll('.chtab');
  for(var j=0;j<tabs.length;j++)tabs[j].className='chtab';
  el.className='chtab act';drawChart();
};
var drawChart=function(){
  var canvas=document.getElementById('chart'),wrap=document.getElementById('chWrap');
  if(!canvas||!wrap)return;
  canvas.width=wrap.clientWidth;canvas.height=200;
  var ctx=canvas.getContext('2d'),w=canvas.width,h=canvas.height;
  var pl=42,pr=10,pt=15,pb=20,cw=w-pl-pr,ch=h-pt-pb;
  ctx.clearRect(0,0,w,h);
  var colors=['#22d3ee','#a78bfa','#fb923c','#3b82f6'];
  var labels=['pH','TDS (ppm)','Temp (\u00b0C)','Level (%)'];
  var ranges=[[0,14],[0,1000],[0,50],[0,100]];
  if(!hist||hist.length<2){ctx.fillStyle='#475569';ctx.font='13px Inter';ctx.textAlign='center';ctx.fillText('Collecting data...',w/2,h/2);return}
  var data=[];for(var i=0;i<hist.length;i++)data.push(hist[i][curTab]);
  var mn=ranges[curTab][0],mx=ranges[curTab][1],col=colors[curTab];
  var dmin=Math.min.apply(null,data),dmax=Math.max.apply(null,data);
  if(dmin<mn)mn=Math.floor(dmin);if(dmax>mx)mx=Math.ceil(dmax);if(mx<=mn)mx=mn+1;
  // grid
  ctx.strokeStyle='rgba(255,255,255,0.04)';ctx.lineWidth=1;
  ctx.fillStyle='#475569';ctx.font='10px Inter';ctx.textAlign='right';
  for(var i=0;i<=4;i++){var y=pt+ch-ch*i/4;ctx.beginPath();ctx.moveTo(pl,y);ctx.lineTo(pl+cw,y);ctx.stroke();
  var v=mn+(mx-mn)*i/4;ctx.fillText(curTab===0?v.toFixed(1):Math.round(v),pl-4,y+3)}
  // line
  ctx.strokeStyle=col;ctx.lineWidth=2;ctx.beginPath();
  for(var i=0;i<data.length;i++){var x=pl+i/(data.length-1)*cw;var pct=(data[i]-mn)/(mx-mn);var y=pt+ch-pct*ch;if(i===0)ctx.moveTo(x,y);else ctx.lineTo(x,y)}
  ctx.stroke();
  // fill
  ctx.lineTo(pl+cw,pt+ch);ctx.lineTo(pl,pt+ch);ctx.closePath();
  var grd=ctx.createLinearGradient(0,pt,0,pt+ch);grd.addColorStop(0,col.slice(0,4)+',.15)'.replace('#','rgba(').replace(/([0-9a-f]{2})/gi,function(m){return parseInt(m,16)+','}));
  // simpler approach for fill
  ctx.globalAlpha=0.08;ctx.fillStyle=col;ctx.fill();ctx.globalAlpha=1;
  // latest dot
  if(data.length>0){var lx=pl+(data.length-1)/(data.length-1)*cw;var lp=(data[data.length-1]-mn)/(mx-mn);var ly=pt+ch-lp*ch;
  ctx.beginPath();ctx.arc(lx,ly,4,0,Math.PI*2);ctx.fillStyle=col;ctx.fill();ctx.strokeStyle='#080c1a';ctx.lineWidth=2;ctx.stroke()}
  // label
  ctx.fillStyle='#64748b';ctx.font='10px Inter';ctx.textAlign='left';ctx.fillText(labels[curTab],pl,pt-4);
  ctx.textAlign='right';ctx.fillText(data.length+' pts / 10 min',pl+cw,pt-4);
};
window.addEventListener('resize',drawChart);

/* ---- Pump Control ---- */
var setMode=function(m){fetch('/pump?mode='+m,{method:'POST'}).then(function(){fetchData()})};
var togPump=function(){
  var cur=document.getElementById('pSt').textContent==='RUNNING';
  fetch('/pump?state='+(cur?'off':'on'),{method:'POST'}).then(function(){fetchData()});
};

/* ---- Calibration ---- */
var togCal=function(){var p=document.getElementById('calP');p.classList.toggle('open');if(p.classList.contains('open'))loadCal()};
var loadCal=function(){
  fetch('/calibrate').then(function(r){return r.json()}).then(function(d){
    document.getElementById('cP7').value=d.ph7v.toFixed(3);
    document.getElementById('cP4').value=d.ph4v.toFixed(3);
    document.getElementById('cTF').value=d.tdscf.toFixed(2);
    document.getElementById('cTe').value=d.temp.toFixed(1);
    document.getElementById('cEmD').value=(d.emD!==undefined)?d.emD.toFixed(1):'100.0';
    document.getElementById('cFuD').value=(d.fuD!==undefined)?d.fuD.toFixed(1):'25.0';
    document.getElementById('cLo').value=d.lowTh.toFixed(0);
    document.getElementById('cHi').value=d.highTh.toFixed(0);
  });
};
var saveCal=function(){
  var q='?ph7v='+document.getElementById('cP7').value+'&ph4v='+document.getElementById('cP4').value
    +'&tdscf='+document.getElementById('cTF').value+'&temp='+document.getElementById('cTe').value
    +'&emD='+document.getElementById('cEmD').value+'&fuD='+document.getElementById('cFuD').value
    +'&lowTh='+document.getElementById('cLo').value
    +'&highTh='+document.getElementById('cHi').value;
  fetch('/calibrate'+q,{method:'POST'}).then(function(r){return r.json()}).then(function(){toastMsg('Settings saved to flash!')}).catch(function(){toastMsg('Save failed')});
};
var resetCal=function(){
  document.getElementById('cP7').value='1.650';document.getElementById('cP4').value='2.200';
  document.getElementById('cTF').value='0.50';document.getElementById('cTe').value='30.0';
  document.getElementById('cEmD').value='100.0';document.getElementById('cFuD').value='25.0';
  document.getElementById('cLo').value='20';
  document.getElementById('cHi').value='80';toastMsg('Reset to defaults \u2014 click Save to apply');
};

/* ---- Init ---- */
fetchData();fetchHist();
setInterval(fetchData,2000);
setInterval(fetchHist,10000);
</script>
</body></html>
)rawliteral";

#endif
