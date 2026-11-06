const char PAGE_Index[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Garage Heater</title>
<link rel="stylesheet" href="/style.css">
</head>
<body>
<div class="container">
  <h1>🔥 Garage Heater</h1>
  <div style="text-align:center; font-size:1.4rem; color:#94a3b8; margin-bottom:16px;" id="localTime">--:--</div>

  <div class="card">
    <div style="text-align:center; color:#94a3b8; font-size:0.9rem;">INDOOR TEMPERATURE</div>
    <div class="temp-big"><span id="temp">--</span><span class="temp-unit">°F</span></div>
    <div style="text-align:center;">
      <span id="relayStatus" class="status-badge status-off">IDLE</span>
    </div>
  </div>

  <div class="card">
    <div style="display:flex; justify-content:space-between; align-items:center;">
      <div>
        <div style="color:#94a3b8; font-size:0.9rem;">OUTDOOR WEATHER</div>
        <div style="font-size:2rem; font-weight:600;" id="outdoorTemp">--°F</div>
        <div style="display:flex; align-items:center; gap:8px; margin-top:4px;">
          <span id="weatherIcon" style="font-size:1.8rem;">❓</span>
          <span id="outdoorCond" style="color:#94a3b8; font-size:1.05rem;">--</span>
        </div>
      </div>
      <div style="text-align:right;">
        <div style="font-size:1.2rem;" id="outdoorHum">--%</div>
        <div style="color:#94a3b8; font-size:0.85rem;">Humidity</div>
      </div>
    </div>
  </div>

  <div class="card">
    <div class="row" style="justify-content:space-between;">
      <div>
        <div style="color:#94a3b8; font-size:0.9rem;">HEATER ENABLED</div>
        <label class="switch">
          <input type="checkbox" id="enabled" onchange="toggleHeater()">
          <span class="slider"></span>
        </label>
      </div>
      <div>
        <label>Setpoint (°F)</label>
        <input type="number" id="sp" min="35" max="80" value="50">
      </div>
      <div>
        <label>Hysteresis (°F)</label>
        <input type="number" id="hist" min="1" max="10" value="4">
      </div>
      <button class="btn" onclick="saveSetpoint()">Save</button>
    </div>
  </div>

  <div class="nav">
    <a href="/schedule.html">Schedule</a>
    <a href="/config.html">Network & Location</a>
    <a href="/changepass">Password</a>
    <a href="/info.html">Info</a>
    <a href="/logout">Logout</a>
  </div>
</div>
<script src="/script.js"></script>
<script>initDashboard();</script>
</body>
</html>
)=====";
