/*
  ESP_WebConfig - Modernized Garage Heater Controller
  Original based on John Lassen's ESP_WebConfig
  Updated with modern UI, outdoor weather (Open-Meteo), and scheduling
*/

#include "secrets.h"
#include <math.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <EEPROM.h>
#include <time.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>
#include "helpers.h"
#include "global.h"

#include "Page_Index.h"
#include "Page_General.h"
#include "Page_Network.h"
#include "Page_Information.h"
#include "Page_Login.h"
#include "Page_Runtime.h"
#include "Page_MQTT.h"
#include "Page_Style.css.h"
#include "Page_Script.js.h"
#include "Page_Favicon.h"
#include "Page_PWA.h"

// FIRMWARE_VERSION is set via build_flags in platformio.ini

unsigned int Rs = 150000;
double Vcc = 3.3;

// Forward declarations (PlatformIO does not auto-prototype)
void send_status_json();
void toggleHeaterState();
void set_setpoint();
void send_info_json();
void send_weather_json();
void fetchWeather();
double Thermister(int val);
void send_history_json();
void send_runtime_json();
void send_backup_json();
void handle_restore();
void handle_reboot();
void mqttCallback(char* topic, byte* payload, unsigned int length);
void mqttPublish();
void send_mqtt_html();
void send_mqtt_json();

WiFiClient mqttWifiClient;
PubSubClient mqtt(mqttWifiClient);

// Serial log buffer for web terminal
#define SERIAL_LOG_SIZE 4096
char serialLog[SERIAL_LOG_SIZE];
size_t serialLogPos = 0;

// Temperature history (5-minute samples, 24 hours = 288 points)
#define HISTORY_SIZE 288
#define HISTORY_INTERVAL 300000  // 5 minutes in ms
float historyIndoor[HISTORY_SIZE];
float historyOutdoor[HISTORY_SIZE];
uint32_t historyTime[HISTORY_SIZE];  // epoch seconds for each sample
uint16_t historyCount = 0;
uint16_t historyHead = 0;
uint32_t time_history = 0;

// Runtime hourly (seconds on per hour today)
uint16_t runtimeHourly[24] = {0};
uint32_t time_mqtt = 0;

// Temperature rate of change (samples every 10 seconds, 6 samples = 1 minute)
#define RATE_SAMPLES 6
float rateSamples[RATE_SAMPLES];
uint8_t rateIndex = 0;
uint8_t rateCount = 0;
uint32_t time_rate = 0;
float tempRate = 0; // degrees per minute

void serialLogPrint(const String& s)
{
  for (size_t i = 0; i < s.length(); i++)
  {
    serialLog[serialLogPos] = s[i];
    serialLogPos = (serialLogPos + 1) % SERIAL_LOG_SIZE;
  }
  if (serialLogPos == 0 || s.endsWith("\n"))
    serialLog[serialLogPos] = '\0';
}

void serialLogPrintln(const String& s)
{
  serialLogPrint(s);
  serialLogPrint("\n");
}

void setup()
{
  EEPROM.begin(512);
  Serial.begin(115200);
  delay(500);
  Serial.println("Starting ESP8266 Garage Heater");
  serialLogPrintln("Starting ESP8266 Garage Heater");

  if (!ReadConfig())
  {
    // DEFAULT CONFIG
    config.ssid = "";
    config.password = "";
    config.dhcp = true;
    config.IP[0] = 192; config.IP[1] = 168; config.IP[2] = 1; config.IP[3] = 100;
    config.Netmask[0] = 255; config.Netmask[1] = 255; config.Netmask[2] = 255; config.Netmask[3] = 0;
    config.Gateway[0] = 192; config.Gateway[1] = 168; config.Gateway[2] = 1; config.Gateway[3] = 1;
    config.ntpServerName = "pool.ntp.org";
    config.timezone = -7; // MDT example
    config.daylight = true;
    config.DeviceName = "Garage Heater";
    config.AutoTurnOff = false;
    config.AutoTurnOn = false;
    config.TurnOffHour = 22;
    config.TurnOffMinute = 0;
    config.TurnOnHour = 6;
    config.TurnOnMinute = 0;
    config.HeatSP = 50;
    config.HeatHist = 4;
    config.HeatStatus = false;
    config.AdminEnabled = false;
    config.zipCode = "57701";
    config.locationName = "Rapid City, SD";
    config.latitude = 44.0805; // Rapid City default
    config.longitude = -103.2310;
    config.scheduleEnabled = false;
    for (int d = 0; d < 7; d++) config.scheduleHours[d] = 0x00FFFFFF; // all hours on by default
    config.loginUser = "admin";
    config.loginHash = simpleHash("heater");
    config.mqttBroker = "";
    config.mqttPort = 1883;
    config.mqttUser = "";
    config.mqttPass = "";
    config.mqttTopic = "garage/heater";
    config.mqttEnabled = false;
    config.runtimeTotal = 0;
    WriteConfig();
    Serial.println("Default config applied");
    delay(1000);
  }

  // Validate runtime (garbage EEPROM reads as huge values)
  if (config.runtimeTotal > 315360000) // > 10 years of seconds
  {
    config.runtimeTotal = 0;
    Serial.println("Runtime total reset (was uninitialized)");
  }
  runtimeTotal = config.runtimeTotal;

  // Force hardcoded WiFi so OTA works even if EEPROM SSID is empty
  if (String(WIFI_SSID) != "YOUR_SSID_HERE")
  {
    config.ssid = WIFI_SSID;
    config.password = WIFI_PASSWORD;
    Serial.println("Using hardcoded WiFi: " + config.ssid);
  }

  // WiFi: AP-only if no SSID saved (more reliable), otherwise AP+STA
  if (config.ssid == "")
  {
    WiFi.mode(WIFI_AP);
    Serial.println("No SSID saved — AP mode only");
  }
  else
  {
    WiFi.mode(WIFI_AP_STA);
  }
  WiFi.softAPConfig(IPAddress(192,168,4,1), IPAddress(192,168,4,1), IPAddress(255,255,255,0));
  WiFi.softAP(AP_SSID, AP_PASSWORD);
  delay(100);
  Serial.println("AP IP: " + WiFi.softAPIP().toString());
  Serial.println("SSID stored: '" + config.ssid + "'");
  ConfigureWifi();

  // ArduinoOTA (local network only)
  ArduinoOTA.setHostname(OTA_HOSTNAME);
  ArduinoOTA.setPassword(OTA_PASSWORD);
  ArduinoOTA.onStart([]() {
    Serial.println("OTA Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nOTA End");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("OTA Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("OTA Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("ArduinoOTA ready");

  // NTP
  configTime(config.timezone * 3600, config.daylight ? 3600 : 0, config.ntpServerName.c_str(), "time.nist.gov");

  // MQTT
  if (config.mqttEnabled && config.mqttBroker.length() > 0)
  {
    mqtt.setServer(config.mqttBroker.c_str(), config.mqttPort);
    mqtt.setCallback(mqttCallback);
    Serial.println("MQTT configured: " + config.mqttBroker);
  }

  // Routes
  server.on("/login", handleLogin);
  server.on("/logout", handleLogout);
  server.on("/changepass", handleChangePassword);
  server.on("/ping", []() { server.send(200, "text/plain", "pong"); });
  server.on("/style.css", []() { server.send(200, "text/css", PAGE_Style_css); });
  server.on("/script.js", []() { server.send(200, "text/javascript", PAGE_Script_js); });
  server.on("/favicon.svg", []() { server.send(200, "image/svg+xml", PAGE_Favicon_svg); });
  server.on("/manifest.json", []() { server.send(200, "application/manifest+json", PAGE_Manifest_json); });
  server.on("/sw.js", []() { server.send(200, "application/javascript", PAGE_ServiceWorker_js); });

  server.on("/", []() { if (requireAuth()) server.send(200, "text/html", PAGE_Index); });
  server.on("/status", []() { if (requireAuth()) send_status_json(); });
  server.on("/toggle", []() { if (requireAuth()) toggleHeaterState(); });
  server.on("/setpoint", []() { if (requireAuth()) set_setpoint(); });
  server.on("/schedule.html", []() { if (requireAuth()) send_schedule_html(); });
  server.on("/schedule.json", []() { if (requireAuth()) send_schedule_json(); });
  server.on("/config.html", []() { if (requireAuth()) send_network_configuration_html(); });
  server.on("/scan", []() { if (requireAuth()) send_scan_json(); });
  server.on("/geocode", []() { if (requireAuth()) send_geocode_json(); });
  server.on("/mqtt.html", []() { if (requireAuth()) send_mqtt_html(); });
  server.on("/mqtt.json", []() { if (requireAuth()) send_mqtt_json(); });
  server.on("/info.html", []() { if (requireAuth()) server.send(200, "text/html", PAGE_Information); });
  server.on("/info.json", []() { if (requireAuth()) send_info_json(); });
  server.on("/history.json", []() { if (requireAuth()) send_history_json(); });
  server.on("/runtime.html", []() { if (requireAuth()) server.send(200, "text/html", PAGE_Runtime); });
  server.on("/runtime.json", []() { if (requireAuth()) send_runtime_json(); });
  server.on("/forecast.json", []() { if (requireAuth()) {
    String j = "{\"temps\":[";
    for (int i = 0; i < FORECAST_HOURS; i++) {
      if (i) j += ",";
      j += forecastValid ? String(forecastTemp[i], 1) : "null";
    }
    j += "],\"hours\":[";
    for (int i = 0; i < FORECAST_HOURS; i++) {
      if (i) j += ",";
      j += String(forecastHour[i]);
    }
    j += "],\"codes\":[";
    for (int i = 0; i < FORECAST_HOURS; i++) {
      if (i) j += ",";
      j += String(forecastCode[i]);
    }
    j += "]}";
    server.send(200, "application/json", j);
  }});
  server.on("/backup.json", []() { if (requireAuth()) send_backup_json(); });
  server.on("/restore", HTTP_POST, []() { if (requireAuth()) handle_restore(); });
  server.on("/reboot", []() { if (requireAuth()) handle_reboot(); });
  server.on("/serial", []() {
    if (!requireAuth()) return;
    String out;
    if (serialLogPos > 0 && serialLog[serialLogPos] == '\0')
    {
      out = String(serialLog);
    }
    else
    {
      out.reserve(SERIAL_LOG_SIZE);
      for (size_t i = 0; i < SERIAL_LOG_SIZE; i++)
      {
        size_t idx = (serialLogPos + i) % SERIAL_LOG_SIZE;
        if (serialLog[idx] == '\0') continue;
        out += serialLog[idx];
      }
    }
    server.send(200, "text/plain", out);
  });
  server.on("/admin/values", []() { if (requireAuth()) send_network_configuration_values_html(); });
  server.on("/admin/connectionstate", []() { if (requireAuth()) send_connection_state_values_html(); });
  server.on("/admin/infovalues", []() { if (requireAuth()) send_information_values_html(); });
  server.on("/admin/generalvalues", []() { if (requireAuth()) send_general_configuration_values_html(); });
  server.on("/weather", []() { if (requireAuth()) send_weather_json(); });
  server.on("/weather/refresh", []() { if (requireAuth()) { manual_weather_trigger = true; server.send(200, "application/json", "{\"ok\":true}"); } });

  server.onNotFound([]() { server.send(404, "text/html", "Page not Found"); });
  const char* headerKeys[] = {"Cookie"};
  size_t headerKeysSize = sizeof(headerKeys) / sizeof(char*);
  server.collectHeaders(headerKeys, headerKeysSize);

  server.begin();
  Serial.println("HTTP server started");
  serialLogPrintln("HTTP server started");

  time_read = millis();
  time_logic = millis();
  time_weather = millis();
  time_history = millis();
  time_mqtt = millis();
  time_rate = millis();

  digitalWrite(16, LOW);
  pinMode(16, INPUT);
  pinMode(relayPin, INPUT);
  digitalWrite(relayPin, LOW);
  pinMode(0, INPUT);
  pinMode(BUILTIN_LED, OUTPUT);
  digitalWrite(BUILTIN_LED, HIGH);
}

void loop()
{
  // WiFi reconnect logic
  if (!firstConnect && WiFi.status() == WL_CONNECTED)
  {
    Serial.println("Local IP: " + WiFi.localIP().toString());
    serialLogPrintln("Local IP: " + WiFi.localIP().toString());
    manual_weather_trigger = true;
    firstConnect = true;
  }
  else if (WiFi.status() == WL_DISCONNECTED || WiFi.status() == WL_CONNECTION_LOST || WiFi.status() == WL_CONNECT_FAILED)
  {
    if (firstConnect) time_reconnect = millis();
    firstConnect = false;
  }
  else
  {
    time_reconnect = millis();
  }

  if ((millis() - time_reconnect) > 5000 && config.ssid != "" && retryCount < 10)
  {
    Serial.println("Attempting to re-connect to " + config.ssid);
    WiFi.begin(config.ssid.c_str(), config.password.c_str());
    time_reconnect = millis();
    retryCount++;
  }

  server.handleClient();
  ArduinoOTA.handle();

  // MQTT
  if (config.mqttEnabled && config.mqttBroker.length() > 0)
  {
    if (!mqtt.connected() && WiFi.status() == WL_CONNECTED)
    {
      String clientId = "garage-" + String(ESP.getChipId(), HEX);
      if (config.mqttUser.length() > 0)
        mqtt.connect(clientId.c_str(), config.mqttUser.c_str(), config.mqttPass.c_str());
      else
        mqtt.connect(clientId.c_str());
      if (mqtt.connected())
      {
        mqtt.subscribe((config.mqttTopic + "/set").c_str());
        Serial.println("MQTT connected");
      }
    }
    mqtt.loop();
    if ((millis() - time_mqtt) > 60000 && mqtt.connected())
    {
      time_mqtt = millis();
      mqttPublish();
    }
  }

  // GPIO0 long press reset
  if (digitalRead(0) == LOW)
  {
    if (!GPIO0Pushed)
    {
      time_gpio0 = millis();
      GPIO0Pushed = true;
    }
    if ((millis() - time_gpio0) > 5000)
    {
      Serial.println("Admin Reset");
      config.ssid = "";
      config.password = "";
      config.HeatSP = 50;
      config.HeatHist = 4;
      config.HeatStatus = false;
      config.scheduleEnabled = false;
      config.loginUser = "admin";
      config.loginHash = simpleHash("heater");
      config.sessionToken = "";
      WriteConfig();
      delay(1000);
      ESP.restart();
    }
  }
  else
  {
    GPIO0Pushed = false;
  }

  // Temperature reading
  if ((millis() - time_read) > TEMP_READ)
  {
    time_read = millis();
    uint16_t val = analogRead(A0);
    yield();
    delay(2);
    Temp_Hist[TEMP_ITER] = Thermister(val) * 9.0 / 5.0 + 32.0;
    TEMP_ITER++;
    if (TEMP_ITER >= AVG_SIZE) TEMP_ITER = 0;

    double sum = 0;
    for (int i = 0; i < AVG_SIZE; i++) sum += Temp_Hist[i];
    Temp_Avg = sum / AVG_SIZE;
    logicReady = true;
  }

  // Rate sampling (every 10 seconds)
  if ((millis() - time_rate) > 10000 && logicReady)
  {
    time_rate = millis();
    rateSamples[rateIndex] = Temp_Avg;
    rateIndex = (rateIndex + 1) % RATE_SAMPLES;
    if (rateCount < RATE_SAMPLES) rateCount++;

    if (rateCount >= 2)
    {
      // Calculate slope over available samples (each sample is 10 seconds apart)
      float oldest = rateSamples[(rateIndex - rateCount + RATE_SAMPLES) % RATE_SAMPLES];
      float newest = rateSamples[(rateIndex - 1 + RATE_SAMPLES) % RATE_SAMPLES];
      float minutes = (rateCount - 1) * 10.0 / 60.0;
      tempRate = (newest - oldest) / minutes;
    }
  }

  // History sampling (every 5 minutes)
  if ((millis() - time_history) > HISTORY_INTERVAL && logicReady)
  {
    time_history = millis();
    historyIndoor[historyHead] = Temp_Avg;
    historyOutdoor[historyHead] = weatherValid ? outdoorTemp : NAN;
    time_t now = time(nullptr);
    historyTime[historyHead] = (now > 100000) ? now : 0;
    historyHead = (historyHead + 1) % HISTORY_SIZE;
    if (historyCount < HISTORY_SIZE) historyCount++;
  }

  // Control logic with schedule
  if (((millis() - time_logic) > LOGIC_CHANGE) && logicReady)
  {
    time_logic = millis();

    bool scheduleAllows = true;
    if (config.scheduleEnabled)
    {
      time_t now = time(nullptr);
      if (now > 100000)
      {
        struct tm *timeinfo = localtime(&now);
        int dow = timeinfo->tm_wday; // 0=Sunday
        int hour = timeinfo->tm_hour;
        if (dow >= 0 && dow < 7 && hour >= 0 && hour < 24)
          scheduleAllows = (config.scheduleHours[dow] & (1UL << hour)) != 0;
      }
    }

    if (config.HeatStatus && scheduleAllows)
    {
      if (Temp_Avg > (config.HeatSP + config.HeatHist))
      {
        config.RelayState = false;
      }
      else if (Temp_Avg < config.HeatSP)
      {
        config.RelayState = true;
      }
    }
    else
    {
      config.RelayState = false;
    }
  }

  // Runtime tracking
  if (config.RelayState && !runtimeWasOn)
  {
    runtimeLastOn = millis();
    runtimeWasOn = true;
  }
  else if (!config.RelayState && runtimeWasOn)
  {
    uint32_t elapsed = (millis() - runtimeLastOn) / 1000;
    runtimeToday += elapsed;
    runtimeTotal += elapsed;
    time_t now = time(nullptr);
    if (now > 100000)
    {
      struct tm *ti = localtime(&now);
      if (ti->tm_hour >= 0 && ti->tm_hour < 24)
        runtimeHourly[ti->tm_hour] += elapsed;
    }
    runtimeWasOn = false;
    config.runtimeTotal = runtimeTotal;
  }

  // Relay control
  if (config.RelayState)
    pinMode(relayPin, OUTPUT);
  else
    pinMode(relayPin, INPUT);

  // Weather update
  if ((manual_weather_trigger || ((millis() - time_weather) > WEATHER_UPDATE)) && WiFi.status() == WL_CONNECTED)
  {
    manual_weather_trigger = false;
    fetchWeather();
    time_weather = millis();
  }

  delay(10);
}

void send_mqtt_html()
{
  if (server.args() > 0)
  {
    config.mqttEnabled = server.hasArg("enabled");
    if (server.hasArg("broker")) config.mqttBroker = server.arg("broker");
    if (server.hasArg("port")) config.mqttPort = server.arg("port").toInt();
    if (server.hasArg("user")) config.mqttUser = server.arg("user");
    if (server.hasArg("pass") && server.arg("pass").length() > 0) config.mqttPass = server.arg("pass");
    if (server.hasArg("topic")) config.mqttTopic = server.arg("topic");
    WriteConfig();
    if (config.mqttEnabled && config.mqttBroker.length() > 0)
    {
      mqtt.setServer(config.mqttBroker.c_str(), config.mqttPort);
      mqtt.disconnect();
    }
    server.sendHeader("Location", "/mqtt.html", true);
    server.send(302, "text/plain", "");
    return;
  }
  server.send(200, "text/html", PAGE_MQTT);
}

void send_mqtt_json()
{
  String json = "{";
  json += "\"enabled\":" + String(config.mqttEnabled ? "true" : "false") + ",";
  json += "\"broker\":\"" + config.mqttBroker + "\",";
  json += "\"port\":" + String(config.mqttPort) + ",";
  json += "\"user\":\"" + config.mqttUser + "\",";
  json += "\"topic\":\"" + config.mqttTopic + "\",";
  json += "\"connected\":" + String(mqtt.connected() ? "true" : "false");
  json += "}";
  server.send(200, "application/json", json);
}

void mqttCallback(char* topic, byte* payload, unsigned int length)
{
  String msg;
  for (unsigned int i = 0; i < length; i++) msg += (char)payload[i];
  msg.trim();
  Serial.println("MQTT: " + msg);
  if (msg == "on" || msg == "1") config.HeatStatus = true;
  else if (msg == "off" || msg == "0") config.HeatStatus = false;
  else if (msg.startsWith("sp:"))
  {
    int sp = msg.substring(3).toInt();
    if (sp >= 35 && sp <= 80) config.HeatSP = sp;
  }
  WriteConfig();
}

void mqttPublish()
{
  if (!mqtt.connected()) return;
  String j = "{\"temp\":" + String(Temp_Avg, 1);
  j += ",\"sp\":" + String(config.HeatSP);
  j += ",\"enabled\":" + String(config.HeatStatus ? "true" : "false");
  j += ",\"relay\":" + String(config.RelayState ? "true" : "false");
  j += ",\"runtime\":" + String(runtimeToday);
  j += ",\"rate\":" + String(tempRate, 2);
  j += "}";
  mqtt.publish(config.mqttTopic.c_str(), j.c_str());
}

double Thermister(int val)
{
  double V_NTC = (double)val / 1024.0;
  double R_NTC = (Rs * V_NTC) / (Vcc - V_NTC);
  R_NTC = log(R_NTC);
  double Temp = 1.0 / (0.001129148 + (0.000234125 + (0.0000000876741 * R_NTC * R_NTC)) * R_NTC);
  Temp = Temp - 273.15;
  return Temp;
}

void fetchWeather()
{
  if (WiFi.status() != WL_CONNECTED) return;

  WiFiClient client;
  HTTPClient http;
  String url = "http://api.open-meteo.com/v1/forecast?latitude=" + String(config.latitude, 4) +
               "&longitude=" + String(config.longitude, 4) +
               "&current=temperature_2m,relative_humidity_2m,weather_code" +
               "&hourly=temperature_2m,weather_code&forecast_hours=12&temperature_unit=fahrenheit" +
               "&timezone=auto";

  Serial.println("Fetching weather: " + url);
  serialLogPrintln("Fetching weather: " + url);
  http.begin(client, url);
  int httpCode = http.GET();

  if (httpCode == 200)
  {
    String payload = http.getString();
    DynamicJsonDocument doc(3072);
    DeserializationError error = deserializeJson(doc, payload);
    if (!error)
    {
      outdoorTemp = doc["current"]["temperature_2m"] | 0.0;
      outdoorHumidity = doc["current"]["relative_humidity_2m"] | 0;
      int code = doc["current"]["weather_code"] | 0;
      outdoorCondition = weatherCodeToString(code);
      outdoorIcon = weatherCodeToIcon(code);
      weatherValid = true;
      time_t now = time(nullptr);
      weatherLastUpdate = String(ctime(&now));
      weatherLastUpdate.trim();

      // Hourly forecast (times are now in local timezone via timezone=auto)
      JsonArray hourly = doc["hourly"]["temperature_2m"];
      JsonArray times = doc["hourly"]["time"];
      JsonArray codes = doc["hourly"]["weather_code"];
      if (hourly.size() >= FORECAST_HOURS)
      {
        for (int i = 0; i < FORECAST_HOURS; i++)
        {
          forecastTemp[i] = hourly[i] | 0.0;
          forecastCode[i] = codes[i] | 0;
          String t = times[i] | "";
          int h = t.length() >= 13 ? t.substring(11, 13).toInt() : i;
          forecastHour[i] = h;
        }
        forecastValid = true;
      }
      Serial.printf("Weather: %.1f F, %d%%, %s\n", outdoorTemp, outdoorHumidity, outdoorCondition.c_str());
    }
  }
  else
  {
    Serial.printf("Weather HTTP error: %d\n", httpCode);
    weatherValid = false;
  }
  http.end();
}

void send_runtime_json()
{
  String json = "{\"today\":" + String(runtimeToday);
  json += ",\"total\":" + String(runtimeTotal);
  json += ",\"hourly\":[";
  for (int i = 0; i < 24; i++)
  {
    if (i) json += ",";
    json += String(runtimeHourly[i]);
  }
  json += "]}";
  server.send(200, "application/json", json);
}

void send_backup_json()
{
  String json = "{";
  json += "\"ssid\":\"" + config.ssid + "\",";
  json += "\"HeatSP\":" + String(config.HeatSP) + ",";
  json += "\"HeatHist\":" + String(config.HeatHist) + ",";
  json += "\"HeatStatus\":" + String(config.HeatStatus ? "true" : "false") + ",";
  json += "\"scheduleEnabled\":" + String(config.scheduleEnabled ? "true" : "false") + ",";
  json += "\"latitude\":" + String(config.latitude, 4) + ",";
  json += "\"longitude\":" + String(config.longitude, 4) + ",";
  json += "\"zipCode\":\"" + config.zipCode + "\",";
  json += "\"locationName\":\"" + config.locationName + "\",";
  json += "\"mqttBroker\":\"" + config.mqttBroker + "\",";
  json += "\"mqttPort\":" + String(config.mqttPort) + ",";
  json += "\"mqttTopic\":\"" + config.mqttTopic + "\",";
  json += "\"mqttEnabled\":" + String(config.mqttEnabled ? "true" : "false");
  json += "}";
  server.send(200, "application/json", json);
}

void handle_restore()
{
  if (!server.hasArg("plain")) { server.send(400, "application/json", "{\"ok\":false}"); return; }
  String body = server.arg("plain");
  DynamicJsonDocument doc(1024);
  if (deserializeJson(doc, body))
  {
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"parse\"}");
    return;
  }
  if (doc.containsKey("HeatSP")) config.HeatSP = doc["HeatSP"];
  if (doc.containsKey("HeatHist")) config.HeatHist = doc["HeatHist"];
  if (doc.containsKey("HeatStatus")) config.HeatStatus = doc["HeatStatus"];
  if (doc.containsKey("scheduleEnabled")) config.scheduleEnabled = doc["scheduleEnabled"];
  if (doc.containsKey("latitude")) config.latitude = doc["latitude"];
  if (doc.containsKey("longitude")) config.longitude = doc["longitude"];
  if (doc.containsKey("zipCode")) config.zipCode = doc["zipCode"].as<String>();
  if (doc.containsKey("locationName")) config.locationName = doc["locationName"].as<String>();
  if (doc.containsKey("mqttBroker")) config.mqttBroker = doc["mqttBroker"].as<String>();
  if (doc.containsKey("mqttPort")) config.mqttPort = doc["mqttPort"];
  if (doc.containsKey("mqttTopic")) config.mqttTopic = doc["mqttTopic"].as<String>();
  if (doc.containsKey("mqttEnabled")) config.mqttEnabled = doc["mqttEnabled"];
  WriteConfig();
  server.send(200, "application/json", "{\"ok\":true}");
  delay(500);
  ESP.restart();
}

void handle_reboot()
{
  server.send(200, "application/json", "{\"ok\":true}");
  delay(500);
  ESP.restart();
}

void send_history_json()
{
  String json = "{\"indoor\":[";
  for (uint16_t i = 0; i < historyCount; i++)
  {
    uint16_t idx = (historyHead - historyCount + i + HISTORY_SIZE) % HISTORY_SIZE;
    if (i > 0) json += ",";
    json += String(historyIndoor[idx], 1);
  }
  json += "],\"outdoor\":[";
  for (uint16_t i = 0; i < historyCount; i++)
  {
    uint16_t idx = (historyHead - historyCount + i + HISTORY_SIZE) % HISTORY_SIZE;
    if (i > 0) json += ",";
    if (isnan(historyOutdoor[idx]))
      json += "null";
    else
      json += String(historyOutdoor[idx], 1);
  }
  json += "],\"times\":[";
  // Find most recent valid timestamp to anchor calculations
  uint32_t anchorTime = 0;
  int16_t anchorIdx = -1;
  for (int16_t i = historyCount - 1; i >= 0; i--)
  {
    uint16_t idx = (historyHead - historyCount + i + HISTORY_SIZE) % HISTORY_SIZE;
    if (historyTime[idx] > 100000)
    {
      anchorTime = historyTime[idx];
      anchorIdx = i;
      break;
    }
  }
  for (uint16_t i = 0; i < historyCount; i++)
  {
    if (i > 0) json += ",";
    uint32_t t = 0;
    if (anchorIdx >= 0)
    {
      // Calculate time based on position relative to anchor (5 min intervals)
      int16_t offset = i - anchorIdx;
      t = anchorTime + (offset * 300);
    }
    else
    {
      // No valid anchor, try stored time
      uint16_t idx = (historyHead - historyCount + i + HISTORY_SIZE) % HISTORY_SIZE;
      t = historyTime[idx];
    }
    if (t > 100000)
    {
      time_t tt = t;
      struct tm *ti = localtime(&tt);
      char buf[6];
      sprintf(buf, "%02d:%02d", ti->tm_hour, ti->tm_min);
      json += "\"" + String(buf) + "\"";
    }
    else
    {
      json += "null";
    }
  }
  json += "]}";
  server.send(200, "application/json", json);
}

void send_info_json()
{
  unsigned long sec = millis() / 1000;
  unsigned long days = sec / 86400;
  unsigned long hours = (sec % 86400) / 3600;
  unsigned long mins = (sec % 3600) / 60;
  char uptimeBuf[32];
  snprintf(uptimeBuf, sizeof(uptimeBuf), "%lud %02luh %02lum", days, hours, mins);

  String json = "{";
  json += "\"version\":\"" + String(FIRMWARE_VERSION) + "\",";
  json += "\"mac\":\"" + GetMacAddress() + "\",";
  json += "\"ip\":\"" + WiFi.localIP().toString() + "\",";
  json += "\"heap\":" + String(ESP.getFreeHeap()) + ",";
  json += "\"uptime\":\"" + String(uptimeBuf) + "\"";
  json += "}";
  server.send(200, "application/json", json);
}

void toggleHeaterState()
{
  config.HeatStatus = !config.HeatStatus;
  WriteConfig();
  send_status_json();
  if (mqtt.connected()) mqttPublish();
}

void set_setpoint()
{
  if (server.hasArg("sp"))
  {
    int sp = server.arg("sp").toInt();
    if (sp >= 35 && sp <= 80) config.HeatSP = sp;
    if (server.hasArg("hist"))
    {
      int h = server.arg("hist").toInt();
      if (h >= 1 && h <= 10) config.HeatHist = h;
    }
    WriteConfig();
  }
  send_status_json();
}

void send_status_json()
{
  time_t now = time(nullptr);
  String timeStr = "--:--";
  if (now > 100000) {
    struct tm *ti = localtime(&now);
    char buf[6];
    sprintf(buf, "%02d:%02d", ti->tm_hour, ti->tm_min);
    timeStr = String(buf);
  }

  String json = "{";
  json += "\"temp\":" + String(Temp_Avg, 1) + ",";
  json += "\"sp\":" + String(config.HeatSP) + ",";
  json += "\"hist\":" + String(config.HeatHist) + ",";
  json += "\"enabled\":" + String(config.HeatStatus ? "true" : "false") + ",";
  json += "\"relay\":" + String(config.RelayState ? "true" : "false") + ",";
  json += "\"schedule\":" + String(config.scheduleEnabled ? "true" : "false") + ",";
  json += "\"outdoor\":" + String(weatherValid ? String(outdoorTemp, 1) : "null") + ",";
  json += "\"humidity\":" + String(outdoorHumidity) + ",";
  json += "\"condition\":\"" + outdoorCondition + "\",";
  json += "\"icon\":\"" + outdoorIcon + "\",";
  json += "\"time\":\"" + timeStr + "\",";
  json += "\"zip\":\"" + config.zipCode + "\",";
  json += "\"location\":\"" + config.locationName + "\",";
  json += "\"lat\":" + String(config.latitude, 4) + ",";
  json += "\"lon\":" + String(config.longitude, 4) + ",";
  json += "\"runtimeToday\":" + String(runtimeToday) + ",";
  json += "\"rate\":" + String(tempRate, 2);
  json += "}";
  server.send(200, "application/json", json);
}

void send_weather_json()
{
  String json = "{";
  json += "\"temp\":" + String(outdoorTemp, 1) + ",";
  json += "\"humidity\":" + String(outdoorHumidity) + ",";
  json += "\"condition\":\"" + outdoorCondition + "\",";
  json += "\"icon\":\"" + outdoorIcon + "\",";
  json += "\"valid\":" + String(weatherValid ? "true" : "false") + ",";
  json += "\"updated\":\"" + weatherLastUpdate + "\"";
  json += "}";
  server.send(200, "application/json", json);
}

// Config write/read implementations
void WriteConfig()
{
  Serial.println("Writing Config");
  EEPROM.write(0, 'C');
  EEPROM.write(1, 'F');
  EEPROM.write(2, 'G');

  EEPROM.write(16, config.dhcp);
  EEPROM.write(17, config.daylight);
  EEPROMWritelong(22, config.timezone);

  EEPROM.write(32, config.IP[0]);
  EEPROM.write(33, config.IP[1]);
  EEPROM.write(34, config.IP[2]);
  EEPROM.write(35, config.IP[3]);
  EEPROM.write(36, config.Netmask[0]);
  EEPROM.write(37, config.Netmask[1]);
  EEPROM.write(38, config.Netmask[2]);
  EEPROM.write(39, config.Netmask[3]);
  EEPROM.write(40, config.Gateway[0]);
  EEPROM.write(41, config.Gateway[1]);
  EEPROM.write(42, config.Gateway[2]);
  EEPROM.write(43, config.Gateway[3]);

  WriteStringToEEPROM(64, config.ssid);
  WriteStringToEEPROM(96, config.password);
  WriteStringToEEPROM(128, config.ntpServerName);

  EEPROM.write(300, config.AutoTurnOn);
  EEPROM.write(301, config.AutoTurnOff);
  EEPROM.write(302, config.TurnOnHour);
  EEPROM.write(303, config.TurnOnMinute);
  EEPROM.write(304, config.TurnOffHour);
  EEPROM.write(305, config.TurnOffMinute);
  EEPROM.write(306, config.HeatSP);
  EEPROM.write(307, config.HeatHist);
  EEPROM.write(308, config.HeatStatus);
  EEPROM.write(309, config.AdminEnabled);
  EEPROM.write(310, config.scheduleEnabled);
  EEPROMWritefloat(311, config.latitude);
  EEPROMWritefloat(315, config.longitude);
  for (int d = 0; d < 7; d++)
    EEPROMWritelong(400 + d * 4, config.scheduleHours[d]);
  EEPROMWritelong(428, config.loginHash);
  WriteStringToEEPROM(432, config.loginUser);
  WriteStringToEEPROM(320, config.DeviceName);
  WriteStringToEEPROM(460, config.zipCode);
  WriteStringToEEPROM(480, config.locationName);
  EEPROMWritelong(200, config.runtimeTotal);
  EEPROM.write(204, config.mqttEnabled);
  EEPROM.write(205, config.mqttPort & 0xFF);
  EEPROM.write(206, (config.mqttPort >> 8) & 0xFF);

  EEPROM.commit();
}

boolean ReadConfig()
{
  Serial.println("Reading Configuration");
  if (EEPROM.read(0) == 'C' && EEPROM.read(1) == 'F' && EEPROM.read(2) == 'G')
  {
    Serial.println("Configuration Found!");
    config.dhcp = EEPROM.read(16);
    config.daylight = EEPROM.read(17);
    config.timezone = EEPROMReadlong(22);

    config.IP[0] = EEPROM.read(32);
    config.IP[1] = EEPROM.read(33);
    config.IP[2] = EEPROM.read(34);
    config.IP[3] = EEPROM.read(35);
    config.Netmask[0] = EEPROM.read(36);
    config.Netmask[1] = EEPROM.read(37);
    config.Netmask[2] = EEPROM.read(38);
    config.Netmask[3] = EEPROM.read(39);
    config.Gateway[0] = EEPROM.read(40);
    config.Gateway[1] = EEPROM.read(41);
    config.Gateway[2] = EEPROM.read(42);
    config.Gateway[3] = EEPROM.read(43);

    config.ssid = ReadStringFromEEPROM(64);
    config.password = ReadStringFromEEPROM(96);
    config.ntpServerName = ReadStringFromEEPROM(128);

    config.AutoTurnOn = EEPROM.read(300);
    config.AutoTurnOff = EEPROM.read(301);
    config.TurnOnHour = EEPROM.read(302);
    config.TurnOnMinute = EEPROM.read(303);
    config.TurnOffHour = EEPROM.read(304);
    config.TurnOffMinute = EEPROM.read(305);
    config.HeatSP = EEPROM.read(306);
    config.HeatHist = EEPROM.read(307);
    config.HeatStatus = EEPROM.read(308);
    config.AdminEnabled = EEPROM.read(309);
    config.scheduleEnabled = EEPROM.read(310);
    config.latitude = EEPROMReadfloat(311);
    config.longitude = EEPROMReadfloat(315);
    for (int d = 0; d < 7; d++)
      config.scheduleHours[d] = EEPROMReadlong(400 + d * 4);
    config.loginHash = EEPROMReadlong(428);
    config.loginUser = ReadStringFromEEPROM(432);
    if (config.loginUser.length() == 0) config.loginUser = "admin";
    config.zipCode = ReadStringFromEEPROM(460);
    config.locationName = ReadStringFromEEPROM(480);
    config.runtimeTotal = EEPROMReadlong(200);
    config.mqttEnabled = EEPROM.read(204);
    config.mqttPort = EEPROM.read(205) | (EEPROM.read(206) << 8);
    if (config.mqttPort == 0 || config.mqttPort == 0xFFFF) config.mqttPort = 1883;
    if (config.loginHash == 0 || config.loginHash == 0xFFFFFFFF)
    {
      config.loginUser = "admin";
      config.loginHash = simpleHash("heater");
      Serial.println("Login credentials reset to defaults");
    }
    Serial.printf("Login user='%s' hash=%lu\n", config.loginUser.c_str(), config.loginHash);
    config.DeviceName = ReadStringFromEEPROM(320);
    return true;
  }
  else
  {
    Serial.println("Configuration NOT FOUND!!!!");
    return false;
  }
}

void ConfigureWifi()
{
  Serial.println("Configuring Wifi");
  if (config.ssid != "")
  {
    Serial.println("Connecting to " + config.ssid);
    WiFi.begin(config.ssid.c_str(), config.password.c_str());
    if (!config.dhcp)
    {
      WiFi.config(IPAddress(config.IP[0], config.IP[1], config.IP[2], config.IP[3]),
                  IPAddress(config.Gateway[0], config.Gateway[1], config.Gateway[2], config.Gateway[3]),
                  IPAddress(config.Netmask[0], config.Netmask[1], config.Netmask[2], config.Netmask[3]));
    }
  }
}
