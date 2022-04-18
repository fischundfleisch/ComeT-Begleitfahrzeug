#include "time.h"
#include <WiFi.h>
#include <WebServer.h>
#include "FS.h"
#include "SPIFFS.h"

//#define FORMAT_SPIFFS_IF_FAILED true FINGER WEG!!!!!

const char* SSID_CART = "ComeT Begleitfahrzeug Marlene";
const char* PASSWORD_CART = "123456789";

String starttime_ = "test";
String endtime_ = "test";

int counter_ = 0;
const int PIN_REED = 14;
unsigned int wheel_rotation_ = 0;
float distance_ = 0;
bool rotation_check_ = false;
bool rotation_last_ = false;
String button_save = "";
int i = 0;
int status_code_ = 0;
String content = "";
String timestamp_;
String file_read_ = "";

IPAddress local_ip(192, 168, 4, 2);
IPAddress gateway(192, 168, 4, 2);
IPAddress subnet(255, 255, 255, 0);
WebServer server(80);

void setup() {
//  bool formatted = SPIFFS.format();                     FINGER WEG! Braucht man zum Formatieren von SPIFFS
//    if(formatted){
//      Serial.println("\n\nSuccess formatting");
//   }else{
//      Serial.println("\n\nError formatting");
//   }
  Serial.begin(115200);
  if (!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }
  connect_ap();

  pinMode(PIN_REED, INPUT);
  digitalWrite(PIN_REED, HIGH);
  readFile(SPIFFS, "/routen.txt");
}

void connect_ap() {
  WiFi.softAP(SSID_CART, PASSWORD_CART);
  WiFi.softAPConfig(local_ip, gateway, subnet);
  Serial.println("Wifi SoftAP");
  Serial.println(SSID_CART);
  server.on("/", handle_root);
  server.on("/save", handle_save);
  server.on("/submit", handle_root);
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
  html += "<a href=\"/save\">";
  html += "<button";
  html += button_save;
  html += ">Speichern</button></a>";
  html += file_read_;
  html += "<form action =\"/submit\">";
  html += "<input type=\"datetime-local\" name=\"starttime\">";
  html += "<input type=\"submit\" value = \"Startzeit\">";
  html += "<input type=\"datetime-local\" name=\"endtime\">";
  html += "<input type=\"submit\" value = \"Endzeit\">";
  html += "</form>";
  html += "</body></html>";
  return html;
}

void handle_root() {
  if (server.hasArg("starttime")) {
    if (server.arg("starttime") != NULL) {
      starttime_ = server.arg("starttime");
    }
  }
  if (server.hasArg("endtime")) {
    if (server.arg("endtime") != NULL) {
      endtime_ = server.arg("endtime");
    }
  }
  server.send(200, "text/html", create_html_header() );
}

void handle_save() {
  String fileSave = starttime_ + "," + endtime_ + "," + distance_ + "\n\r";
  appendFile(SPIFFS, "/routen.txt", fileSave.c_str());
  button_save = " style=\"background-color: blue\"";
  server.send(200, "text/html", create_html_header());
}

void readFile(fs::FS &fs, const char * path){
    Serial.printf("Reading file: %s\r\n", path);

    File file = fs.open(path);
    if(!file || file.isDirectory()){
        Serial.println("- failed to open file for reading");
        return;
    }

    Serial.println("- read from file:");
    while(file.available()){
//    Serial.write(file.read());
    char buff_read = file.read();
    file_read_ += buff_read;
    }
    file.close();
}

void appendFile(fs::FS & fs, const char * path, const char * message) {
  Serial.printf("Appending to file: %s\n", path);

  File file = fs.open("/routen.txt", FILE_APPEND);
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