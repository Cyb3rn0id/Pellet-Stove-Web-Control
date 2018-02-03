/*
PELLET STOVE IR RECEIVE
*/

#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRutils.h>

#define BUZZER 16 // active buzzer
#define BUTTON 12
#define WIFILED 2
#define RECV_PIN 14
#define IRLED 4
#define ONE_WIRE_BUS 5 // DS1820 on D1 (GPIO5) - don't forget a 4K7 pull-up (to 3.3V!)

#define BAUD_RATE 115200

// As this program is a special purpose capture/decoder, let us use a larger
// than normal buffer so we can handle Air Conditioner remote codes.
#define CAPTURE_BUFFER_SIZE 1024

// TIMEOUT is the Nr. of milli-Seconds of no-more-data before we consider a
// message ended.
// This parameter is an interesting trade-off. The longer the timeout, the more
// complex a message it can capture. e.g. Some device protocols will send
// multiple message packets in quick succession, like Air Conditioner remotes.
// Air Coniditioner protocols often have a considerable gap (20-40+ms) between
// packets.
// The downside of a large timeout value is a lot of less complex protocols
// send multiple messages when the remote's button is held down. The gap between
// them is often also around 20+ms. This can result in the raw data be 2-3+
// times larger than needed as it has captured 2-3+ messages in a single
// capture. Setting a low timeout value can resolve this.
// So, choosing the best TIMEOUT value for your use particular case is
// quite nuanced. Good luck and happy hunting.
// NOTE: Don't exceed MAX_TIMEOUT_MS. Typically 130ms.
#define TIMEOUT 15U  // Suits most messages, while not swallowing many repeats.

// Set the smallest sized "UNKNOWN" message packets we actually care about.
// This value helps reduce the false-positive detection rate of IR background
// noise as real messages. The chances of background IR noise getting detected
// as a message increases with the length of the TIMEOUT value. (See above)
// The downside of setting this message too large is you can miss some valid
// short messages for protocols that this library doesn't yet decode.
//
// Set higher if you get lots of random short UNKNOWN messages when nothing
// should be sending a message.
// Set lower if you are sure your setup is working, but it doesn't see messages
// from your device. (e.g. Other IR remotes work.)
// NOTE: Set this value very high to effectively turn off UNKNOWN detection.
#define MIN_UNKNOWN_SIZE 12
// ==================== end of TUNEABLE PARAMETERS ====================


// Use turn on the save buffer feature for more complete capture coverage.
IRrecv irrecv(RECV_PIN, CAPTURE_BUFFER_SIZE, TIMEOUT, true);
decode_results results;  // Somewhere to store the results

  
// The section of code run only once at start-up.
void setup() 
  {
  pinMode(BUZZER,OUTPUT);
  pinMode(WIFILED,OUTPUT);
  pinMode(IRLED,OUTPUT);
  pinMode(BUTTON,INPUT);
  digitalWrite(BUZZER,LOW);
  digitalWrite(WIFILED,HIGH);
  digitalWrite(IRLED,LOW);
  Serial.begin(BAUD_RATE, SERIAL_8N1, SERIAL_TX_ONLY);
  delay(500);  // Wait a bit for the serial connection to be establised.

  #if DECODE_HASH
  // Ignore messages with less than minimum on or off pulses.
  irrecv.setUnknownThreshold(MIN_UNKNOWN_SIZE);
  #endif  // DECODE_HASH
  irrecv.enableIRIn();  // Start the receiver
  Serial.println("");
  Serial.println("");
  Serial.println(F("IR Receiver v 1.0"));
  Serial.println("");
  }

// The repeating section of the code
//
void loop() 
  {
  // Check if the IR code has been received.
  if (irrecv.decode(&results)) 
    {
    // Display a crude timestamp.
    digitalWrite(BUZZER,HIGH);
    digitalWrite(WIFILED,LOW);
    if (results.overflow)
      Serial.printf("WARNING: IR code is too big for buffer (>= %d). "
                    "This result shouldn't be trusted until this is resolved. "
                    "Edit & increase CAPTURE_BUFFER_SIZE.\n",
                    CAPTURE_BUFFER_SIZE);
    // Display the basic output of what we found.
    Serial.print(resultToHumanReadableBasic(&results));
    yield();  // Feed the WDT as the text output can take a while to print.
    // Output the results as source code
    Serial.println(resultToSourceCode(&results));
    Serial.println("");  // Blank line between entries
    digitalWrite(BUZZER,LOW);
    digitalWrite(WIFILED,HIGH);
    yield();  // Feed the WDT (again)
    }
  }
