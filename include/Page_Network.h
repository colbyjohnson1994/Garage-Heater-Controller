const char PAGE_Network[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Settings - Garage Heater</title>
<link rel="stylesheet" href="/style.css">
</head>
<body>
<div class="container">
  <h1>Network & Location</h1>
  <div class="card">
    <form action="/config.html" method="get" id="wifiForm">
      <label>SSID</label>
      <input type="text" name="ssid" id="ssid" value="" autocomplete="off">
      <label>Password</label>
      <input type="password" name="password" id="password" value="" autocomplete="off">
      <label><input type="checkbox" name="dhcp" id="dhcp" checked> Use DHCP</label>
      <div class="row">
        <div><label>IP</label><input type="text" name="ip" id="ip" placeholder="192.168.1.100"></div>
        <div><label>Gateway</label><input type="text" name="gw" id="gw" placeholder="192.168.1.1"></div>
      </div>
      <hr style="border-color:#334155; margin:20px 0;">
      <label>Zip Code</label>
      <div style="display:flex; gap:8px;">
        <input type="text" name="zip" id="zip" placeholder="57701" maxlength="10" style="flex:1;">
        <button type="button" class="btn btn-secondary" onclick="lookupZip()">Lookup</button>
      </div>
      <p id="geoResult" style="color:#94a3b8; font-size:0.85rem; margin-top:8px;"></p>
      <input type="hidden" name="lat" id="lat">
      <input type="hidden" name="lon" id="lon">
      <button type="submit" class="btn">Save Settings</button>
    </form>
  </div>

  <div class="card">
    <div style="display:flex; justify-content:space-between; align-items:center; margin-bottom:12px;">
      <strong>Available Networks</strong>
      <button type="button" class="btn btn-secondary" onclick="scanNetworks()">Scan</button>
    </div>
    <div id="networks" style="font-size:0.95rem;">Click Scan to search for nearby WiFi networks.</div>
  </div>

  <div class="card">
    <strong>Connection:</strong> <span id="connState">Checking...</span>
  </div>
  <div class="nav"><a href="/">← Dashboard</a></div>
</div>
<script src="/script.js"></script>
<script>
function scanNetworks() {
  const el = document.getElementById('networks');
  el.innerHTML = '<span style="color:#94a3b8;">Scanning…</span>';
  fetch('/scan')
    .then(r => r.json())
    .then(data => {
      if (!data.networks || data.networks.length === 0) {
        el.innerHTML = '<span style="color:#ef4444;">No networks found.</span>';
        return;
      }
      let html = '<div style="display:flex; flex-direction:column; gap:8px;">';
      data.networks.forEach(n => {
        const lock = n.enc ? '🔒' : '🔓';
        const bars = n.quality >= 70 ? '▂▄▆█' : n.quality >= 40 ? '▂▄▆' : '▂▄';
        html += `<div style="display:flex; justify-content:space-between; align-items:center; padding:10px 12px; background:#0f172a; border-radius:8px; cursor:pointer;" onclick="selectSSID('${n.ssid.replace(/'/g, "\\'")}')">
          <span><strong>${n.ssid}</strong> ${lock}</span>
          <span style="color:#94a3b8; font-size:0.85rem;">${bars} ${n.quality}%</span>
        </div>`;
      });
      html += '</div>';
      el.innerHTML = html;
    })
    .catch(e => {
      el.innerHTML = '<span style="color:#ef4444;">Scan failed.</span>';
      console.error(e);
    });
}

function selectSSID(ssid) {
  document.getElementById('ssid').value = ssid;
  document.getElementById('password').focus();
}

function lookupZip() {
  const zip = document.getElementById('zip').value.trim();
  const result = document.getElementById('geoResult');
  if (!zip) { result.textContent = 'Enter a zip code'; result.style.color = '#ef4444'; return; }
  result.textContent = 'Looking up…';
  result.style.color = '#94a3b8';
  fetch('/geocode?zip=' + encodeURIComponent(zip))
    .then(r => r.json())
    .then(d => {
      if (d.ok) {
        document.getElementById('lat').value = d.lat;
        document.getElementById('lon').value = d.lon;
        result.style.color = '#22c55e';
        result.textContent = d.place + ' (' + d.lat.toFixed(4) + ', ' + d.lon.toFixed(4) + ')';
      } else {
        result.style.color = '#ef4444';
        result.textContent = d.error || 'Lookup failed';
      }
    })
    .catch(() => { result.style.color = '#ef4444'; result.textContent = 'Lookup failed'; });
}

function loadCurrentSettings() {
  fetch('/admin/values')
    .then(r => r.text())
    .then(txt => {
      const lines = txt.split('\n');
      lines.forEach(line => {
        const parts = line.split('|');
        if (parts.length < 2) return;
        const id = parts[0];
        const val = parts[1];
        const el = document.getElementById(id);
        if (!el) return;
        if (el.type === 'checkbox') el.checked = (val === 'checked');
        else el.value = val;
      });
      // Show current coords if present
      const lat = document.getElementById('lat').value;
      const lon = document.getElementById('lon').value;
      if (lat && lon) {
        document.getElementById('geoResult').textContent = 'Current: ' + parseFloat(lat).toFixed(4) + ', ' + parseFloat(lon).toFixed(4);
      }
    });
  fetch('/admin/connectionstate')
    .then(r => r.text())
    .then(txt => { document.getElementById('connState').textContent = txt; });
}

window.onload = loadCurrentSettings;
</script>
</body>
</html>
)=====";

void send_network_configuration_html()
{
  if (server.args() > 0)
  {
    for (uint8_t i = 0; i < server.args(); i++)
    {
      if (server.argName(i) == "ssid") config.ssid = urldecode(server.arg(i));
      if (server.argName(i) == "password") config.password = urldecode(server.arg(i));
      if (server.argName(i) == "dhcp") config.dhcp = true;
      if (server.argName(i) == "zip") config.zipCode = server.arg(i);
      if (server.argName(i) == "lat") config.latitude = server.arg(i).toFloat();
      if (server.argName(i) == "lon") config.longitude = server.arg(i).toFloat();
    }
    if (!server.hasArg("dhcp")) config.dhcp = false;
    WriteConfig();
    ConfigureWifi();
    server.sendHeader("Location", "/config.html", true);
    server.send(302, "text/plain", "");
    return;
  }
  server.send(200, "text/html", PAGE_Network);
}

void send_network_configuration_values_html()
{
  String values = "";
  values += "ssid|" + config.ssid + "|input\n";
  values += "password|" + config.password + "|input\n";
  values += "dhcp|" + String(config.dhcp ? "checked" : "") + "|chk\n";
  values += "zip|" + config.zipCode + "|input\n";
  values += "lat|" + String(config.latitude, 4) + "|input\n";
  values += "lon|" + String(config.longitude, 4) + "|input\n";
  server.send(200, "text/plain", values);
}

void send_connection_state_values_html()
{
  String state = WiFi.status() == WL_CONNECTED
    ? "Connected to " + WiFi.SSID() + " (" + WiFi.localIP().toString() + ")"
    : "Disconnected / AP Mode";
  server.send(200, "text/plain", state);
}

void send_scan_json()
{
  int n = WiFi.scanNetworks();
  String json = "{\"networks\":[";
  if (n > 0)
  {
    for (int i = 0; i < n; i++)
    {
      if (i > 0) json += ",";
      int quality = 0;
      int rssi = WiFi.RSSI(i);
      if (rssi <= -100) quality = 0;
      else if (rssi >= -50) quality = 100;
      else quality = 2 * (rssi + 100);

      String ssid = WiFi.SSID(i);
      ssid.replace("\\", "\\\\");
      ssid.replace("\"", "\\\"");

      json += "{";
      json += "\"ssid\":\"" + ssid + "\",";
      json += "\"rssi\":" + String(rssi) + ",";
      json += "\"quality\":" + String(quality) + ",";
      json += "\"enc\":" + String(WiFi.encryptionType(i) != ENC_TYPE_NONE ? "true" : "false");
      json += "}";
    }
  }
  json += "]}";
  WiFi.scanDelete();
  server.send(200, "application/json", json);
}

void send_geocode_json()
{
  String zip = server.arg("zip");
  zip.trim();
  if (zip.length() < 5)
  {
    server.send(200, "application/json", "{\"ok\":false,\"error\":\"Invalid zip\"}");
    return;
  }

  WiFiClient client;
  HTTPClient http;
  String url = "http://api.zippopotam.us/us/" + zip;
  http.begin(client, url);
  int code = http.GET();

  if (code == 200)
  {
    String payload = http.getString();
    DynamicJsonDocument doc(1024);
    if (!deserializeJson(doc, payload))
    {
      JsonArray places = doc["places"];
      if (places.size() > 0)
      {
        float lat = places[0]["latitude"].as<float>();
        float lon = places[0]["longitude"].as<float>();
        String place = places[0]["place name"].as<String>() + ", " + places[0]["state abbreviation"].as<String>();
        String json = "{\"ok\":true,\"lat\":" + String(lat, 4) + ",\"lon\":" + String(lon, 4) + ",\"place\":\"" + place + "\"}";
        http.end();
        server.send(200, "application/json", json);
        return;
      }
    }
  }
  http.end();
  server.send(200, "application/json", "{\"ok\":false,\"error\":\"Zip not found\"}");
}

void send_information_values_html()
{
  server.send(200, "text/plain", "ok");
}
