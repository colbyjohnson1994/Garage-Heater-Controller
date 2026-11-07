const char PAGE_Information[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Info - Garage Heater</title>
<link rel="stylesheet" href="/style.css">
</head>
<body>
<div class="container">
  <h1>Device Information</h1>
  <div class="card">
    <p><strong>Version:</strong> <span id="version">--</span></p>
    <p><strong>Device:</strong> <span id="devName">Garage Heater</span></p>
    <p><strong>MAC:</strong> <span id="mac">--</span></p>
    <p><strong>IP:</strong> <span id="ip">--</span></p>
    <p><strong>Free Heap:</strong> <span id="heap">--</span> bytes</p>
    <p><strong>Uptime:</strong> <span id="uptime">--</span></p>
  </div>

  <div class="card">
    <div style="display:flex; gap:8px; flex-wrap:wrap;">
      <a href="/backup.json" class="btn" download="config.json">Download Config</a>
      <button class="btn btn-secondary" onclick="document.getElementById('restoreFile').click()">Restore Config</button>
      <input type="file" id="restoreFile" accept=".json" style="display:none" onchange="restoreConfig(this)">
      <button class="btn btn-secondary" onclick="rebootDevice()">Reboot</button>
    </div>
  </div>

  <div class="card">
    <div style="display:flex; justify-content:space-between; align-items:center; margin-bottom:8px;">
      <strong>Serial Output</strong>
      <button class="btn btn-secondary" style="padding:4px 12px; font-size:0.8rem;" onclick="clearSerial()">Clear</button>
    </div>
    <pre id="serialOut" style="background:#0f172a; color:#22c55e; font-family:ui-monospace,monospace; font-size:0.8rem; padding:12px; border-radius:8px; height:280px; overflow-y:auto; margin:0; white-space:pre-wrap;"></pre>
  </div>

  <div class="nav"><a href="/">← Dashboard</a></div>
</div>
<script>
fetch('/info.json').then(r=>r.json()).then(d => {
  document.getElementById('version').textContent = d.version || '--';
  document.getElementById('mac').textContent = d.mac || '--';
  document.getElementById('ip').textContent = d.ip || '--';
  document.getElementById('heap').textContent = d.heap || '--';
  document.getElementById('uptime').textContent = d.uptime || '--';
}).catch(()=>{});

function loadSerial() {
  fetch('/serial').then(r => r.text()).then(txt => {
    const el = document.getElementById('serialOut');
    el.textContent = txt || '(no output yet)';
    el.scrollTop = el.scrollHeight;
  }).catch(()=>{});
}
function clearSerial() {
  document.getElementById('serialOut').textContent = '';
}
function restoreConfig(input) {
  const file = input.files[0];
  if (!file) return;
  const reader = new FileReader();
  reader.onload = e => {
    fetch('/restore', { method: 'POST', body: e.target.result, headers: {'Content-Type':'application/json'} })
      .then(r => r.json())
      .then(d => { alert(d.ok ? 'Restored. Rebooting...' : 'Restore failed'); if (d.ok) setTimeout(()=>location.href='/', 3000); })
      .catch(() => alert('Restore failed'));
  };
  reader.readAsText(file);
}
function rebootDevice() {
  if (!confirm('Reboot the controller?')) return;
  fetch('/reboot').then(() => { alert('Rebooting...'); setTimeout(()=>location.href='/', 5000); });
}
loadSerial();
setInterval(loadSerial, 2000);
</script>
</body>
</html>
)=====";
