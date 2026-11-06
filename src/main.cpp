/*
  ESP_WebConfig - Modernized Garage Heater Controller
  Original based on John Lassen's ESP_WebConfig
  Updated with modern UI, outdoor weather (Open-Meteo), and scheduling
*/

#include <math.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <EEPROM.h>
#include <time.h>
#include <ArduinoOTA.h>
#include "helpers.h"
#include "global.h"

#include "Page_Index.h"
#include "Page_General.h"
#include "Page_Network.h"
#include "Page_Information.h"
#include "Page_Login.h"
#include "Page_Style.css.h"
#include "Page_Script.js.h"

#define ACCESS_POINT_NAME "Garage Heater"
#define ACCESS_POINT_PASSWORD "12345678"
#define OTA_PASSWORD "heaterota"
#ifndef FIRMWARE_VERSION
#define FIRMWARE_VERSION "1.2.0"
#endif

unsigned int Rs = 150000;
double Vcc = 3.3;

void setup()
{
  EEPROM.begin(512);
  Serial.begin(115200);
  delay(500);
  Serial.println("Starting ESP8266 Garage Heater");

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
    config.latitude = 44.0805; // Rapid City default
    config.longitude = -103.2310;
    config.scheduleEnabled = false;
    for (int d = 0; d < 7; d++) config.scheduleHours[d] = 0x00FFFFFF; // all hours on by default
    config.loginUser = "admin";
    config.loginHash = simpleHash("heater");
    WriteConfig();
    Serial.println("Default config applied");
    delay(1000);
  }

  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(ACCESS_POINT_NAME, ACCESS_POINT_PASSWORD);
  ConfigureWifi();

  // ArduinoOTA (local network only)
  ArduinoOTA.setHostname("GarageHeater");
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

  // Routes
  server.on("/login", handleLogin);
  server.on("/logout", handleLogout);
  server.on("/changepass", handleChangePassword);
  server.on("/style.css", []() { server.send(200, "text/css", PAGE_Style_css); });
  server.on("/script.js", []() { server.send(200, "text/javascript", PAGE_Script_js); });

  server.on("/", []() { if (requireAuth()) server.send(200, "text/html", PAGE_Index); });
  server.on("/status", []() { if (requireAuth()) send_status_json(); });
  server.on("/toggle", []() { if (requireAuth()) toggleHeaterState(); });
  server.on("/setpoint", []() { if (requireAuth()) set_setpoint(); });
  server.on("/schedule.html", []() { if (requireAuth()) send_schedule_html(); });
  server.on("/schedule.json", []() { if (requireAuth()) send_schedule_json(); });
  server.on("/config.html", []() { if (requireAuth()) send_network_configuration_html(); });
  server.on("/scan", []() { if (requireAuth()) send_scan_json(); });
  server.on("/info.html", []() { if (requireAuth()) server.send(200, "text/html", PAGE_Information); });
  server.on("/info.json", []() { if (requireAuth()) send_info_json(); });
  server.on("/admin/values", []() { if (requireAuth()) send_network_configuration_values_html(); });
  server.on("/admin/connectionstate", []() { if (requireAuth()) send_connection_state_values_html(); });
  server.on("/admin/infovalues", []() { if (requireAuth()) send_information_values_html(); });
  server.on("/admin/generalvalues", []() { if (requireAuth()) send_general_configuration_values_html(); });
  server.on("/weather", []() { if (requireAuth()) send_weather_json(); });
  server.on("/weather/refresh", []() { if (requireAuth()) { manual_weather_trigger = true; server.send(200, "application/json", "{\"ok\":true}"); } });

  server.onNotFound([]() { server.send(404, "text/html", "Page not Found"); });
  // Must collect Cookie header or auth will always fail
  const char* headerKeys[] = {"Cookie"};
  size_t headerKeysSize = sizeof(headerKeys) / sizeof(char*);
  server.collectHeaders(headerKeys, headerKeysSize);

  server.begin();
  Serial.println("HTTP server started");

  time_read = millis();
  time_logic = millis();
  time_weather = millis();

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
      // Reset to defaults
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
               "&current=temperature_2m,relative_humidity_2m,weather_code&temperature_unit=fahrenheit";

  Serial.println("Fetching weather: " + url);
  http.begin(client, url);
  int httpCode = http.GET();

  if (httpCode == 200)
  {
    String payload = http.getString();
    DynamicJsonDocument doc(1024);
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
  // Local time
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
  json += "\"time\":\"" + timeStr + "\"";
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
    // Force known-good default if hash looks unset
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
    Serial.println("Configuration NOT FOUND");
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
