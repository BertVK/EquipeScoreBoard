#include <Arduino.h>
#include <SoftwareSerial.h>

// Please select the corresponding model
// #define SIM800L_IP5306_VERSION_20190610
// #define SIM800L_AXP192_VERSION_20200327
// #define SIM800C_AXP192_VERSION_20200609
#define SIM800L_IP5306_VERSION_20200811
#define SIM800L_STANDALONE

// Define the serial console for debug prints, if needed
#define DUMP_AT_COMMANDS
#define TINY_GSM_DEBUG          Serial

#include "utilities.h"

// Set serial for AT commands (to the module)
SoftwareSerial SerialAT = SoftwareSerial(2,3);

// Configure TinyGSM library
#define TINY_GSM_MODEM_SIM800          // Modem is SIM800
#define TINY_GSM_RX_BUFFER      1024   // Set RX buffer to 1Kb

#include <TinyGsmClient.h>

#ifdef DUMP_AT_COMMANDS
#include <StreamDebugger.h>
StreamDebugger debugger(SerialAT, Serial);
TinyGsm modem(debugger);
#else
TinyGsm modem(SerialAT);
#endif

// Set phone numbers, if you want to test SMS and Calls
#define SMS_TARGET  "+32478385741"
#define CALL_TARGET "+32478385741"

void setup()
{
    // Set console baud rate
    Serial.begin(115200);

    delay(10);

    // Start power management
    // if (setupPMU() == false) {
    //     Serial.println("Setting power error");
    // }

    // Some start operations
    setupModem();

    // Set GSM module baud rate and UART pins
    // SerialAT.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX);
    SerialAT.begin(9600);

    delay(6000);
}

void loop()
{
    // Restart takes quite some time
    // To skip it, call init() instead of restart()
    Serial.println("Initializing modem...");
    modem.restart();


    delay(10000);

    String imei = modem.getIMEI();
    DBG("IMEI:", imei);

    bool  res = modem.sendSMS(SMS_TARGET, String("Hello from ") + imei);
    DBG("SMS:", res ? "OK" : "fail");

    // Do nothing forevermore
    while (true) {
        modem.maintain();
    }
}