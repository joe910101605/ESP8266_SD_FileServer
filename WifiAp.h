#ifndef _WIFI_AP_H
#define _WIFI

#include <ESP8266WiFi.h>
#include "Constants.h"

static String getIP(){
  IPAddress IP = WiFi.softAPIP();
  return IP.toString();
}

static bool initWifi(){
  Serial.println("Initializing Wifi...");
  if(!WiFi.softAP(WIFI_SSID, WIFI_PASSWD))
    return false;

  // Print ESP8266 Local IP Address
  Serial.println(WiFi.localIP());

  return true;
}

#endif
