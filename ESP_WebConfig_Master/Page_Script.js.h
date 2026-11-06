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
  setInterval(loadStatus, 3000);
}

function initSchedule() {
  // Load current values if needed
}
)=====";
