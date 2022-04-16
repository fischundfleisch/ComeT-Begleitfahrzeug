#include "time.h"
#include <WiFi.h>
#include <WebServer.h>
#include "FS.h"
#include "SPIFFS.h"

#define FORMAT_SPIFFS_IF_FAILED true

const char* SSID_CART = "ComeT Begleitfahrzeug";
const char* PASSWORD_CART = "123456789";

String starttime_ = "test";
String endtime_ = "test";
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
String file_read = "";

IPAddress local_ip(192, 168, 4, 1);
IPAddress gateway(192, 168, 4, 1);
IPAddress subnet(255, 255, 255, 0);
WebServer server(80);


void setup() {
  Serial.begin(115200);
  if (!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }
  readFile(SPIFFS, "/routen.txt");
  connect_ap();

  pinMode(PIN_REED, INPUT);
  digitalWrite(PIN_REED, HIGH);
  
}

void connect_ap() {
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
  html += file_read;
  html += "</body></html>";
  return html;
}

void handle_root() {
  server.send(200, "text/html", create_html_header() );
}

void handle_starttime() {

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

void handle_save() {
  String fileSave = starttime_ + "," + endtime_ + "," + distance_ + "\n\r";
  appendFile(SPIFFS, "/test.txt", fileSave.c_str());

  button_start = "";
  button_end = "";
  button_save = " disabled";
  server.send(200, "text/html", create_html_header());
}

void handle_connect() {
  content = "<!DOCTYPE HTML>\r\n<html>Einstellungen";
  content += "<form method =\"get\" action =\"setting\"><input type=\"submit\">";
  content += "<input type=\"time\" name=\"timestamp\" step=\"1\"></form>";
  content += "</html>";
  server.send(200, "text/html", content);
}

void readFile(fs::FS & fs, const char * path) {
  Serial.printf("Reading file: %s\n", path);

  File file = fs.open("/routen.txt", FILE_READ);
  if (!file) {
    Serial.println("Failed to open file for reading");
    return;
  }

  Serial.print("Read from file: ");
  while (file.available()) {
    file_read += file.read();
    Serial.println(file_read);
  }
  file.close();
}

void handle_setting() {
  timestamp_ = server.arg("timestamp");

  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/html", create_html_header());
}


void appendFile(fs::FS & fs, const char * path, const char * message) {
  Serial.printf("Appending to file: %s\n", path);

  File file = fs.open("/test.txt", FILE_APPEND);
  if (!file) {
    Serial.println("Failed to open file for appending");
    return;
  }
  if (file.print(message)) {
    Serial.println(message);
    Serial.println("Message appended");
  } else {
    Serial.println("Append failed");
  }
  file.close();
}
