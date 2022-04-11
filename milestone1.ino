#include "time.h"
#include <WiFi.h>
#include <WebServer.h>
#include "FS.h"
#include "SD_MMC.h"
#include "EEPROM.h"
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

const char* SSID_CART = "ComeT Begleitfahrzeug";
const char* PASSWORD_CART = "123456789";
const char* NTPSERVER = "pool.ntp.org";
const long GMTOFFSET_SEC = 3600;
const int DAYLIGHTOFFSET_SEC = 3600;
String starttime_ = "test";
String endtime_ = "";
char time_buff_[20];
int counter_ = 0;
const int PIN_REED = 12;
unsigned int wheel_rotation_ = 0;
float distance_ = 0;
bool rotation_check_ = false;
bool rotation_last_ = false;
String button_start = "";       //By default the button is enabled. We will disable it at certain points.
String button_end = "";
String button_save = "";
// für das Speichern der SSID und dem Einlesen des EEPROM:
int i = 0;
int status_code_ = 0;
String st = "";
String content = "";
String esid = "";
String epass = "";

IPAddress local_ip(192, 168, 4, 1);
IPAddress gateway(192, 168, 4, 1);
IPAddress subnet(255, 255, 255, 0);
int wifi_status_ = WL_IDLE_STATUS;
WebServer server(80);


void setup() {
  Serial.begin(115200);
  EEPROM.begin(512);
  // Lese aus dem EEPROM die Zugangsdaten
  for (int i = 0; i < 32; ++i) {
    esid += char(EEPROM.read(i));
  }
  for (int i = 32; i < 96; ++i) {
    epass += char(EEPROM.read(i));
  }
connect_sta_and_ap();
 ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Serial.println("Start updating " + type);
    })
    .onEnd([]() {
      Serial.println("\nEnd");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });

  ArduinoOTA.begin();

  
  pinMode(PIN_REED, INPUT);
  digitalWrite(PIN_REED, HIGH);
}

void connect_sta_and_ap() {

  WiFi.mode(WIFI_MODE_APSTA);

  WiFi.softAP(SSID_CART, PASSWORD_CART);
  WiFi.softAPConfig(local_ip, gateway, subnet);
  Serial.println("Wifi SoftAP");
  Serial.println(SSID_CART);
  server.on("/", handle_root);
  server.on("/starttime", handle_starttime);
  server.on("/endtime", handle_endtime);
  server.on("/save", handle_save);
  server.on("/connect", handle_connect);
  server.on("/setting", handle_setting);
  server.begin();

  WiFi.begin(esid.c_str(), epass.c_str());

  for (int x=0; x<15; x++) {
    if (WiFi.status() == WL_CONNECTED) {
      break;
    }
    delay(500);
    Serial.println(".");
  }

}


void connect_time_server() {
  configTime(GMTOFFSET_SEC, DAYLIGHTOFFSET_SEC, NTPSERVER);
  struct tm timeinfo;
  if (getLocalTime(&timeinfo) == false) {
    Serial.println("Fehler");
    return;
  }
  Serial.println(&timeinfo, "%d. %B %Y %H:%M:%S");
  strftime(time_buff_, 20, "%d.%m.%Y %H:%M:%S", &timeinfo);
}

void loop() {
    ArduinoOTA.handle();
  rotation_check_ = digitalRead(PIN_REED);
  if (rotation_check_ == rotation_last_ ) {
    //nichts hat sich geändert, tu auch nichts
  }
  else if (rotation_check_ == 0) {
    rotation_last_ = rotation_check_;
  }
  else {
    counter_++;
    distance_ = counter_ * 84 / 100;
    rotation_last_ = rotation_check_;
    wheel_rotation_ = counter_;
  }

  server.handleClient();

}

String create_html_header() {
  String html = "<!DOCTYPE html>";
  html += "<html><head><title>ComeT Begleitfahrzeug</title>";
  html += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"> </head><body><a href=\"/\">";
  html += "<h1>ComeT Begleitfahrzeug</h1></a>";
  html += "<br> <br> <br>";
  html += "<table> <tbody> <tr>";
  html += "<td><strong>Startzeit</strong></td>";
  html += "<td><strong>Endzeit</strong></td>";
  html += "<td><strong>Umdrehungen</strong></td>";
  html += "<td><strong>Entfernung</strong></td>";
  html += "</tr><tr>";
  html += "<td>";
  html += starttime_;
  html += "</td><td>";
  html += endtime_;
  html += "</td><td>";
  html += wheel_rotation_;
  html += "</td><td>";
  html += distance_;
  html += "</td></tr></tbody></table>";

  html += "<br> <br>";
  html += "<a href=\"/starttime\">";
  html += "<button";
  html += button_start;
  html += ">Start Time</button></a>";
  html += "<a href=\"/endtime\">";
  html += "<button";
  html += button_end;
  html += ">End Time</button></a>";
  html += "<a href=\"/save\">";
  html += "<button";
  html += button_save;
  html += ">Speichern</button></a>";
  html += "<br> <br> <a href = \"/connect\">Zugangsdaten eingeben</a>";
  html += "</body></html>";
  return html;
}

void handle_root() {
  server.send(200, "text/html", create_html_header() );
}

void handle_starttime() {               // wird so den User jedes Mal beim connect_time_server rausschmeißen
  connect_time_server();
  starttime_ = String(time_buff_);
  //    connect_soft_ap();
  button_start = " disabled";
  button_end = "";
  button_save = "";
  server.send(200, "text/html", create_html_header());
}

void handle_endtime() {
  connect_time_server();
  endtime_ = String(time_buff_);
  //connect_soft_ap();
  button_start = "";
  button_end = " disabled";
  button_save = "";
  server.send(200, "text/html", create_html_header());
}

void handle_save() {         // ACHTUNG: nur 8.3-Dateinamen: 8 Zeichen Dateiname, 3 Zeichen extention. docx fällt aus ;)
  //hier die SD Karte beschreiben
  if (!SD_MMC.begin()) {
    Serial.println("konnte Karte nicht laden");
    return;
  }
  uint8_t cardType = SD_MMC.cardType();

  if (cardType == CARD_NONE) {
    Serial.println("Keine Karte eingelegt");
    return;
  }
  // hier String zum schreiben aufbereiten:
  String fileSave = starttime_ + "," + endtime_ + "," + distance_ + "\n";       //hier endterminieren? Nötig?
  appendFile(SD_MMC, "/routen.txt", fileSave.c_str());

  button_start = "";
  button_end = "";
  button_save = " disabled";
  server.send(200, "text/html", create_html_header());
}

void handle_connect() {
  content = "<!DOCTYPE HTML>\r\n<html>Bitte Zugangsdaten eingeben";
  content += "<form method =\"get\" action =\"setting\"><label>SSID: </label><input name=\"ssid\" lenght=32><input name=\"pass\" lenght=64><input type=\"submit\"></form>";
  content += "</html>";
  server.send(200, "text/html", content);
}

void readFile(fs::FS & fs, const char * path) {
  Serial.printf("Reading file: %s\n", path);

  File file = fs.open(path);
  if (!file) {
    Serial.println("Failed to open file for reading");
    return;
  }

  Serial.print("Read from file: ");
  while (file.available()) {
    Serial.write(file.read());
  }
}

void handle_setting() {
  String qsid = server.arg("ssid");
  String qpass = server.arg("pass");
  if (qsid.length() > 0 && qpass.length() > 0) {
    for (int i = 0; i < 96; i++) {
      EEPROM.write(i, 0);
    }

    for (int i = 0; i < qsid.length(); i++) {
      EEPROM.write(i, qsid[i]);
    }
    for (int i = 0; i < qpass.length(); i++) {
      EEPROM.write(i + 32, qpass[i]);
    }
    EEPROM.commit();

    content = "{\"Success\":\"saved to eeprom...reset to boot into new wifi\"}";
    status_code_ = 200;
    //eventuell: ESP.restart();
  }
  else {
    content = "{\"Error\":\"404 not found\}";
    status_code_ = 404;
  }
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(status_code_, "application/json", content);
}


void appendFile(fs::FS & fs, const char * path, const char * message) {
  Serial.printf("Appending to file: %s\n", path);

  File file = fs.open(path, FILE_APPEND);
  if (!file) {
    Serial.println("Failed to open file for appending");
    return;
  }
  if (file.print(message)) {
    Serial.println("Message appended");
  } else {
    Serial.println("Append failed");
  }
}
