#include <Arduino.h>
#include <SoftwareSerial.h>
#include "Adafruit_FONA.h"

#define FONA_RX 3
#define FONA_TX 2
#define FONA_RST 4

#define FONA_RI_INTERRUPT 0

SoftwareSerial fonaSS = SoftwareSerial(FONA_TX, FONA_RX);

Adafruit_FONA fona = Adafruit_FONA(FONA_RST);
char pin[] = "3860";

void setup()
{
  Serial.begin(115200);
  Serial.println(F("Initializing....(May take 3 seconds)"));
  fonaSS.begin(9600); // if you're using software serial
  if (!fona.begin(fonaSS))
  { // can also try fona.begin(Serial1)
    Serial.println(F("Couldn't find FONA"));
    while (1)
      ;
  }
  Serial.println(F("FONA is OK"));
  // fona.unlockSIM(pin);
  // Serial.println("Waiting 10 seconds");
  // delay(10000);
  // Serial.print("Call status = ");
  // Serial.println(fona.getCallStatus());

  uint8_t t = fona.getNetworkStatus();
  Serial.print(F("Network status: "));
  if (t == 0)
    Serial.println(F("Not registered"));
  if (t == 1)
    Serial.println(F("Registered (home)"));
  if (t == 2)
    Serial.println(F("Not registered (searching)"));
  if (t == 3)
    Serial.println(F("Denied"));
  if (t == 4)
    Serial.println(F("Unknown"));
  if (t == 5)
    Serial.println(F("Registered roaming"));

  Serial.print("RSSI : ");
  Serial.println(fona.getRSSI());
}

void loop()
{
  // char phone[32] = {0};
  // if(fona.incomingCallNumber(phone)){
  //   Serial.println(F("RING!"));
  //   Serial.print(F("Phone Number: "));
  //   Serial.println(phone);
  // }

  // if (fonaSS.available())
  // {
  //   Serial.println(fonaSS.readString());
  // }
  // if (Serial.available())
  // {
  //   fonaSS.println(Serial.readString());
  // }
}