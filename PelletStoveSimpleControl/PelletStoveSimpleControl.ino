/*
PELLET STOVE SIMPLE CONTROL
Web Remote control for Pellet Stoves based on some "Micronova" controllers
Tested on UNGARO Maia Blend 34 model
Copyright (c) 2017 Giovanni Bernardo (CYB3rn0id)
http://www.settorezero.com
http://www.facebook.com/settorezero
http://www.twitter.com/settorezero

DESCRIPTION
----------------------------------------------------
This application runs on an NodeMCU Devkit connected via Wi-Fi in a local network 
and make same functions such the original IR remote command.
You can turn on/off the stove, set the power and water temperature.
You can read the ambient temperature too and check if the stove is working or not.
Page is pin/password protected.
Maybe works on every stove that has a Micronova controller board with double led 
display and an IR remote with 4 buttons

PREREQUISITES
----------------------------------------------------
(libraries to be installed first : Menu Sketch -> #include library -> Library management)
- OneWire (by Paul Stoffregen) => https://www.pjrc.com/teensy/td_libs_OneWire.html 
- DallasTemperature (by Miles Burton) => https://github.com/milesburton/Arduino-Temperature-Control-Library 
- IRremoteESP8266 (by Mark Szabo) => https://github.com/markszabo/IRremoteESP8266 

SETTINGS for Arduino IDE for use with NodeMCU Devkit
----------------------------------------------------
*: is the default setting on Arduino IDE, so you must not change
I've reported default settings only in case you change one accidentally
!: is the setting you must change, so you can focus only on it!

Board:              Generic ESP8266 Module
Upload speed:       115200*
CPU frequency:      80MHz*
Flash frequency:    40MHz*
Flash mode:       ! DIO
Flash size:       ! 4M (3M SPIFFS)
Crystal Frequency:  26MHz*
Reset method:     ! nodemcu
Debug port:         disabled*
Debug level:        none*
IwIP variant:       v2 lower memory*
Vtables:            flash*
Exceptions:         disabled*
Builtin Led:        2*
Erase Flash:        Only sketch*

NOTE on NodeMCU Analog input
----------------------------------------------------
NodeMCU Devkit has a voltage divider on Analog input of ESP8266.
On a bare ESP8266 module you cannot give more than 1V on analog input
On NodeMCU devkit, since the voltage divider,
you can give up to 3.3V (3.3V = reading of 1024).

NOTE on NodeMCU Powering
----------------------------------------------------
The NodeMCU devkit has an on-board 3.3V voltage regulator.
You can power NodeMCU devkit using the microUSB connector. In this case
you can obtain a 5V voltage on "VU" pin (in this project 5V is used for powering
the Active Buzzer).
OR you can power NodeMCU giving 5V on VIN pin (in this case you'll obtain the 5V
from this pin).

LICENSE
----------------------------------------------------
Copyright (c)2019 Giovanni Bernardo

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.
 
You should have received a copy of the GNU Affero General Public License
along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiClient.h>
#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <OneWire.h>
#include <DallasTemperature.h>

/*
NodeMCU to Arduino GPIO pinout
#define D0  16
#define D1   5      // SCL
#define D2   4      // SDA
#define D3   0		// (has pullup => does not connect anything that pulls this pin low at startup!)
#define D4   2		// OnBoard LED (led act as pullup => does not connect anything that pulls this pin low at startup!)
#define D5  14
#define D6  12
#define D7  13
#define D8  15      // (has pulldown => does not connect anything that pulls this pin high at startup!)
#define D9   3      // RX
#define D10  1      // TX
*/

// Used IOs
#define BUZZER        16  // active buzzer. Note: while uploading, GPIO16 goes high!
#define BUTTON        12  // used for reset settings
#define WIFILED       2   // wifi connected led
#define RECV_PIN      14  // used only for sketch IRReceive
#define IRLED         4   // infrared led
#define ONE_WIRE_BUS  5   // DS1820 on D1 (GPIO5) - don't forget a 4K7 pull-up (to 3.3V!)

IRsend irsend(IRLED); // IR Led on D2 (GPIO4) - direct driven without resistor
OneWire oneWire(ONE_WIRE_BUS); // Setup a oneWire instance
DallasTemperature Temperature(&oneWire); // Pass our oneWire reference to Dallas Temperature. 

#define USE_STATIC_IP // comment this if you want address assigned by DHCP

// SSID and Password of your Wi-Fi Network
const char* ssid = "[YOUR SSID]";
const char* password = "[YOUR PASSPHRASE]";

// Page password
String pinPs="1234"; // your *numeric* pin/password for accessing the page, leave blank if you don't want a pin

// Analog Threshold for detecting cochlea running
#define AnalogTH 85

// Milliseconds for analog signal antibounce
#define msAnalogAB 50

// Milliseconds for "stay alive" cochlea running
#define msCo  12000L

// data used for static IP configuration
#ifdef USE_STATIC_IP
  IPAddress subnet(255,255,255,0);
  IPAddress ip(192,168,1,31); // board address
  IPAddress gateway(192,168,1,1); // router address
#endif

ESP8266WebServer server(80);

// user for controlling stove running, an inductive sensor
// is mounted around a cable of the cochlea motor
unsigned long LastTimeRunning=0;
String MsgStoveOn="La stufa &egrave; <span style=\"color:#33cc00;\">IN FUNZIONE</span>"; // message for "stove is working  
String MsgStoveOff="La stufa &egrave; <span style=\"color:#FF3300;\">FERMA</span>"; // message for "stove is stopped"

// IR remote commands obtained with IRrecvDumpV2.ino for Ungaro Maia 34 Blend pellet stove (Micronova controller board)
// those codes may work on other Micronova controllers that have the 4 buttons remote
// ON/OFF (toggle):
uint16_t TOGGLE[21] = {6680, 2412,  3374, 1560,  920, 732,  918, 1594,  1720, 758,  894, 1584,  896, 732,  920, 1558,  1748, 758,  1722, 1584,  1720};  // UNKNOWN 9EB58962
// Power Up (flame up):
uint16_t P_UP[19] = {6778, 2336,  3446, 1510,  938, 712,  938, 1556,  4270, 686,  936, 716,  938, 1540,  3438, 692,  936, 714,  1774};  // UNKNOWN 2B890461
// Power Down (flame down):
uint16_t P_DN[21] = {6700, 2406,  3366, 1580,  892, 756,  892, 1598,  2562, 732,  918, 726,  922, 696,  948, 1544,  2544, 1558,  904, 734,  1728};  // UNKNOWN BD1A8437
// Water Temperature Up:
uint16_t T_UP[23] = {6710, 2352,  3422, 1562,  894, 732,  918, 1572,  1746, 758,  1720, 758,  892, 734,  924, 1554,  1744, 756,  892, 756,  892, 756,  1736};  // UNKNOWN 4B3AB9D1
// Water Temperature Down:
uint16_t T_DN[19] = {6726, 2390,  3394, 1536,  944, 734,  916, 1576,  3394, 1538,  942, 736,  918, 1534,  4246, 1536,  190, 86,  1602};  // UNKNOWN 56631A11

// Return Temperature value from DS1820 sensor in string format
String getTemperature(void)
 {
 float T=0;
 char stringTemp[7];
 Temperature.requestTemperatures();
 T=Temperature.getTempCByIndex(0);
 // string will be 5 chars long with 1 decimal, /0 included
 // if shorter than 5 char, initial chars will be spaces
 dtostrf(T,5,1,stringTemp); //https://blog.protoneer.co.nz/arduino-float-to-string-that-actually-works/
 return String(stringTemp)+"&deg;C";
 }

// Return "Stove working/stopped" in string format
String getStoveWorking(void)
  {
  static bool CochleaRunning=false;
  if (analogRead(A0)>AnalogTH) // some voltage detected => cochlea is running?
    {
    delay (msAnalogAB); // antibounce
    if (analogRead(A0)>AnalogTH)
        {
        LastTimeRunning=millis();  
        CochleaRunning=true;
        }
    }
  else // no voltage detected
    {
    if (millis()<LastTimeRunning) // millis() rollover occurred, reset
      {
      LastTimeRunning=0;  
      }
    else
      {
      // if more than msCo seconds passed from last detected running
      if ((millis()-LastTimeRunning)>msCo)
        {
        // re-check if voltage present
        if (analogRead(A0)<AnalogTH) // no voltage present
          {
          CochleaRunning=false;
          }
        else // voltage present
          {
          CochleaRunning=true;
          LastTimeRunning=millis();  
          }
        } 
      }   
    }
  if (CochleaRunning)
    {
    return (MsgStoveOn);  
    }
  else
    {
    return MsgStoveOff;   
    }
  }
  
// return client IP in String format
String clientIP(void)
  {
  return server.client().remoteIP().toString();  
  }

// turn on/off the wifi led
void turnLed(bool v)
  {
  v?digitalWrite(WIFILED,LOW):digitalWrite(WIFILED,HIGH);  
  }


// Return HTML page
// every button is a "Submit"-type button
String Index_Html(void)
  {
  return
  "<!DOCTYPE HTML>"
  "<html>"
  "<head>"
  "<title>Stufa</title>"
  "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=0\">"
  "<style type=\"text/css\">"
  "body{font-family:arial; font-size:15pt; font-weight:bold;}"
  ".bu{font-family:arial; font-weight:bold; font-size:27pt; color:#ffffff; text-align:center; padding:7px; border:0; border-radius:15px; box-shadow:0 8px #666666; outline:none; width:100%;}"
  ".bu:active{box-shadow:0 3px #333333; transform:translateY(4px);}"
  ".buno{font-family:arial; font-weight:bold; font-size:27pt; color:#ffffff; text-align:center; padding:7px; border:0; border-radius:15px; box-shadow:0 8px #666666; outline:none; background-color:#b63642;}"
  ".tx{font-family:arial; font-size:26pt; text-align:left; padding:8px; border:2px #c9c9c9 solid; border-radius:15px;}"
  ".sm{color:#585858; text-decoration:none; font-family:tahoma,arial; font-size:10pt; font-weight:normal; font-variant:small-caps; padding:8px; margin-top:10px; display:block;}"
  "</style>"
  "<script language=\"javascript\">function getValues(){var sensorValues=[];var xhr=new XMLHttpRequest();xhr.open('GET','/values.txt',true);xhr.onload=function(){if (xhr.readyState==4 && xhr.status==200){sensorValues=xhr.responseText.split(\",\");document.getElementById(\"temp\").innerHTML=sensorValues[0];document.getElementById(\"stove\").innerHTML=sensorValues[1];}};xhr.send(null);}setInterval('getValues()', 2000);</script>"
  "</head>"
  "<body onload=\"getValues()\">"
  "<div style=\"text-align:center; min-width:260px; display:inline-block;\">"
  "<div>Pellet Stove Web Control</div>"
  "<div class=\"sm\" id=\"stove\" style=\"font-weight:bold\">"+getStoveWorking()+"</div>"
  "<div class=\"buno\" id=\"temp\">"+getTemperature()+"</div><br/>"
  "<div><form name=\"f\" method=\"post\">"
  "<input type=\"submit\" name=\"submit\" value=\"no\" style=\"visibility:hidden; display:none; height:0px; margin:0px;\">"
  "<input type=\"submit\" name=\"submit\" value=\"ON/OFF\" class=\"bu\" style=\"background-color:#ff6600;\"><br/><br/>"
  "<input type=\"submit\" name=\"submit\" value=\"Pow +\" class=\"bu\" style=\"background-color:#00FF00; width:50%;\">"
  "<input type=\"submit\" name=\"submit\" value=\"Temp +\" class=\"bu\" style=\"background-color:#0099FF; width:50%;\"><br/><br/>"
  "<input type=\"submit\" name=\"submit\" value=\"Pow -\" class=\"bu\" style=\"background-color:#00CC00; width:50%;\">"
  "<input type=\"submit\" name=\"submit\" value=\"Temp -\" class=\"bu\" style=\"background-color:#0099AA; width:50%;\"><br/>"
  "<span class=\"sm\"><b>PIN:</b></span>"
  "<input type=\"hidden\" name=\"username\" value=\"anonymous\">"
  "<input type=\"number\" name=\"password\" class=\"tx\" style=\"-webkit-text-security:disc; width:93%;\" pattern=\"[0-9]*\" inputmode=\"numeric\">"  
  "</form></div>"
  "<div class=\"sm\">&copy;2018 GIOVANNI BERNARDO</div></div></body></html>";
  // greater webpages can be sent using following method:
  // taken from https://github.com/esp8266/Arduino/issues/3205
  // server.setContentLength(HTML_PART_1().length() + HTML_PART_2().length());   (or hardcode the length)
  // server.send(200,"text/html",HTML_PART_1()); <---Initial send includes the header
  // server.sendContent((HTML_PART_2()); <--- Any subsequent sends use this function 
  }

void setup(void)
  {
  pinMode(BUZZER,OUTPUT);
  digitalWrite(BUZZER,LOW);
  pinMode(WIFILED,OUTPUT);
  
  irsend.begin();
  Temperature.begin();
  turnLed(false);
  Serial.begin(115200);
    
  // workaround to issue 2186
  // https://github.com/esp8266/Arduino/issues/2186#issuecomment-228581052
  WiFi.persistent(false);
  WiFi.mode(WIFI_OFF);
  WiFi.mode(WIFI_STA);
  
  // WiFi.config to be used after WiFi.begin
  // https://github.com/esp8266/Arduino/blob/master/doc/esp8266wifi/station-class.rst
  WiFi.begin(ssid, password);
  #ifdef USE_STATIC_IP
    WiFi.config(ip,gateway,subnet);
  #endif
  
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) 
    {
    delay(500);
    Serial.print(".");
    }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  turnLed(true);

  // double beep for indicating Wi-Fi connected
  digitalWrite(BUZZER,HIGH);
  delay(200);
  digitalWrite(BUZZER,LOW);
  delay(200);
  digitalWrite(BUZZER,HIGH);
  delay(200);
  digitalWrite(BUZZER,LOW);
    
  // attach functions on server request
  server.on("/", handleRoot);
  server.on("/values.txt",sendValues);
  server.onNotFound(handleNotFound);
  server.begin();
  LastTimeRunning=millis();
  }

void sendValues()
  {
  server.send(200, "text", getTemperature()+","+getStoveWorking()+",0");  
  }

void handleRoot()
  {
  if (server.hasArg("submit"))
    {
    Serial.println("Form submitted");
    handleSubmit();
    }
  else 
    {
    server.send(200, "text/html", Index_Html());
    }
  }

void returnFail(String msg)
  {
  server.sendHeader("Connection", "close");
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(500, "text/html", "<h1>"+ msg + "\r\n"+clientIP()+"</h1>");
  Serial.println("Failed request: "+msg);
  }

void handleSubmit()
  {
  if (!server.hasArg("submit")) return returnFail("BAD ARGS");
  // check pin, if a pin is defined
  if (pinPs!='\0')
  {
  if (server.arg("password")!=pinPs) return returnFail("PIN Errato!");
  }
  // check if user pressed "go" on keyboard
  String toExec=server.arg("submit");
  if (toExec=="no")
    {
    return returnFail("Non devi premere invio, ma un pulsante!");  
    }
  Serial.println(toExec+" command requested");
  if (toExec=="ON/OFF")
    {
    digitalWrite(BUZZER,HIGH);
    irsend.sendRaw(TOGGLE, 21, 38);
    digitalWrite(BUZZER,LOW);
    server.send(200, "text/html", Index_Html());
    Serial.println("Requested ON/OFF");
    }
  else if (toExec=="Pow +")
    { 
    digitalWrite(BUZZER,HIGH);
    irsend.sendRaw(P_UP, 19, 38);
    digitalWrite(BUZZER,LOW);
    server.send(200, "text/html", Index_Html());
    Serial.println("Requested Power Up");
    }
  else if (toExec=="Pow -")
    { 
    digitalWrite(BUZZER,HIGH);
    irsend.sendRaw(P_DN, 21, 38);
    digitalWrite(BUZZER,LOW);
    server.send(200, "text/html", Index_Html());
    Serial.println("Requested Power Down");
    }
  else if (toExec=="Temp +")
    { 
    digitalWrite(BUZZER,HIGH);
    irsend.sendRaw(T_UP, 23, 38);
    digitalWrite(BUZZER,LOW);
    server.send(200, "text/html", Index_Html());
    Serial.println("Requested Temperature Up");
    }
  else if (toExec=="Temp -")
    { 
    digitalWrite(BUZZER,HIGH);
    irsend.sendRaw(T_DN, 19, 38);
    digitalWrite(BUZZER,LOW);
    server.send(200, "text/html", Index_Html());
    Serial.println("Requested Temperature Down");
    }
  else
    {
    Serial.println("Not a valid command");
    returnFail("Bad value");
    }
  }// \HandleSubmit

void returnOK()
  {
  server.sendHeader("Connection", "close");
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/plain", "OK\r\n");
  }

void handleNotFound()
  {
  String message = "<h1>File Not Found</h1>\n\n";
  message += "<h3>URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i=0; i<server.args(); i++)
    {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
    }
  server.send(404, "text/html", message+"</h3>");
  Serial.println("File not found");
  }

void loop(void)
  {
  server.handleClient();
  }
