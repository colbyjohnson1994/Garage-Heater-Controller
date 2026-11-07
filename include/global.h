#ifndef GLOBAL_H
#define GLOBAL_H

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>

ESP8266WebServer server(80);

#define AVG_SIZE 5
#define TEMP_READ 1000 // 1 second
#define LOGIC_CHANGE  5000 // 5 seconds
#define WEATHER_UPDATE 900000 // 15 minutes

#define relayPin  5

uint8_t retryCount = 0;

uint32_t time_read = 0;
uint32_t time_logic = 0;
uint32_t time_reconnect = 0;
uint32_t time_weather = 0;
uint8_t TEMP_ITER = 0;
bool manual_weather_trigger = false;

uint32_t time_gpio0 = 0;
bool GPIO0Pushed = false;

bool logicReady = false;
bool firstConnect = false;

double Temp_Hist[AVG_SIZE];
double Temp_Avg = 0;

// Weather data
float outdoorTemp = 0;
int outdoorHumidity = 0;
String outdoorCondition = "Unknown";
String outdoorIcon = "❓";
bool weatherValid = false;
String weatherLastUpdate = "";

struct strConfig {
	String ssid;
	String password;
	byte  IP[4];
	byte  Netmask[4];
	byte  Gateway[4];
	boolean dhcp;
	String ntpServerName;
	long timezone; // hours, e.g. -7 for MDT
	boolean daylight;
	String DeviceName;
	boolean AutoTurnOff;
	boolean AutoTurnOn;
	byte TurnOffHour;
	byte TurnOffMinute;
	byte TurnOnHour;
	byte TurnOnMinute;
	byte HeatSP;
	byte HeatHist;
	bool HeatStatus;
	bool RelayState;
	bool AdminEnabled;
	// Location
	String zipCode;
	float latitude;
	float longitude;
	bool scheduleEnabled;
	uint32_t scheduleHours[7]; // bit 0 = hour 0, bit 23 = hour 23 for each day (0=Sun)
	String loginUser;
	uint32_t loginHash;
	String sessionToken; // runtime only, not persisted
} config;

void WriteConfig();
boolean ReadConfig();
void ConfigureWifi();

#endif
