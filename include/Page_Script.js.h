const char PAGE_Script_js[] PROGMEM = R"=====(
function loadStatus() {
  fetch('/status')
    .then(r => r.json())
    .then(data => {
      document.getElementById('temp').textContent = data.temp.toFixed(1);
      document.getElementById('sp').value = data.sp;
      document.getElementById('hist').value = data.hist;
      const toggle = document.getElementById('enabled');
      if (toggle) toggle.checked = data.enabled;
      const badge = document.getElementById('relayStatus');
      if (badge) {
        badge.textContent = data.relay ? 'HEATING' : 'IDLE';
        badge.className = 'status-badge ' + (data.relay ? 'status-on' : 'status-off');
      }
      const outdoorEl = document.getElementById('outdoorTemp');
      if (outdoorEl && data.outdoor !== null) {
        outdoorEl.textContent = data.outdoor.toFixed(1) + '°F';
      }
      const condEl = document.getElementById('outdoorCond');
      if (condEl) condEl.textContent = data.condition || '--';
      const iconEl = document.getElementById('weatherIcon');
      if (iconEl) iconEl.textContent = data.icon || '❓';
      const humEl = document.getElementById('outdoorHum');
      if (humEl) humEl.textContent = (data.humidity || '--') + '%';
      const timeEl = document.getElementById('localTime');
      if (timeEl && data.time) timeEl.textContent = data.time;
    })
    .catch(e => console.error(e));
}

function drawChart(indoor, outdoor) {
  const canvas = document.getElementById('tempChart');
  if (!canvas || indoor.length === 0) return;
  const ctx = canvas.getContext('2d');
  const w = canvas.width;
  const h = canvas.height;
  ctx.clearRect(0, 0, w, h);

  const all = indoor.concat(outdoor.filter(v => v !== null));
  let min = Math.min(...all);
  let max = Math.max(...all);
  if (max - min < 5) { min -= 2; max += 2; }
  const pad = 30;

  function y(v) {
    return h - pad - ((v - min) / (max - min)) * (h - pad * 2);
  }
  function x(i) {
    return pad + (i / (indoor.length - 1)) * (w - pad * 2);
  }

  // Grid lines
  ctx.strokeStyle = '#334155';
  ctx.lineWidth = 1;
  for (let i = 0; i <= 4; i++) {
    const yy = pad + (i / 4) * (h - pad * 2);
    ctx.beginPath();
    ctx.moveTo(pad, yy);
    ctx.lineTo(w - pad, yy);
    ctx.stroke();
  }

  // Indoor line
  ctx.strokeStyle = '#3b82f6';
  ctx.lineWidth = 2;
  ctx.beginPath();
  indoor.forEach((v, i) => {
    if (i === 0) ctx.moveTo(x(i), y(v));
    else ctx.lineTo(x(i), y(v));
  });
  ctx.stroke();

  // Outdoor line
  ctx.strokeStyle = '#f59e0b';
  ctx.beginPath();
  let started = false;
  outdoor.forEach((v, i) => {
    if (v === null) return;
    if (!started) { ctx.moveTo(x(i), y(v)); started = true; }
    else ctx.lineTo(x(i), y(v));
  });
  ctx.stroke();

  // Labels
  ctx.fillStyle = '#94a3b8';
  ctx.font = '12px system-ui';
  ctx.fillText(max.toFixed(0) + '°', 4, pad + 4);
  ctx.fillText(min.toFixed(0) + '°', 4, h - pad);
}

function loadHistory() {
  fetch('/history.json')
    .then(r => r.json())
    .then(d => {
      if (d.indoor && d.indoor.length > 1) drawChart(d.indoor, d.outdoor || []);
    })
    .catch(() => {});
}

function refreshWeather() {
  fetch('/weather/refresh').then(() => {
    setTimeout(loadStatus, 1500);
  });
}

function toggleHeater() {
  fetch('/toggle').then(() => loadStatus());
}

function saveSetpoint() {
  const sp = document.getElementById('sp').value;
  const hist = document.getElementById('hist').value;
  fetch('/setpoint?sp=' + sp + '&hist=' + hist).then(() => loadStatus());
}

function initDashboard() {
  loadStatus();
  loadHistory();
  setInterval(loadStatus, 3000);
  setInterval(loadHistory, 60000);
}
)=====";
