const char PAGE_Script_js[] PROGMEM = R"=====(
let chartData = { indoor: [], outdoor: [], min: 0, max: 0, pad: 30 };

function weatherIcon(code) {
  if (code === 0) return '☀️';
  if (code <= 3) return '⛅';
  if (code <= 48) return '🌫️';
  if (code <= 55) return '🌦️';
  if (code <= 65) return '🌧️';
  if (code <= 75) return '❄️';
  if (code <= 82) return '🌧️';
  if (code >= 95) return '⛈️';
  return '❓';
}

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
      const locEl = document.getElementById('locationInfo');
      if (locEl) {
        let parts = [];
        if (data.location) parts.push(data.location);
        if (data.zip) parts.push(data.zip);
        if (data.lat && data.lon) parts.push(data.lat.toFixed(2) + ', ' + data.lon.toFixed(2));
        locEl.textContent = parts.join(' · ') || '--';
      }
    })
    .catch(e => console.error(e));
}

function loadForecast() {
  fetch('/forecast.json').then(r => r.json()).then(d => {
    const el = document.getElementById('forecast');
    if (!el || !d.temps) return;
    let html = '<div style="display:flex; gap:12px; overflow-x:auto; padding:8px 0;">';
    d.temps.forEach((t, i) => {
      if (t === null) return;
      const h = d.hours[i];
      const icon = weatherIcon(d.codes ? d.codes[i] : 0);
      html += '<div style="text-align:center; min-width:50px;">';
      html += '<div style="font-size:0.75rem; color:#64748b;">' + h + ':00</div>';
      html += '<div style="font-size:1.2rem;">' + icon + '</div>';
      html += '<div style="font-weight:500;">' + t.toFixed(0) + '°</div>';
      html += '</div>';
    });
    html += '</div>';
    el.innerHTML = html;
  }).catch(()=>{});
}

function drawChart(indoor, outdoor, hoverIndex) {
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

  chartData = { indoor, outdoor, min, max, pad };

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

  // Hover highlight
  if (hoverIndex >= 0 && hoverIndex < indoor.length) {
    const hx = x(hoverIndex);
    ctx.strokeStyle = '#64748b';
    ctx.lineWidth = 1;
    ctx.setLineDash([4, 4]);
    ctx.beginPath();
    ctx.moveTo(hx, pad);
    ctx.lineTo(hx, h - pad);
    ctx.stroke();
    ctx.setLineDash([]);

    ctx.fillStyle = '#3b82f6';
    ctx.beginPath();
    ctx.arc(hx, y(indoor[hoverIndex]), 4, 0, Math.PI * 2);
    ctx.fill();
    if (outdoor[hoverIndex] !== null) {
      ctx.fillStyle = '#f59e0b';
      ctx.beginPath();
      ctx.arc(hx, y(outdoor[hoverIndex]), 4, 0, Math.PI * 2);
      ctx.fill();
    }

    const tip = document.getElementById('chartTip');
    if (tip) {
      let txt = 'In: ' + indoor[hoverIndex].toFixed(1) + '°';
      if (outdoor[hoverIndex] !== null) txt += '  Out: ' + outdoor[hoverIndex].toFixed(1) + '°';
      tip.textContent = txt;
      tip.style.display = 'block';
      const rect = canvas.getBoundingClientRect();
      tip.style.left = (rect.left + hx * (rect.width / w) + 10) + 'px';
      tip.style.top = (rect.top + 10) + 'px';
    }
  }

  ctx.fillStyle = '#94a3b8';
  ctx.font = '12px system-ui';
  ctx.fillText(max.toFixed(0) + '°', 4, pad + 4);
  ctx.fillText(min.toFixed(0) + '°', 4, h - pad);
}

function setupChartHover() {
  const canvas = document.getElementById('tempChart');
  if (!canvas || canvas._hoverSetup) return;
  canvas._hoverSetup = true;

  let tip = document.getElementById('chartTip');
  if (!tip) {
    tip = document.createElement('div');
    tip.id = 'chartTip';
    tip.style.cssText = 'position:fixed;background:#1e293b;color:#fff;padding:6px 10px;border-radius:6px;font-size:0.8rem;pointer-events:none;display:none;z-index:100;box-shadow:0 2px 8px rgba(0,0,0,0.4);';
    document.body.appendChild(tip);
  }

  canvas.addEventListener('mousemove', e => {
    const rect = canvas.getBoundingClientRect();
    const scaleX = canvas.width / rect.width;
    const mx = (e.clientX - rect.left) * scaleX;
    const pad = chartData.pad;
    const n = chartData.indoor.length;
    if (n < 2) return;
    const idx = Math.round(((mx - pad) / (canvas.width - pad * 2)) * (n - 1));
    if (idx >= 0 && idx < n) {
      drawChart(chartData.indoor, chartData.outdoor, idx);
    }
  });

  canvas.addEventListener('mouseleave', () => {
    const tip = document.getElementById('chartTip');
    if (tip) tip.style.display = 'none';
    drawChart(chartData.indoor, chartData.outdoor, -1);
  });
}

function loadHistory() {
  fetch('/history.json')
    .then(r => r.json())
    .then(d => {
      if (d.indoor && d.indoor.length > 1) {
        drawChart(d.indoor, d.outdoor || [], -1);
        setupChartHover();
      }
    })
    .catch(() => {});
}

function refreshWeather() {
  fetch('/weather/refresh').then(() => {
    setTimeout(() => { loadStatus(); loadForecast(); }, 1500);
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
  loadForecast();
  setInterval(loadStatus, 3000);
  setInterval(loadHistory, 60000);
  setInterval(loadForecast, 300000);
}
)=====";
