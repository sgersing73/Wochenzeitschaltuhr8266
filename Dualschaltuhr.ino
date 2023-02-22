
// ****************************************************************
// Sketch Esp8266 Zeitschaltuhr Dual Modular(Tab)
// created: Jens Fleischer, 2019-03-08
// last mod: Jens Fleischer, 2020-04-25
// For more information visit: https://fipsok.de
// ****************************************************************
// Hardware: Esp8266, Relais Modul o. Mosfet IRF3708 o. Fotek SSR-40 DA
// für Relais Modul
// GND an GND
// IN1 an D5 = GPIO14
// IN2 an D6 = GPIO12
// VCC an VIN -> je nach verwendeten Esp.. möglich
// Jumper JD-VCC VCC
// alternativ ext. 5V Netzteil verwenden
//
// für Mosfet IRF3708
// Source an GND
// Mosfet1 Gate an D5 = GPIO14
// Mosfet2 Gate an D6 = GPIO12
//
// für 3V Solid State Relais
// GND an GND
// SSR1 Input + an D5 = GPIO14
// SSR2 Input + an D6 = GPIO12
//
// Software: Esp8266 Arduino Core 2.4.2 / 2.5.2 / 2.6.3 /2.7.4
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
// Diese Version von Wochenzeitschaltuhr sollte als Tab eingebunden werden.
// #include <FS.h> #include <ESP8266WebServer.h> müssen im Haupttab aufgerufen werden
// Die Funktionalität des ESP8266 Webservers ist erforderlich.
// Der Lokalzeit Tab ist zum ausführen der Zeitschaltuhr einzubinden.
// Die Funktion "setupTimerSwitch();" muss im Setup aufgerufen werden.
// Zum schalten muss die Funktion "dualTimerSwitch();" im loop(); aufgerufen werden.
/**************************************************************************************/

const auto aktiv = LOW;                    // LOW für LOW aktive Relais oder HIGH für HIGH aktive (zB. SSR, Mosfet) einstellen
const uint8_t relPin[] = {D5, D6, D7, D8}; // Pin für Relais einstellen
const auto count = 20;                     // Anzahl Schaltzeiten (analog Html Dokument) einstellen 2 bis 40
bool relState[] {!aktiv, !aktiv, !aktiv, !aktiv};

struct Collection {
  byte switchActive[count];
  byte wday[count];
  char switchTime[count * 2][6];
};

Collection times;

void setupTimerSwitch() {
  for (auto& pin : relPin) digitalWrite(pin, !aktiv), pinMode(pin, OUTPUT);
  File file = SPIFFS.open("/stimes.dat", "r");
  if (file && file.size() == sizeof(times)) {                          // Einlesen aller Daten falls die Datei im Spiffs vorhanden und deren Größe stimmt.
    file.read(reinterpret_cast<byte*>(&times), sizeof(times));         // Deserialisierung
    file.close();
  } else {                                                             // Sollte die Datei nicht existieren
    for (auto i = 0; i < count; i++) {
      times.switchActive[i] = 1;                                       // werden alle Schaltzeiten
      times.wday[i] = ~times.wday[i];                                  // und alle Wochentage aktiviert.
    }
  }
  server.on("/timer", HTTP_POST, []() {
    if (server.args() == 1) {
      times.switchActive[server.argName(0).toInt()] = server.arg(0).toInt();
      toSave();
      String temp = "\"";
      for (auto& elem : times.switchActive) {
        temp += elem;
      }
      temp += "\"";
      server.send(200, "application/json", temp);
    }
    if (server.hasArg("sTime")) {
      byte i {0};
      char str[count * 14];
      strcpy (str, server.arg("sTime").c_str());
      char* ptr = strtok(str, ",");
      while (ptr != NULL) {
        strcpy (times.switchTime[i++], ptr);
        ptr = strtok(NULL, ",");
      }
      if (server.arg("sDay")) {
        strcpy (str, server.arg("sDay").c_str());
        ptr = strtok(str, ",");
        i = 0;
        while (ptr != NULL) {
          times.wday[i++] = atoi(ptr);
          ptr = strtok(NULL, ",");
        }
        toSave();
      }
      else {
        server.send(400, "");
      }
    }
    String temp = "[";
    for (auto& elem : times.switchTime) {
      if (temp != "[") temp += ',';
      temp += "\"" + static_cast<String>(elem) + "\"";
    }
    temp += ",\"";
    for (auto& elem : times.switchActive) {
      temp += elem;
    }
    for (auto& elem : times.wday) {
      temp += "\",\"";
      temp += elem;
    }
    temp += "\"]";
    server.send(200, "application/json", temp);
  });
  server.on("/timer", HTTP_GET, []() {
    if (server.hasArg("tog") && server.arg(0) == "0") {
      relState[0] = !relState[0];                                 // Relais1 Status manuell ändern
      timeDataLogger(0, server.client().remoteIP().toString());   // Funktionsaufruf Zeitdatenlogger
    }
    if (server.hasArg("tog") && server.arg(0) == "1") {
      relState[1] = !relState[1];                                 // Relais2 Status manuell ändern
      timeDataLogger(1, server.client().remoteIP().toString());   // Funktionsaufruf Zeitdatenlogger
    }
    if (server.hasArg("tog") && server.arg(0) == "2") {
      relState[2] = !relState[2];                                 // Relais3 Status manuell ändern
      timeDataLogger(1, server.client().remoteIP().toString());   // Funktionsaufruf Zeitdatenlogger
    }
    if (server.hasArg("tog") && server.arg(0) == "3") {
      relState[3] = !relState[3];                                 // Relais4 Status manuell ändern
      timeDataLogger(1, server.client().remoteIP().toString());   // Funktionsaufruf Zeitdatenlogger
    }
    server.send(200, "application/json", ("[\"" + static_cast<String>(relState[0] == aktiv) + "\",\"" + 
                                                  static_cast<String>(relState[1] == aktiv) + "\",\"" + 
                                                  static_cast<String>(relState[2] == aktiv) + "\",\"" + 
                                                  static_cast<String>(relState[3] == aktiv) + "\",\"" + 
                                                  localTime() + "\"]"));
  });
}

void toSave() {
  File file = SPIFFS.open("/stimes.dat", "w");
  if (file) {
    file.write(reinterpret_cast<byte*>(&times), sizeof(times));   // Serialisierung
    file.close();
  }
}

void dualTimerSwitch() {
  static uint8_t lastmin {60}, lastState[] {aktiv, aktiv, aktiv, aktiv};
  hobbsMeter(relState[0], relState[1], relState[2], relState[3]);              // Funktionsaufruf Betriebsstundenzähler mit Relais Status
  if (tm.tm_min != lastmin) {
    lastmin = tm.tm_min;
    char buf[6];
    snprintf(buf, sizeof(buf), "%.2d:%.2d", tm.tm_hour, tm.tm_min);
    for (auto i = 0; i < count * 2; i++) {
      if (times.switchActive[i / 2] && !strcmp(times.switchTime[i], buf)) {
        if (times.wday[i / 2] & (1 << (tm.tm_wday ? tm.tm_wday - 1 : 6))) {
          // i < (count % 2 ? count + 1 : count) ? ({relState[0] = i % 2 ? !aktiv : aktiv; timeDataLogger(0, "Programm");}) : ({relState[1] = i % 2 ? !aktiv : aktiv; timeDataLogger(1, "Programm");});  // Relais Status nach Zeit ändern          
          if ( i >= 0  &&  i <= 9 ) {
            relState[0] = i % 2 ? !aktiv : aktiv; timeDataLogger(0, "Programm");
          }          
          if ( i >= 10  &&  i <= 19 ) {
            relState[1] = i % 2 ? !aktiv : aktiv; timeDataLogger(1, "Programm");
          }
          if ( i >= 20  &&  i <= 29 ) {
            relState[2] = i % 2 ? !aktiv : aktiv; timeDataLogger(2, "Programm");
          }
          if ( i >= 30  &&  i <= 39 ) {
            relState[3] = i % 2 ? !aktiv : aktiv; timeDataLogger(3, "Programm");
          }
        }
      }
    }
  }
  if (relState[0] != lastState[0] || 
      relState[1] != lastState[1] || 
      relState[2] != lastState[2] || 
      relState[3] != lastState[3]
     ) {    // Relais schalten wenn sich der Status geändert hat
    for (auto i = 0; i < 4; i++) {
      lastState[i] = relState[i];
      digitalWrite(relPin[i], relState[i]);
      Serial.printf("Relais %d %s\n", 1 + i,  digitalRead(relPin[i]) == aktiv ? "an" : "aus");
    }
  }
}

uint8_t getRelaisState(uint8_t iRel) {
  return !relState[iRel];
}
