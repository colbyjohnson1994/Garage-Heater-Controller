#ifndef HELPERS_H
#define HELPERS_H

#include <EEPROM.h>
#include "global.h"

static const uint8_t monthDays[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
#define LEAP_YEAR(Y) ( ((1970+Y)>0) && !((1970+Y)%4) && ( ((1970+Y)%100) || !((1970+Y)%400) ) )

struct strDateTime
{
  byte hour;
  byte minute;
  byte second;
  int year;
  byte month;
  byte day;
  byte wday;
};

boolean checkRange(String Value)
{
  int v = Value.toInt();
  return (v >= 0 && v <= 255);
}

void WriteStringToEEPROM(int beginaddress, String string)
{
  char charBuf[string.length() + 1];
  string.toCharArray(charBuf, string.length() + 1);
  for (int t = 0; t < sizeof(charBuf); t++)
  {
    EEPROM.write(beginaddress + t, charBuf[t]);
  }
}

String ReadStringFromEEPROM(int beginaddress)
{
  byte counter = 0;
  char rChar;
  String retString = "";
  while (1)
  {
    rChar = EEPROM.read(beginaddress + counter);
    if (rChar == 0 || rChar == (char)0xFF) break;
    if (counter > 31) break;
    counter++;
    retString.concat(rChar);
  }
  return retString;
}

void EEPROMWritelong(int address, long value)
{
  byte four = (value & 0xFF);
  byte three = ((value >> 8) & 0xFF);
  byte two = ((value >> 16) & 0xFF);
  byte one = ((value >> 24) & 0xFF);
  EEPROM.write(address, four);
  EEPROM.write(address + 1, three);
  EEPROM.write(address + 2, two);
  EEPROM.write(address + 3, one);
}

long EEPROMReadlong(long address)
{
  long four = EEPROM.read(address);
  long three = EEPROM.read(address + 1);
  long two = EEPROM.read(address + 2);
  long one = EEPROM.read(address + 3);
  return ((four << 0) & 0xFF) + ((three << 8) & 0xFFFF) + ((two << 16) & 0xFFFFFF) + ((one << 24) & 0xFFFFFFFF);
}

void EEPROMWritefloat(int address, float value)
{
  byte* p = (byte*)(void*)&value;
  for (int i = 0; i < 4; i++)
    EEPROM.write(address + i, p[i]);
}

float EEPROMReadfloat(int address)
{
  float value;
  byte* p = (byte*)(void*)&value;
  for (int i = 0; i < 4; i++)
    p[i] = EEPROM.read(address + i);
  return value;
}

String GetMacAddress()
{
  uint8_t mac[6];
  char macStr[18] = {0};
  WiFi.macAddress(mac);
  sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  return String(macStr);
}

unsigned char h2int(char c)
{
  if (c >= '0' && c <= '9') return ((unsigned char)c - '0');
  if (c >= 'a' && c <= 'f') return ((unsigned char)c - 'a' + 10);
  if (c >= 'A' && c <= 'F') return ((unsigned char)c - 'A' + 10);
  return 0;
}

String urldecode(String input)
{
  char c;
  String ret = "";
  for (byte t = 0; t < input.length(); t++)
  {
    c = input[t];
    if (c == '+') c = ' ';
    if (c == '%') {
      t++;
      c = input[t];
      t++;
      c = (h2int(c) << 4) | h2int(input[t]);
    }
    ret.concat(c);
  }
  return ret;
}

bool isAuthenticated()
{
  if (config.sessionToken.length() == 0) return false;
  if (server.hasHeader("Cookie")) {
    String cookie = server.header("Cookie");
    Serial.println("Cookie: " + cookie);
    if (cookie.indexOf("session=" + config.sessionToken) != -1) {
      return true;
    }
  } else {
    Serial.println("No Cookie header");
  }
  return false;
}

// Simple non-cryptographic hash for password storage (sufficient behind TLS proxy)
uint32_t simpleHash(const String& s)
{
  uint32_t h = 2166136261u;
  for (size_t i = 0; i < s.length(); i++) {
    h ^= (uint8_t)s[i];
    h *= 16777619u;
  }
  return h;
}

// Weather condition from WMO code + simple icon
String weatherCodeToString(int code) {
  switch(code) {
    case 0: return "Clear";
    case 1: case 2: case 3: return "Partly Cloudy";
    case 45: case 48: return "Fog";
    case 51: case 53: case 55: return "Drizzle";
    case 61: case 63: case 65: return "Rain";
    case 71: case 73: case 75: return "Snow";
    case 80: case 81: case 82: return "Showers";
    case 95: return "Thunderstorm";
    default: return "Unknown";
  }
}

String weatherCodeToIcon(int code) {
  switch(code) {
    case 0: return "☀️";
    case 1: case 2: case 3: return "⛅";
    case 45: case 48: return "🌫️";
    case 51: case 53: case 55: return "🌦️";
    case 61: case 63: case 65: return "🌧️";
    case 71: case 73: case 75: return "❄️";
    case 80: case 81: case 82: return "🌧️";
    case 95: return "⛈️";
    default: return "❓";
  }
}

#endif
