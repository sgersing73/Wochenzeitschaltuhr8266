// ****************************************************************
// Sketch Esp8266 Lokalzeit Modular(Tab)
// created: Jens Fleischer, 2018-07-10
// last mod: Jens Fleischer, 2021-08-15
// For more information visit: https://fipsok.de
// ****************************************************************
// Hardware: Esp8266
// Software: Esp8266 Arduino Core 2.4.2 - 3.0.2
// Getestet auf: Nodemcu, Wemos D1 Mini Pro, Sonoff Switch, Sonoff Dual
/******************************************************************
  Copyright (c) 2018 Jens Fleischer. All rights reserved.

  This file is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  This file is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.
*******************************************************************/
// Diese Version von Lokalzeit sollte als Tab eingebunden werden.
// #include <ESP8266WebServer.h> oder #include <ESP8266WiFi.h> muss im Haupttab aufgerufen werden.
// Funktion "setupTime();" muss im setup() nach dem Verbindungsaufbau aufgerufen werden.
/**************************************************************************************/

#include <sntp.h>

const uint32_t SYNC_INTERVAL = 12;                             // NTP Sync Interval in Stunden einstellen

const char* const PROGMEM NTP_SERVER[] = {"fritz.box", "de.pool.ntp.org", "at.pool.ntp.org", "ch.pool.ntp.org", "ptbtime1.ptb.de", "europe.pool.ntp.org"};
const char* const PROGMEM MONTH_NAMES[] = {"Januar", "Februar", "März", "April", "Mai", "Juni", "Juli", "August", "September", "Oktober", "November", "Dezember"};
const char* const PROGMEM MONTH_SHORT[] = {"Jan", "Feb", "Mrz", "Apr", "Mai", "Jun", "Jul", "Aug", "Sep", "Okt", "Nov", "Dez"};

void setNtp(const char* server1, const char* server2 = nullptr, const char* server3 = nullptr);

void setupTime() {
  setNtp(NTP_SERVER[1], NTP_SERVER[2], NTP_SERVER[3]);         // deinen NTP Server einstellen (von 0 - 5 aus obiger Liste)
  server.on("/time", []() {
    server.send(200, "application/json", localTime());
  });
}

void setNtp(const char* server1, const char* server2, const char* server3) {
  sntp_stop();
  sntp_setservername(0, (char*)server1);
  if (server2) sntp_setservername(1, (char*)server2);
  if (server3) sntp_setservername(2, (char*)server3);
  static uint8_t once {0};
  if (!once++) {
    sntp_set_timezone(0);
    setenv("TZ", "CET-1CEST,M3.5.0/02,M10.5.0/03", 1);         // Zeitzone einstellen https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv
  }
  sntp_init();
}

char* localTime() {
  static char buf[9];                                          // je nach Format von "strftime" eventuell anpassen
  static time_t lastsec;
  time_t now = time(&now);
  localtime_r(&now, &tm);
  if (tm.tm_sec != lastsec) {
    lastsec = tm.tm_sec;
    strftime(buf, sizeof(buf), "%T", &tm);                     // http://www.cplusplus.com/reference/ctime/strftime/
    if (!(time(&now) % (SYNC_INTERVAL * 3600))) {
      setupTime();
    }
  }
  return buf;
}
