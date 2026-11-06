/* 
  ESP_WebConfig 

  Copyright (c) 2015 John Lassen. All rights reserved.
  This is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  This software is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.
  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

  Latest version: 1.1.3  - 2015-07-20
  Changed the loading of the Javascript and CCS Files, so that they will successively loaded and that only one request goes to the ESP.

  -----------------------------------------------------------------------------------------------
  History

  Version: 1.1.2  - 2015-07-17
  Added URLDECODE for some input-fields (SSID, PASSWORD...)

  Version  1.1.1 - 2015-07-12
  First initial version to the public



  
  */

#include <math.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include "helpers.h"
#include "global.h"
/*
Include the HTML, STYLE and Script "Pages"
*/
#include "Page_Login.h"
#include "Page_Index.h"
#include "Page_Admin.h"
#include "Page_Script.js.h"
#include "Page_Style.css.h"
#include "Page_Information.h"
#include "Page_General.h"
#include "PAGE_NetworkConfiguration.h"

#define ACCESS_POINT_NAME "Garage Heater" 
#define ACCESS_POINT_PASSWORD "12345678"

unsigned int Rs = 150000;
double Vcc = 3.3;

void setup ( void ) 
{
	EEPROM.begin(512);
	Serial.begin(115200);
	delay(500);
	Serial.println("Starting ES8266");
	if (!ReadConfig())
	{
		// DEFAULT CONFIG
		config.ssid = "";
		config.password = "";
		config.dhcp = true;
		config.IP[0] = 192;config.IP[1] = 168;config.IP[2] = 1;config.IP[3] = 100;
		config.Netmask[0] = 255;config.Netmask[1] = 255;config.Netmask[2] = 255;config.Netmask[3] = 0;
		config.Gateway[0] = 192;config.Gateway[1] = 168;config.Gateway[2] = 1;config.Gateway[3] = 1;
		config.ntpServerName = "pool.ntp.org";
		config.Update_Time_Via_NTP_Every =  0;
		config.timezone = -70;
		config.daylight = true;
		config.DeviceName = "Garage Heater";
		config.AutoTurnOff = false;
		config.AutoTurnOn = false;
		config.TurnOffHour = 0;
		config.TurnOffMinute = 0;
		config.TurnOnHour = 0;
		config.TurnOnMinute = 0;
    config.HeatSP = 50;
    config.HeatHist = 4;
    config.HeatStatus = false;
    config.AdminEnabled = false;
		WriteConfig();
		Serial.println("General config applied");
    Serial.println("Restarting ESP...");
    delay(3000);
    // cause a reset
    pinMode(16, OUTPUT);
	}
 
	WiFi.mode(WIFI_AP_STA);
	WiFi.softAP( ACCESS_POINT_NAME , ACCESS_POINT_PASSWORD);

	ConfigureWifi();
	

	server.on ( "/", []() { Serial.println("index.html"); server.send ( 200, "text/html", PAGE_ControlMainPage );   }  );
  server.on ( "/sendValues", send_status_values_html );
  server.on ( "/toggle", toggleHeaterState );
 
	server.on ( "/favicon.ico",   []() { Serial.println("favicon.ico"); server.send ( 200, "text/html", "" );   }  );


	server.on ( "/config.html", send_network_configuration_html );
	server.on ( "/info.html", []() { Serial.println("info.html"); server.send ( 200, "text/html", PAGE_Information );   }  );
	server.on ( "/setpoints.html", send_general_html  );
	server.on ( "/style.css", []() { Serial.println("style.css"); server.send ( 200, "text/plain", PAGE_Style_css );  } );
	server.on ( "/microajax.js", []() { Serial.println("microajax.js"); server.send ( 200, "text/plain", PAGE_microajax_js );  } );
	server.on ( "/admin/values", send_network_configuration_values_html );
	server.on ( "/admin/connectionstate", send_connection_state_values_html );
	server.on ( "/admin/infovalues", send_information_values_html );
	server.on ( "/admin/generalvalues", send_general_configuration_values_html);
  


	server.onNotFound ( []() { Serial.println("Page Not Found"); server.send ( 400, "text/html", "Page not Found" );   }  );
	server.begin();
	Serial.println( "HTTP server started" );

  time_read = millis(); // start the counter
  time_logic = millis();

  digitalWrite(16, LOW);
  pinMode(16, INPUT);
  pinMode(relayPin, INPUT);
  digitalWrite(relayPin, LOW);
  pinMode(0, INPUT);
  pinMode(BUILTIN_LED, OUTPUT);
  digitalWrite(BUILTIN_LED, HIGH);
}

 
void loop ( void ) 
{
  if (!firstConnect && WiFi.status() == WL_CONNECTED)
  {
    Serial.println("Local IP: " + (String) WiFi.localIP()[0] + "." +  (String) WiFi.localIP()[1] + "." +  (String) WiFi.localIP()[2] + "." + (String) WiFi.localIP()[3]);
    firstConnect = true;
  }
  else if(WiFi.status() == WL_DISCONNECTED || WiFi.status() == WL_CONNECTION_LOST || WiFi.status() == WL_CONNECT_FAILED)
  {
    if (firstConnect)
    {
      time_reconnect = millis();
    }
    firstConnect = false;
  }
  else
  {
    time_reconnect = millis();
  }

  if ((millis() - time_reconnect) > 5000 && config.ssid != "")
  {
    if (retryCount > 5)
    {
      // trying to prevent it restarting in the wrong clock mode
      // reboot
      Serial.println("Restarting ESP");
      delay(1000);
      // cause a reset
      pinMode(16, OUTPUT);
    }
    
    Serial.println("Attempting to re-connect to " + config.ssid + " with " + config.password);
    Serial.println("WiFi Status: " + (String)WiFi.status());
    WiFi.begin (config.ssid.c_str(), config.password.c_str());

    time_reconnect = millis();
    
    retryCount++;
  }
    
	server.handleClient();


	/*
	*    Your Code here
	*/

  if (digitalRead(0) == LOW)
  {
    // GPIO 0 button is pushed
    if (!GPIO0Pushed)
    {
      time_gpio0 = millis();
      GPIO0Pushed = true;
      Serial.println("GPIO0 Pushed!");
    }

    if ((millis() - time_gpio0) > 5000)
    {
      // reset exceeded
      Serial.println("Admin Reset");
      // DEFAULT CONFIG
      config.ssid = "";
      config.password = "";
      config.dhcp = true;
      config.IP[0] = 192;config.IP[1] = 168;config.IP[2] = 1;config.IP[3] = 100;
      config.Netmask[0] = 255;config.Netmask[1] = 255;config.Netmask[2] = 255;config.Netmask[3] = 0;
      config.Gateway[0] = 192;config.Gateway[1] = 168;config.Gateway[2] = 1;config.Gateway[3] = 1;
      config.ntpServerName = "pool.ntp.org";
      config.Update_Time_Via_NTP_Every =  0;
      config.timezone = -70;
      config.daylight = true;
      config.DeviceName = "Garage Heater";
      config.AutoTurnOff = false;
      config.AutoTurnOn = false;
      config.TurnOffHour = 0;
      config.TurnOffMinute = 0;
      config.TurnOnHour = 0;
      config.TurnOnMinute = 0;
      config.HeatSP = 50;
      config.HeatHist = 4;
      config.HeatStatus = true;
      config.AdminEnabled = false;
      
      WriteConfig();

      digitalWrite(BUILTIN_LED, LOW);
      // reboot
      delay(3000);
      // cause a reset
      pinMode(16, OUTPUT);
    }
  }
  else
  {
    if (GPIO0Pushed)
    {
      Serial.println("GPIO0 Released");
    }
    GPIO0Pushed = false;
  }

  if ((millis() - time_read) > TEMP_READ)
  {
    // read a new start time
    time_read = millis();
    
    // the specified time to read has elapsed
    // read the temp sensor
    uint16_t val = analogRead(A0);
    yield();
    delay(2);
    Temp_Hist[TEMP_ITER] = Thermister(val) * 9 / 5 + 32;

    // increment iterator
    TEMP_ITER++;

    // calculate new average temp
    int i;
    double sum = 0;
    for (i = 0; i < AVG_SIZE; i++)
      sum += Temp_Hist[i];

    Temp_Avg = sum / AVG_SIZE;

    if (TEMP_ITER >= AVG_SIZE)
    {
      // reset
      TEMP_ITER = 0; 
      logicReady = true;       
    }
  }

  if (((millis() - time_logic) > LOGIC_CHANGE) && logicReady)
  {
    time_logic = millis();

    if (config.HeatStatus)
    {
      Serial.printf("FreeMem : %d\r\n",ESP.getFreeHeap());
      
      if (Temp_Avg > (config.HeatSP + config.HeatHist))
      {
        if (config.RelayState)
          Serial.println("Heat cycle complete");
        
        // turn relay off
        config.RelayState = false;
      }
      else if(Temp_Avg < config.HeatSP)
      {
        if (!config.RelayState)
          Serial.println("Heat cycle started");
        
        // turn relay on
        config.RelayState = true;
      }
      // do nothing
    }
    else
    {
      config.RelayState = false;
    }
  }

  if (config.RelayState)
    pinMode(relayPin, OUTPUT);
  else
    pinMode(relayPin, INPUT);

  delay(10);
}

double Thermister(int val) 
{
  double V_NTC = (double)val / 1024;
  double R_NTC = (Rs * V_NTC) / (Vcc - V_NTC);
  R_NTC = log(R_NTC);
  double Temp = 1 / (0.001129148 + (0.000234125 + (0.0000000876741 * R_NTC * R_NTC ))* R_NTC );
  Temp = Temp - 273.15;         
  return Temp;
}

void toggleHeaterState()
{ 
  if (config.HeatStatus == true)
    config.HeatStatus = false;
  else
    config.HeatStatus = true;

  Serial.println("Heater is now " + (String)(config.HeatStatus ? "On" : "Off"));
  
  WriteConfig();
  
  String values ="";
  values += "temp|" +   (String) (int8_t)Temp_Avg +  "|label\n";
  values += "sp|" +  (String)  config.HeatSP +  "|label\n";
  values += "status|" +  (String)  (config.RelayState ? "On" : "Off") +  "|label\n";
  values += "state|" + (String) (config.HeatStatus ? "checked" : "") + "|chk\n";
  server.send ( 200, "text/plain", values);
  Serial.println(__FUNCTION__);
}
