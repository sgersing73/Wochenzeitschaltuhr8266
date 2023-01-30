// ****************************************************************
// Sketch Esp8266 Dateiverwaltung spezifisch Sortiert Modular(Tab)
// created: Jens Fleischer, 2020-08-03
// last mod: Jens Fleischer, 2020-08-03
// For more information visit: https://fipsok.de
// ****************************************************************
// Hardware: Esp8266
// Software: Esp8266 Arduino Core 2.4.2 - 2.7.4
// Geprüft: von 1MB bis 16MB Flash
// Getestet auf: Nodemcu, Wemos D1 Mini Pro, Sonoff Switch, Sonoff Dual
/******************************************************************
  Copyright (c) 2020 Jens Fleischer. All rights reserved.

  This file is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  This file is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.
*******************************************************************/
// Diese Version von Spiffs sollte als Tab eingebunden werden.
// #include <FS.h> #include <ESP8266WebServer.h> müssen im Haupttab aufgerufen werden
// Die Funktionalität des ESP8266 Webservers ist erforderlich.
// "server.onNotFound()" darf nicht im Setup des ESP8266 Webserver stehen.
// Die Funktion "spiffs();" muss im Setup aufgerufen werden.
/**************************************************************************************/

#include <list>

const char WARNING[] PROGMEM = R"(<h2>Der Sketch wurde mit "no SPIFFS" kompilliert!)";
const char HELPER[] PROGMEM = R"(<form method="POST" action="/upload" enctype="multipart/form-data">
<input type="file" name="[]" multiple><button>Upload</button></form>Lade die spiffs.html hoch.)";

void spiffs() {     // Funktionsaufruf "spiffs();" muss im Setup eingebunden werden
  SPIFFS.begin();
  server.on("/list", handleList);
  server.on("/format", formatSpiffs);
  server.on("/upload", HTTP_POST, sendResponce, handleUpload);
  server.onNotFound([]() {
    if (!handleFile(server.urlDecode(server.uri())))
      server.send(404, "text/plain", "FileNotFound");
  });
}

void handleList() {                           // Senden aller Daten an den Client
  FSInfo fs_info;  SPIFFS.info(fs_info);      // Füllt FSInfo Struktur mit Informationen über das Dateisystem
  Dir dir = SPIFFS.openDir("/");              // Auflistung aller im Spiffs vorhandenen Dateien
  typedef std::pair<String, int> prop;
  std::list<prop> dirList;                                                               // Liste anlegen
  while (dir.next()) dirList.emplace_back(dir.fileName().substring(1), dir.fileSize());  // Liste füllen
  dirList.sort([](const prop & f, const prop & l) {                                      // Liste sortieren
    if (server.arg(0) == "1") {
      return f.second > l.second;
    } else {
      for (uint8_t i = 0; i < 30; i++) {
        if (tolower(f.first[i]) < tolower(l.first[i])) return true;
        else if (tolower(f.first[i]) > tolower(l.first[i])) return false;
      }
      return false;
    }
  });
  String temp = "[";
  for (auto& p : dirList) {
    if (temp != "[") temp += ',';
    temp += "{\"name\":\"" + p.first + "\",\"size\":\"" + formatBytes(p.second) + "\"}";
  }
  temp += ",{\"usedBytes\":\"" + formatBytes(fs_info.usedBytes * 1.05) + "\"," +             // Berechnet den verwendeten Speicherplatz + 5% Sicherheitsaufschlag
          "\"totalBytes\":\"" + formatBytes(fs_info.totalBytes) + "\",\"freeBytes\":\"" +    // Zeigt die Größe des Speichers
          (fs_info.totalBytes - (fs_info.usedBytes * 1.05)) + "\"}]";                        // Berechnet den freien Speicherplatz + 5% Sicherheitsaufschlag
  server.send(200, "application/json", temp);
}

bool handleFile(String&& path) {
  if (server.hasArg("delete")) {
    SPIFFS.remove(server.arg("delete"));                   // Datei löschen
    sendResponce();
    return true;
  }
  if (!SPIFFS.exists("/spiffs.html"))server.send(200, "text/html", SPIFFS.begin() ? HELPER : WARNING);     // ermöglicht das hochladen der spiffs.html
  if (path.endsWith("/")) path += "index.html";
  return SPIFFS.exists(path) ? ({File f = SPIFFS.open(path, "r"); server.streamFile(f, getContentType(path)); f.close(); true;}) : false;
}

void handleUpload() {                                      // Dateien ins SPIFFS schreiben
  static File fsUploadFile;                                // enthält den aktuellen Upload
  HTTPUpload& upload = server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    if (upload.filename.length() > 30) {
      upload.filename = upload.filename.substring(upload.filename.length() - 30, upload.filename.length());  // Dateinamen auf 30 Zeichen kürzen
    }
    printf("handleFileUpload Name: /%s\n", upload.filename.c_str());
    fsUploadFile = SPIFFS.open("/" + server.urlDecode(upload.filename), "w");
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    printf("handleFileUpload Data: %u\n", upload.currentSize);
    fsUploadFile.write(upload.buf, upload.currentSize);
  } else if (upload.status == UPLOAD_FILE_END) {
    printf("handleFileUpload Size: %u\n", upload.totalSize);
    fsUploadFile.close();
  }
}

void formatSpiffs() {                                      // Formatiert den Speicher
  SPIFFS.format();
  sendResponce();
}

void sendResponce() {
  server.sendHeader("Location", "spiffs.html");
  server.send(303, "message/http");
}

const String formatBytes(size_t const& bytes) {            // lesbare Anzeige der Speichergrößen
  return bytes < 1024 ? static_cast<String>(bytes) + " Byte" : bytes < 1048576 ? static_cast<String>(bytes / 1024.0) + " KB" : static_cast<String>(bytes / 1048576.0) + " MB";
}

const String &getContentType(String& filename) {           // ermittelt den Content-Typ
  if (filename.endsWith(".htm") || filename.endsWith(".html")) filename = "text/html";
  else if (filename.endsWith(".css")) filename = "text/css";
  else if (filename.endsWith(".js")) filename = "application/javascript";
  else if (filename.endsWith(".json")) filename = "application/json";
  else if (filename.endsWith(".png")) filename = "image/png";
  else if (filename.endsWith(".gif")) filename = "image/gif";
  else if (filename.endsWith(".jpg")) filename = "image/jpeg";
  else if (filename.endsWith(".ico")) filename = "image/x-icon";
  else if (filename.endsWith(".xml")) filename = "text/xml";
  else if (filename.endsWith(".pdf")) filename = "application/x-pdf";
  else if (filename.endsWith(".zip")) filename = "application/x-zip";
  else if (filename.endsWith(".gz")) filename = "application/x-gzip";
  else filename = "text/plain";
  return filename;
}

bool freeSpace(uint16_t const& printsize) {                // Funktion um beim speichern in Logdateien zu prüfen ob noch genügend freier Platz verfügbar ist.
  FSInfo fs_info;   SPIFFS.info(fs_info);                  // Füllt FSInfo Struktur mit Informationen über das Dateisystem
  //Serial.printf("Funktion: %s meldet in Zeile: %d FreeSpace: %s\n", __PRETTY_FUNCTION__, __LINE__, formatBytes(fs_info.totalBytes - (fs_info.usedBytes * 1.05)).c_str());
  return (fs_info.totalBytes - (fs_info.usedBytes * 1.05) > printsize) ? true : false;
}
