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
</script>
</body>
</html>
)=====";
