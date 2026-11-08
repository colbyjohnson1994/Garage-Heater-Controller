const char PAGE_MQTT[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>MQTT - Garage Heater</title>
<link rel="icon" href="/favicon.svg" type="image/svg+xml">
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
  <h1>MQTT Settings</h1>
  <div class="card">
    <form action="/mqtt.html" method="get">
      <label><input type="checkbox" name="enabled" id="enabled"> Enable MQTT</label>
      <label>Broker</label>
      <input type="text" name="broker" id="broker" placeholder="192.168.1.10">
      <label>Port</label>
      <input type="number" name="port" id="port" value="1883" min="1" max="65535">
      <label>Username</label>
      <input type="text" name="user" id="user" autocomplete="off">
      <label>Password</label>
      <input type="password" name="pass" id="pass" autocomplete="off">
      <label>Topic</label>
      <input type="text" name="topic" id="topic" value="garage/heater">
      <p style="color:#64748b; font-size:0.8rem;">Commands: publish to {topic}/set with "on", "off", or "sp:55"</p>
      <button type="submit" class="btn">Save</button>
    </form>
  </div>
  <div class="card">
    <strong>Status:</strong> <span id="mqttStatus">--</span>
  </div>
</div>
<script>
function loadMQTT() {
  fetch('/mqtt.json').then(r => r.json()).then(d => {
    document.getElementById('enabled').checked = d.enabled;
    document.getElementById('broker').value = d.broker || '';
    document.getElementById('port').value = d.port || 1883;
    document.getElementById('user').value = d.user || '';
    document.getElementById('topic').value = d.topic || 'garage/heater';
    document.getElementById('mqttStatus').textContent = d.connected ? 'Connected' : 'Disconnected';
    document.getElementById('mqttStatus').style.color = d.connected ? '#22c55e' : '#ef4444';
  }).catch(()=>{});
}
window.onload = loadMQTT;
</script>
</body>
</html>
)=====";
