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
<div class="topnav">
  <a href="/">Dashboard</a>
  <a href="/schedule.html">Schedule</a>
  <a href="/runtime.html">Runtime</a>
  <a href="/config.html">Network</a>
  <a href="/mqtt.html">MQTT</a>
  <a href="/info.html">Info</a>
</div>
<div class="container">
  <h1>🔥 Garage Heater</h1>
  <div style="text-align:center; font-size:1.4rem; color:#94a3b8; margin-bottom:16px;" id="localTime">--:--</div>

  <div class="card">
    <div style="text-align:center; color:#94a3b8; font-size:0.9rem;">INDOOR TEMPERATURE</div>
    <div class="temp-big"><span id="temp">--</span><span class="temp-unit">°F</span></div>
    <div class="rate" id="rate">-- °/min</div>
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
        <div style="color:#64748b; font-size:0.8rem; margin-top:6px;" id="locationInfo">--</div>
      </div>
      <div style="text-align:right;">
        <div style="font-size:1.2rem;" id="outdoorHum">--%</div>
        <div style="color:#94a3b8; font-size:0.85rem;">Humidity</div>
        <button class="btn btn-secondary" style="margin-top:10px; padding:6px 12px; font-size:0.85rem;" onclick="refreshWeather()">Refresh</button>
      </div>
    </div>
    <div id="forecast" style="margin-top:12px; font-size:0.8rem; color:#94a3b8;"></div>
  </div>

  <div class="card">
    <div style="display:flex; justify-content:space-between; align-items:center; margin-bottom:8px;">
      <div style="color:#94a3b8; font-size:0.9rem;">TEMPERATURE HISTORY (24h)</div>
      <div style="font-size:0.8rem;">
        <span style="color:#3b82f6;">● Indoor</span>
        <span style="color:#f59e0b; margin-left:12px;">● Outdoor</span>
      </div>
    </div>
    <canvas id="tempChart" width="640" height="200" style="width:100%; height:200px; background:#0f172a; border-radius:8px;"></canvas>
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
</div>
<script src="/script.js"></script>
<script>initDashboard();</script>
</body>
</html>
)=====";
