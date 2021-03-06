#include "time.h"
#include <WiFi.h>
#include <WebServer.h>
#include "SD_MMC.h"

const char* SSID_CART = "ComeT Begleitfahrzeug";
const char* PASSWORD_CART = "123456789";

String starttime_ = "test";
String endtime_ = "";
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
String content = "";

String timestamp_;

IPAddress local_ip(192, 168, 4, 1);
IPAddress gateway(192, 168, 4, 1);
IPAddress subnet(255, 255, 255, 0);
WebServer server(80);


void setup() {
  Serial.begin(115200);
  
connect_ap();
  
  pinMode(PIN_REED, INPUT);
  digitalWrite(PIN_REED, HIGH);
}

void connect_ap(){
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
}


void loop() {
//    ArduinoOTA.handle();
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
  html += timestamp_;
  html += "</body></html>";
  return html;
}

void handle_root() {
  server.send(200, "text/html", create_html_header() );
}

void handle_starttime() {               // wird so den User jedes Mal beim connect_time_server rausschmeißen

  //    connect_soft_ap();
  button_start = " disabled";
  button_end = "";
  button_save = "";
  server.send(200, "text/html", create_html_header());
}

void handle_endtime() {

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
  content += "<form method =\"get\" action =\"setting\"><input type=\"submit\">";
  content += "<input type=\"time\" name=\"timestamp\" step=\"1\"></form>";
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

  timestamp_ = server.arg("timestamp");
  
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
