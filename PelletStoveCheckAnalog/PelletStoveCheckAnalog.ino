/*
PELLET STOVE CHECK ANALOG
This sketch is used for checking analog value from Cochlea Sensor
Copyright (c) 2017 Giovanni Bernardo (CYB3rn0id)
http://www.settorezero.com
http://www.facebook.com/settorezero
http://www.twitter.com/settorezero

SETTINGS for Arduino IDE for use with NodeMCU Devkit
----------------------------------------------------
Board:              Generic ESP8266 Module
Flash mode:         DIO
Flash frequency:    40MHz
CPU frequency:      80MHz
Flash size:         4M (3M SPIFFS)
Reset method:       nodemcu
Upload speed:       115200

NOTE on NodeMCU Analog input
----------------------------
NodeMCU Devkit has a voltage divider on Analog input of ESP8266.
On a bare ESP8266 module you cannot give more than 1V on analog input
On NodeMCU devkit, since the voltage divider,
you can give up to 3.3V (3.3V = reading of 1024).

NOTE on NodeMCU Powering
------------------------
The NodeMCU devkit has an on-board 3.3V voltage regulator.
You can power NodeMCU devkit using the microUSB connector. In this case
you can obtain a 5V voltage on "VU" pin (in this project 5V is used for powering
the Active Buzzer).
OR you can power NodeMCU giving 5V on VIN pin (in this case you'll obtain the 5V
from this pin).

MIT License
-----------
Copyright (c) 2017 Giovanni Bernardo

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

// Used IOs
#define BUZZER        16  // active buzzer. Note: while uploading, GPIO16 goes high!
#define BUTTON        12  // used for reset settings
#define WIFILED       2   // wifi connected led
#define RECV_PIN      14  // used only for sketch IRReceive
#define IRLED         4   // infrared led
#define ONE_WIRE_BUS  5   // DS1820

void setup() 
  {
  pinMode(BUZZER,OUTPUT);
  pinMode(BUTTON,INPUT);
  pinMode(WIFILED,OUTPUT);
  pinMode(RECV_PIN,INPUT);
  pinMode(IRLED,OUTPUT);
  pinMode(ONE_WIRE_BUS,INPUT);
  
  digitalWrite(BUZZER,LOW);
  digitalWrite(IRLED,LOW);
  digitalWrite(WIFILED,HIGH);
  Serial.begin(115200);
  }

void loop() 
  {
  Serial.println(analogRead(A0));
  delay(100);
  }
