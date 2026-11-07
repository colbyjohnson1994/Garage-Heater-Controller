const char PAGE_Runtime[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Runtime - Garage Heater</title>
<link rel="stylesheet" href="/style.css">
</head>
<body>
<div class="topnav">
  <a href="/">Dashboard</a>
  <a href="/schedule.html">Schedule</a>
  <a href="/runtime.html">Runtime</a>
  <a href="/config.html">Network</a>
  <a href="/mqtt.html">MQTT</a>
  <a href="/info.html">Info</a>
</div>
<div class="container">
  <h1>Runtime Usage</h1>

  <div class="card">
    <div class="row" style="justify-content:space-around; text-align:center;">
      <div>
        <div style="color:#94a3b8; font-size:0.85rem;">TODAY</div>
        <div style="font-size:1.8rem; font-weight:600;" id="todayHours">--</div>
        <div style="color:#94a3b8; font-size:0.8rem;">hours</div>
      </div>
      <div>
        <div style="color:#94a3b8; font-size:0.85rem;">LIFETIME</div>
        <div style="font-size:1.8rem; font-weight:600;" id="totalHours">--</div>
        <div style="color:#94a3b8; font-size:0.8rem;">hours</div>
      </div>
    </div>
  </div>

  <div class="card">
    <div style="color:#94a3b8; font-size:0.9rem; margin-bottom:8px;">TODAY BY HOUR</div>
    <canvas id="runtimeChart" width="640" height="160" style="width:100%; height:160px; background:#0f172a; border-radius:8px;"></canvas>
  </div>
</div>
<script>
function loadRuntime() {
  fetch('/runtime.json').then(r => r.json()).then(d => {
    document.getElementById('todayHours').textContent = (d.today / 3600).toFixed(1);
    document.getElementById('totalHours').textContent = (d.total / 3600).toFixed(1);
    drawRuntime(d.hourly || []);
  }).catch(()=>{});
}

function drawRuntime(hourly) {
  const canvas = document.getElementById('runtimeChart');
  if (!canvas || hourly.length === 0) return;
  const ctx = canvas.getContext('2d');
  const w = canvas.width, h = canvas.height;
  ctx.clearRect(0, 0, w, h);
  const max = Math.max(...hourly, 1);
  const barW = (w - 40) / 24;
  ctx.fillStyle = '#3b82f6';
  hourly.forEach((v, i) => {
    const bh = (v / max) * (h - 30);
    ctx.fillRect(20 + i * barW + 2, h - 20 - bh, barW - 4, bh);
  });
  ctx.fillStyle = '#94a3b8';
  ctx.font = '10px system-ui';
  for (let i = 0; i < 24; i += 4) {
    ctx.fillText(i + '', 20 + i * barW, h - 4);
  }
}

loadRuntime();
setInterval(loadRuntime, 30000);
</script>
</body>
</html>
)=====";
