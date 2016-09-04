#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino

#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <WiFiManager.h>         //https://github.com/tzapu/WiFiManager
#include <Wire.h>
#include <Adafruit_BMP085.h>
                                // Connect VCC of the BMP085 sensor to 3.3V (NOT 5.0V!)
                                // Connect GND to Ground
                                // Connect SCL to i2c clock thats Analog 5 = D1
                                // Connect SDA to i2c data  thats Analog 4 = D2
//#include "my_html_page.h"

#define SIZE 96 // 24 hour every 15 min
const unsigned long PERIOD = 5000;
const String htmlHeader = "<html>\
  <head>\
    <meta charset='UTF-8'>\
    <meta http-equiv='refresh' content='20'/>\
    <title>Барометр</title>\
    <style>\
      body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }\
    </style>\
  </head>\
  <body>";
const String htmlHeader1 = "<html><head><meta charset='UTF-8'><title>Барометр (статистика)</title></head><body>";
const String mySvgHeader = "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\" width=\"255\" height=\"200\">\n\
<rect width=\"96\" height=\"200\" fill=\"rgb(250, 230, 210)\" stroke-width=\"1\" stroke=\"rgb(0, 0, 0)\" />\n\
<rect width=\"96\" height=\"200\" x=\"127\" y=\"0\" fill=\"rgb(210, 230, 250)\" stroke-width=\"1\" stroke=\"rgb(0, 0, 0)\" />\n\
<g stroke=\"blue\">\n<line x1=\"0\" y1=\"100\" x2=\"96\" y2=\"100\" stroke-width=\"1\" />\n</g>\n\
<g stroke=\"green\">\n<line x1=\"127\" y1=\"100\" x2=\"223\" y2=\"100\" stroke-width=\"1\" />\n</g>\
\n<text\nx=\"97\"\ny=\"100\"\nstyle=\"font-size:9px\">0</text>\
\n<text\nx=\"97\"\ny=\"80\"\nstyle=\"font-size:9px\">10</text>\
\n<text\nx=\"97\"\ny=\"60\"\nstyle=\"font-size:9px\">20</text>\
\n<text\nx=\"97\"\ny=\"40\"\nstyle=\"font-size:9px\">30</text>\
\n<text\nx=\"97\"\ny=\"120\"\nstyle=\"font-size:9px\">-10</text>\
\n<text\nx=\"97\"\ny=\"140\"\nstyle=\"font-size:9px\">-20</text>\
\n<text\nx=\"97\"\ny=\"160\"\nstyle=\"font-size:9px\">-30</text>\
\n<text\nx=\"224\"\ny=\"100\"\nstyle=\"font-size:9px\">760</text>\
\n<text\nx=\"224\"\ny=\"80\"\nstyle=\"font-size:9px\">780</text>\
\n<text\nx=\"224\"\ny=\"60\"\nstyle=\"font-size:9px\">800</text>\
\n<text\nx=\"224\"\ny=\"40\"\nstyle=\"font-size:9px\">820</text>\
\n<text\nx=\"224\"\ny=\"120\"\nstyle=\"font-size:9px\">740</text>\
\n<text\nx=\"224\"\ny=\"140\"\nstyle=\"font-size:9px\">720</text>\
\n<text\nx=\"224\"\ny=\"160\"\nstyle=\"font-size:9px\">700</text>";

int TAIL = 0;
int pressBuf[SIZE]={0};
float tempBuf[SIZE]={0};
int pressBufout[SIZE]={0};
float tempBufout[SIZE]={0};
unsigned long old=0;

void handleRoot();
void drawGraph();
void printStat();

Adafruit_BMP085 bmp;
ESP8266WebServer server (80);

void setup() {
    Serial.begin(115200);
    WiFiManager wifiManager;
    wifiManager.setSTAStaticIPConfig(IPAddress (192,168,1,11), IPAddress (192,168,1,1), IPAddress (255,255,255,0));
    wifiManager.autoConnect("AutoConnectAP");
    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(WiFi.SSID());
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

  if (!bmp.begin()) {
  Serial.println("Could not find a valid BMP085 sensor, check wiring!");
  server.send (200, "text/plain", "Could not find a valid BMP085 sensor, check wiring!");
  while (1) {}
  }

    if (MDNS.begin ("esp8266")) {
    Serial.println ("MDNS responder started");
  }
    server.on ("/", handleRoot);
    server.on ("/stat", printStat);
    server.on ( "/test.svg", drawGraph );
    server.begin();
}

void loop() {
    if ((millis()-old)>PERIOD) {
    pressBuf[TAIL]=bmp.readPressure();
    tempBuf [TAIL]=bmp.readTemperature();
    int i=0;
    for (int j=TAIL+1;j<SIZE;j++){
      pressBufout[i]=pressBuf[j];
      tempBufout[i]=tempBuf[j];
      i++;
    }
    for (int j=0;j<TAIL+1;j++){
      pressBufout[i]=pressBuf[j];
      tempBufout[i]=tempBuf[j];
      i++;
    }
    TAIL++;
    if(TAIL==SIZE) TAIL=0;
    old=millis();
}
  server.handleClient();
}


void handleRoot() {
  char temp[40];
  int sec = millis() / 1000;
  int min = sec / 60;
  int hr = min / 60;
  snprintf ( temp, 40,"<p>Uptime: %02d:%02d:%02d</p>",hr, min % 60, sec % 60);
  String s = htmlHeader;
    s+="<h1>Температура:&nbsp;";    
    s+=bmp.readTemperature();
    s+="&nbsp;&deg;C</h1>";
    s+="<h1>Давление:<br>";    
    s+=bmp.readPressure();
    s+="&nbsp;Па<br>";
    s+=bmp.readPressure()/133.322;
    s+="&nbsp;мм рт. ст.</h1>";
    s+="<h6>Давление, приведенное к уровню моря:&nbsp;"; 
    s+=bmp.readSealevelPressure();
    s+="&nbsp;Па или &nbsp;";
    s+=bmp.readSealevelPressure()/133.322;
    s+="&nbsp;мм рт. ст.</h6>";
    s+="<img src=\"/test.svg\" />";
    s+=temp;
    s+="</body></html>";
  server.send ( 200, "text/html", s);
  }


void drawGraph() {
  String out = "";
  char temp[100];
  out += mySvgHeader;
  out += "<g stroke=\"black\">\n";
  int x=0;
  for (int i = SIZE-96; i < SIZE; i++) {
    int y = tempBufout[i];
    sprintf(temp, "<line x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" stroke-width=\"1\" />\n", x, 100-y*2, x + 1,99-y*2);
    out += temp;
    x++;
  }
    out += "</g>\n";
    out+="<g stroke=\"black\">\n";
  x=127;
  for (int i = SIZE-96; i < SIZE; i++) {
    int y = pressBufout[i]/133.322;
    sprintf(temp, "<line x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" stroke-width=\"1\" />\n", x, 100-(y-760), x + 1,99-(y-760));
    out += temp;
    x++;
  }
  out += "</g>";

  out +="</svg>\n";

  server.send ( 200, "image/svg+xml", out);
}


void printStat() {
  String stat = htmlHeader1;
  stat+="<table>\n<tr><td>Температура, &deg;C</td><td>Давление, Па</td><td></td></tr>";
  for (int i=TAIL+1; i<SIZE; i++){
       stat+="<tr><td>";
       stat+=tempBuf[i];
       stat+="</td><td>";
       stat+=pressBuf[i];
       stat+="</td><td>";
       stat+=i;
       stat+="</td></tr>\n";
      }

  for (int i=0; i<TAIL+1; i++){
       stat+="<tr><td>";
       stat+=tempBuf[i];
       stat+="</td><td>";
       stat+=pressBuf[i];
       stat+="</td><td>";
       stat+=i;
       stat+="</td></tr>\n";
      }
  stat+="</table></body></html>";
  server.send ( 200, "text/html", stat);
}

/* TODO
 *  WebUpdate - основа для удаленного обновления. Сделать отдельной страницей
 *  AdvancedWebServer - основа для страниц
 */
