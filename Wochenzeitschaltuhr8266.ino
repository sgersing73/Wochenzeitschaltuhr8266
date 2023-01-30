// ****************************************************************
// Sketch Esp8266 Wochenzeitschaltuhrdual Modular(Tab)
// created: Jens Fleischer, 2019-03-08
// last mod: Jens Fleischer, 2021-08-28
// For more information visit: https://fipsok.de
// ****************************************************************
// Hardware: Esp8266
// Software: Esp8266 Arduino Core 2.4.2 / 2.5.2 / 2.6.3 / 2.7.4
// Getestet auf: Nodemcu, Wemos D1 Mini Pro, Sonoff Dual
/******************************************************************
  Copyright (c) 2019 Jens Fleischer. All rights reserved.

  This file is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  This file is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.
*******************************************************************/
// Der WebServer Tab ist der Haupt Tab mit "setup" und "loop".
// #include <FS.h> #include <ESP8266WebServer.h> müssen im Haupttab aufgerufen werden
// Die Funktionalität des ESP8266 Webservers ist erforderlich.
// "server.onNotFound()" darf nicht im Setup des ESP8266 Webserver stehen.
// Inklusive Arduino OTA-Updates (Erfordert 512K Spiffs)
/**************************************************************************************/

#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

#include <ESP8266WebServer.h>
#include <ArduinoOTA.h>     // https://arduino-esp8266.readthedocs.io/en/latest/ota_updates/readme.html
#include <FS.h>
#include <time.h>

struct tm tm;

char file[sizeof(__FILE__)] = __FILE__; // Dateiname für den Admin Tab

ESP8266WebServer server(80);

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.printf("\nSketchname: %s\nBuild: %s\t\tIDE: %d.%d.%d\n%s\n\n",
                (__FILE__), (__TIMESTAMP__), ARDUINO / 10000, ARDUINO % 10000 / 100, ARDUINO % 10, ESP.getFullVersion().c_str());
  spiffs();
  Connect();
  admin();
  setupTime();
  setupTimerSwitch();
  setupHobbsMeter();
  ArduinoOTA.onStart([]() {
    save();                  // vor dem OTA Sketch Update Betriebsstunden in Datei schreiben
  });
  ArduinoOTA.begin();
  server.begin();
}

void loop() {
  ArduinoOTA.handle();
  server.handleClient();
  if (millis() < 0x2FFF || millis() > 0xFFFFF0FF) runtime();
  static uint32_t previousMillis {0};
  uint32_t currentMillis {millis()};
  if (currentMillis - previousMillis >= 1e2) {
    previousMillis = currentMillis;
    localTime();
    dualTimerSwitch();
  }
}
