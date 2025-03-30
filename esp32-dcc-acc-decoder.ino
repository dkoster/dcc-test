/**
 * ESP32 DCC Accessory Decoder v2.1
 *
 * This is script enables a Marklin Mobile Station 3 
 * to interact with the DCC-EX CommandStation (dcc-ex.com)
 * 
 * This code is specific for the ESP32 WeMos D1 R32
 * and uses this schema to read the DCC signal and
 * converts this into DCC-EX commands.
 * Instead of using Serial connection, which caused a lot
 * of headaches due to common ground issues, etc. 
 * This uses WiFi to transmit the commands, which opens up
 * more possiblities. 
 */

#include <WiFi.h>
#include <SPI.h>
#include "Arduino.h"

#define ONBOARD_LED 2

WiFiClient client; // WiFi class, client.

//script variables

/**
 * WiFi AP SSID and password setup
 * cp the wifi_credentials.h.example to wifi_credentials.h
 */
 #include "wifi_credentials.h"

/**
 * Custom vars just for the code.
 */
const char* decoder_version = "2.1";
const char* dccex_host = "192.168.178.8";
const int dccex_port = 2560;

/**
 * onebittime is a time gap in microseconds that is used to time the bits transitted by DCC. 
 * My Marklin Mobile Station 3 works best with 150 ms.
 * If your system struggles try changing the value of this line. 
 * I would NOT advise going much above or below 130-180
 */
int onebittime = 150;

byte timeRiseFall = 1;  //0 = Rising 1 = Falling
byte timingSync = 1;    //1 means in sync
int timingSyncCounter;  //Counter that works through a number of interrupts to check they are correct

static byte  ISRRISING;
static unsigned long ISRRISETime;
static unsigned long ISRLastRISETime;
static byte  Preamble = 0;
static byte  PreambleCounter = 0;
static byte  PacketStart = 0;
static byte  bitcounter = 0;
static byte  packetdata[28];  //array to get all the data ready for processing
int counter = 0;

const int dccpin = 5;   //set to PIN you want to use, this set to GPIO5.

//This is the interrupt function checking when pin state change
void IRAM_ATTR pinstate(){
  ISRRISING = 1;
  ISRRISETime = micros();
}

/**
 * NOTE: 
 * Due to the way DCC sends its commands, this function will be triggered multiple times
 * for the same command. This is by design and not a bug. So you need to add some logica
 * to your command solution, that prevents it from being executed multiple times. Useless
 * to flip a turnout multiple times and may cause issues. 
 */
void ControlAccDecoder(byte index,byte Direction,int Addr,int BoardAddr)
{
  /**
   * Some static vars we can use to keep track of the latest
   * DCC address/action call, which we use in our logica.
   */
  static uint16_t lastAddr      = 0xFFFF;
  static uint8_t  lastDirection = 0xFF;

  // Using switch, which beats a lot of if then else structure.
  switch(Addr)
  {
    /**
     * This would be an example of a none turnout accessory under ID 25.
     * The function still executes a turnout command, but you get the point.
     */
    case 25:
      if (!((Addr == lastAddr) && (Direction == lastDirection))) {
        if(Direction < 1) {
          client.printf("<T %d %d>\n", Addr, Direction);
          Serial.printf("Sending '<T %d %d>' to %s:%d...\n", Addr, Direction, dccex_host, dccex_port);        
        } 
        else {
          client.printf("<T %d %d>\n", Addr, Direction);
          Serial.printf("Sending '<T %d %d>' to %s:%d...\n", Addr, Direction, dccex_host, dccex_port);
        }
      }
      break;

    // Default is used for turnouts, to keep the code clean and simple
    default:
      /**
       * This if prevents the turnout action to be executed multiple times
       */
      if (! ( (Addr == lastAddr) && (Direction == lastDirection) ) ) {
        client.printf("<T %d %d>\n", Addr, Direction);
        Serial.printf("Sending '<T %d %d>' to %s:%d...\n", Addr, Direction, dccex_host, dccex_port);
      }
      break;
  }

  // Store last Addr and Direction in the static vars.
  lastAddr      = Addr;
  lastDirection = Direction;
}

void setup() {
  Serial.begin(115200); // Setup Serial
  Serial.printf("Welcome to ESP32 DCC Accessory Decoder V%s\n", decoder_version); // some nice greating

  pinMode(dccpin, INPUT_PULLUP);  // Enable the internal pullup resistor for the dccpin
  pinMode(ONBOARD_LED, OUTPUT);   // Set the onboard led port to output

  attachInterrupt(digitalPinToInterrupt(dccpin),pinstate, RISING);  // Attach the dccpin to the interrupt call
 
  WiFi.onEvent(WiFiEvent);    // set WiFiEvent function for all wifi events.
  WiFi.mode(WIFI_STA);        // set WiFi mode to station mode, so it connects to existing AP
  WiFi.begin(ssid, password); // Startup and connect the wifi
}

void loop() {
  /**
   * If we have a WiFi connection and we are not connected to the 
   * DCC-EX CommandStation, setup the connection. Otherwise we cannot 
   * transmit the commands and nothing happens.
   */
  if ( (WiFi.status() == WL_CONNECTED) && (! client.connected()) )
  {
    Serial.printf("Connecting to %s:%d\n", dccex_host, dccex_port); // Some serial output
    client.connect(dccex_host, dccex_port); // Connect to the DCC-EX CS IP:PORT
    if (client.connected()) {
      Serial.printf("Connected to %s:%d\n", dccex_host, dccex_port);  // report back connected
      digitalWrite(ONBOARD_LED, HIGH);  // Enable onboard led for some feedback
    }
  }

  /**
   * If we have a working connection, check for any feedback from the DCC-EX CS
   */
  if (client.available()) 
  {
    char ch = static_cast<char>(client.read());
    Serial.print(ch);
  }

  /**
   * Processing the DCC part. 
   * Make sure you DO NOT use any delay in your code.
   * This will break your DCC input and nothing works
   */
  processDCC();
}