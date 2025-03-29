const char* ssid = "";
const char* password = "";

const char* dccex_host = "192.168.178.8";
const int dccex_port = 2560;
const char* debug_host = "192.168.178.16";
const int debug_port = 9000;

#define CONNECTION_TIMEOUT 10
#define ONBOARDLED 2

#include <WiFi.h>
#include <SPI.h>
#include "Arduino.h"

WiFiClient client;
WiFiClient client2;

//script variables

/*onebittime is a time gap in microseconds that is used to time the bits transitted by
DCC. I have got values between 130 and 180 to work on an NCE system
If your system struggles try changing the value of this line. I would NOT advise going much above or below 130-180
*/

//int onebittime = 130;//works NCE
//int onebittime = 150;//works nce..using average
int onebittime = 140;//works NCE

byte timeRiseFall = 1;//0 = Rising 1 = Falling
byte timingSync = 1; //1 means in sync
int timingSyncCounter; //Counter that works through a number of interrupts to check they are correct

static byte  ISRRISING;
static unsigned long ISRRISETime;
static unsigned long ISRLastRISETime;
static byte  Preamble = 0;
static byte  PreambleCounter = 0;
static byte  PacketStart = 0;
static byte  bitcounter = 0;
static byte  packetdata[28];//array to get all the data ready for processing
int counter = 0;

const int dccpin = 5; //set to PIN you want to use

//This is the interrupt function checking when pin state change
void IRAM_ATTR pinstate(){
  ISRRISING = 1;
  ISRRISETime = micros();
}

void ControlAccDecoder(byte index,byte Direction,int Addr,int BoardAddr)
{
  static uint16_t lastAddr      = 0xFFFF;
  static uint8_t  lastDirection = 0xFF;
  
  char buffer[20];
  char buffer2[20];

  if (! client.connected())
  {
    Serial.printf("Connecting to %s:%d\n", dccex_host, dccex_port);
    client.connect(dccex_host, dccex_port);
  }

  if (! client2.connected())
  {
    client2.connect(debug_host, debug_port);
  }

  switch(Addr)
  {
    default:
      if (!((Addr == lastAddr) && (Direction == lastDirection))) {
        sprintf(buffer, "<T %d %d>", Addr, Direction);
        Serial.print("Sending... ");
        Serial.println(buffer);
        client.println(buffer);
      }
      break;
  }

  lastAddr      = Addr;
  lastDirection = Direction;
}

void setup() {
  Serial.begin(115200);
  Serial.println("ESP32DecoderV10"); 
  pinMode(dccpin,INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(dccpin),pinstate, RISING);
 
  WiFi.onEvent(WiFiEvent);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  if (client.connect(dccex_host, dccex_port))
  {
    Serial.printf("Connected to %s:%d\n", dccex_host, dccex_port);
  }
 
 if (client2.connect(debug_host, debug_port))
  {
    Serial.printf("Connected to %s:%d\n", debug_host, debug_port);
  }
}

void loop() {
  processDCC();
}
