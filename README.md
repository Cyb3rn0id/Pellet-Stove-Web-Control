# Pellet Stove Web Control

Remote web control for some models of Pellet Stoves. I've developed it for my stove (an Ungaro Maia Blend 34) but I think will work on other stoves based on Micronova controller board. It is a non-invasive circuit since uses the infrared control.

Source code is for the NodeMCU Devkit (ESP8266 based) programmed with Arduino IDE.

![webpage](https://github.com/Cyb3rn0id/Pellet-Stove-Web-Control/blob/master/images/_overview.jpg)

## STEP 1

In the Arduino IDE install first the required libraries you can do this in the Menù Sketch -> #include library -> Library management. Type the library name and select the correct library from the list, then push "Install" button.

* OneWire (by Paul Stoffregen) => https://www.pjrc.com/teensy/td_libs_OneWire.html 
* DallasTemperature (by Miles Burton) => https://github.com/milesburton/Arduino-Temperature-Control-Library 
* IRremoteESP8266 (by Mark Szabo) => https://github.com/markszabo/IRremoteESP8266 

## STEP 2

Note: This step is only required if the IR remote command of your stove is different than mine (please see pictures in the images folder for the remote command reference).

Mount an 38KHz IR receiver as showed in the schematic (documents folder) and then upload the "IRrecvDumpV2.ino" sketch in the IRremoteESP8266 example folder.

After uploaded, start the Serial Monitor, point the IR remote on the receiver, push buttons and copy codes given in the serial monitor on a text file.

## STEP 3

Edit source code. Change your SSID, Passphrase, and other variables. If you have followed Step 2, change codes in the "IR Remote commands" section. Note: in this case you must change also the second parameter in the `irsend.sendRaw` functions in the `handleSubmit()` function. The second parameter is the array length of the command. Example:

My Toggle (ON/OFF) command, contained in the `TOGGLE[]` array, is 21 bytes long and works with a modulation at 38KHz, so the irsend command is:

`irsend.sendRaw(TOGGLE, 21, 38); //21=21 bytes long, 38=38KHz`

## STEP 4

Point the IR led of your circuit on the receiver of the stove and try to connect to board with your phone, a pin is required first than send any command, default one is 1234, you can change it in the `pinPs` variable.

## STEP 5

If you like this project, please condider to make a little gift (not only money, I collect also a lot of garbage!): http://www.settorezero.com/wordpress/donazioni/
