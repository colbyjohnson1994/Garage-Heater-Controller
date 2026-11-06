const char PAGE_Schedule[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Schedule - Garage Heater</title>
<link rel="stylesheet" href="/style.css">
<style>
.day-row { margin-bottom: 18px; }
.day-label { font-weight: 600; margin-bottom: 6px; display: flex; justify-content: space-between; align-items: center; }
.hour-grid { display: flex; flex-wrap: wrap; gap: 3px; }
.hour-btn {
  width: 32px; height: 28px;
  border: none; border-radius: 4px;
  background: #334155; color: #94a3b8;
  font-size: 0.7rem; cursor: pointer;
  padding: 0;
}
.hour-btn.on { background: #3b82f6; color: white; }
.preset-btn { font-size: 0.75rem; padding: 4px 8px; margin-left: 4px; }
</style>
</head>
<body>
<div class="container">
  <h1>Weekly Schedule</h1>
  <div class="card">
    <label>
      <input type="checkbox" id="scheduleEnabled"> Enable Weekly Schedule
    </label>
    <p style="color:#94a3b8; font-size:0.85rem; margin-top:4px;">When enabled, the heater will only run during the selected hours for each day.</p>
  </div>

  <div class="card" id="scheduleGrid">
    <!-- filled by JS -->
  </div>

  <div class="card" style="text-align:center;">
    <button class="btn" onclick="saveSchedule()">Save Schedule</button>
  </div>
  <div class="nav"><a href="/">← Dashboard</a></div>
</div>
<script src="/script.js"></script>
<script>
const days = ['Sunday','Monday','Tuesday','Wednesday','Thursday','Friday','Saturday'];
let schedule = Array(7).fill(0xFFFFFF); // all hours on

function renderSchedule() {
  const grid = document.getElementById('scheduleGrid');
  grid.innerHTML = '';
  days.forEach((day, d) => {
    const row = document.createElement('div');
    row.className = 'day-row';
    row.innerHTML = `
      <div class="day-label">
        <span>${day}</span>
        <span>
          <button type="button" class="btn btn-secondary preset-btn" onclick="setDay(${d}, true)">All</button>
          <button type="button" class="btn btn-secondary preset-btn" onclick="setDay(${d}, false)">None</button>
          <button type="button" class="btn btn-secondary preset-btn" onclick="setRange(${d}, 6, 22)">6–22</button>
        </span>
      </div>
      <div class="hour-grid" id="day-${d}"></div>
    `;
    grid.appendChild(row);

    const hourGrid = document.getElementById('day-' + d);
    for (let h = 0; h < 24; h++) {
      const btn = document.createElement('button');
      btn.type = 'button';
      btn.className = 'hour-btn' + ((schedule[d] & (1 << h)) ? ' on' : '');
      btn.textContent = h;
      btn.onclick = () => toggleHour(d, h, btn);
      hourGrid.appendChild(btn);
    }
  });
}

function toggleHour(day, hour, btn) {
  schedule[day] ^= (1 << hour);
  btn.classList.toggle('on');
}

function setDay(day, on) {
  schedule[day] = on ? 0xFFFFFF : 0;
  renderSchedule();
}

function setRange(day, start, end) {
  schedule[day] = 0;
  for (let h = start; h < end; h++) schedule[day] |= (1 << h);
  renderSchedule();
}

function loadSchedule() {
  fetch('/schedule.json')
    .then(r => r.json())
    .then(data => {
      document.getElementById('scheduleEnabled').checked = data.enabled;
      if (data.hours && data.hours.length === 7) schedule = data.hours;
      renderSchedule();
    })
    .catch(() => renderSchedule());
}

function saveSchedule() {
  const payload = {
    enabled: document.getElementById('scheduleEnabled').checked,
    hours: schedule
  };
  fetch('/schedule.json', {
    method: 'POST',
    headers: {'Content-Type': 'application/json'},
    body: JSON.stringify(payload)
  }).then(() => alert('Schedule saved'));
}

window.onload = loadSchedule;
</script>
</body>
</html>
)=====";

void send_schedule_html()
{
  server.send(200, "text/html", PAGE_Schedule);
}

void send_schedule_json()
{
  if (server.method() == HTTP_POST)
  {
    String body = server.arg("plain");
    DynamicJsonDocument doc(512);
    if (!deserializeJson(doc, body))
    {
      config.scheduleEnabled = doc["enabled"] | false;
      JsonArray hours = doc["hours"];
      if (hours.size() == 7)
      {
        for (int d = 0; d < 7; d++)
          config.scheduleHours[d] = hours[d] | 0;
      }
      WriteConfig();
    }
    server.send(200, "application/json", "{\"ok\":true}");
    return;
  }

  // GET
  String json = "{\"enabled\":" + String(config.scheduleEnabled ? "true" : "false") + ",\"hours\":[";
  for (int d = 0; d < 7; d++)
  {
    if (d > 0) json += ",";
    json += String(config.scheduleHours[d]);
  }
  json += "]}";
  server.send(200, "application/json", json);
}

void send_general_configuration_values_html()
{
  server.send(200, "text/plain", "ok");
}
