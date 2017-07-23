/*******************************************************************************
 * Copyright (c) 2015 Thomas Telkamp and Matthijs Kooijman
 *
 * Permission is hereby granted, free of charge, to anyone
 * obtaining a copy of this document and accompanying files,
 * to do whatever they want with them without any restriction,
 * including, but not limited to, copying, modification and redistribution.
 * NO WARRANTY OF ANY KIND IS PROVIDED.
 *
 * This example sends a valid LoRaWAN packet with payload "Hello,
 * world!", using frequency and encryption settings matching those of
 * the (early prototype version of) The Things Network.
 *
 * Note: LoRaWAN per sub-band duty-cycle limitation is enforced (1% in g1,
 *  0.1% in g2).
 *
 * Change DEVADDR to a unique address!
 * See http://thethingsnetwork.org/wiki/AddressSpace
 *
 * Do not forget to define the radio type correctly in config.h.
 *
 *******************************************************************************/

#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>

#define debugSerial Serial

#include <ModbusMaster.h>

#include <SoftwareSerial.h>

SoftwareSerial mySerial(3, 4); // RX, TX
/*!
  We're using a MAX485-compatible RS485 Transceiver.
  Rx/Tx is hooked up to the hardware serial port at 'Serial'.
  The Data Enable and Receiver Enable pins are hooked up as follows:
*/
#define MAX485_DE      6
#define MAX485_RE_NEG  7

// instantiate ModbusMaster object
ModbusMaster node;

bool state = true;

void preTransmission()
{
  digitalWrite(MAX485_RE_NEG, 1);
  digitalWrite(MAX485_DE, 1);
}

void postTransmission()
{
  digitalWrite(MAX485_RE_NEG, 0);
  digitalWrite(MAX485_DE, 0);
}

byte buf[31];
#include "EnergyReading.h"
#include <SimpleDHT.h>

int pinDHT22 = 2;
SimpleDHT22 dht22;    // what digital pin we're connected to d2

// Uncomment whatever type you're using!
//#define DHTTYPE DHT11   // DHT 11
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321

// LoRaWAN NwkSKey, network session key
// This is the default Semtech key, which is used by the prototype TTN
// network initially.
static const PROGMEM u1_t NWKSKEY[16] = { 0x00, 0x38, 0xEA, 0xBF, 0x40, 0xCA, 0x3D, 0x5B, 0x05, 0x12, 0xBD, 0x08, 0x6F, 0xC6, 0x55, 0xA4 };

// LoRaWAN AppSKey, application session key
// This is the default Semtech key, which is used by the prototype TTN
// network initially.
static const u1_t PROGMEM APPSKEY[16] = { 0x00, 0xBC, 0xA7, 0xFF, 0x57, 0x52, 0x75, 0x5B, 0x10, 0xC0, 0x4D, 0xCA, 0x3C, 0xB4, 0xDE, 0x2F };

// LoRaWAN end-device address (DevAddr)
// See http://thethingsnetwork.org/wiki/AddressSpace
static const u4_t DEVADDR = 0x26041B00 ; // <-- Change this address for every node!

// These callbacks are only used in over-the-air activation, so they are
// left empty here (we cannot leave them out completely unless
// DISABLE_JOIN is set in config.h, otherwise the linker will complain).
void os_getArtEui (u1_t* buf) { }
void os_getDevEui (u1_t* buf) { }
void os_getDevKey (u1_t* buf) { }

static char mydata[20];

static osjob_t sendjob;

// Schedule TX every this many seconds (might become longer due to duty
// cycle limitations).
const unsigned TX_INTERVAL = 60;

  int8_t send_code = 0x88;
  float temperature = 0;
  float humidity = 0;

  uint16_t tempC_int = 0;
  uint16_t hum_int = 0;
  uint16_t volt = 0;
  uint16_t amp = 0;
  uint16_t watt = 0;
  uint16_t kwh = 0;
  
  
// Pin mapping
const lmic_pinmap lmic_pins = {
    .nss = 10,
    .rxtx = LMIC_UNUSED_PIN,
    .rst = 5,
    .dio = {9, 8, 0},
};
void onEvent (ev_t ev) {
    Serial.print(os_getTime());
    Serial.print(": ");
    switch(ev) {
        case EV_SCAN_TIMEOUT:
            Serial.println(F("EV_SCAN_TIMEOUT"));
            break;
        case EV_BEACON_FOUND:
            Serial.println(F("EV_BEACON_FOUND"));
            break;
        case EV_BEACON_MISSED:
            Serial.println(F("EV_BEACON_MISSED"));
            break;
        case EV_BEACON_TRACKED:
            Serial.println(F("EV_BEACON_TRACKED"));
            break;
        case EV_JOINING:
            Serial.println(F("EV_JOINING"));
            break;
        case EV_JOINED:
            Serial.println(F("EV_JOINED"));
            break;
        case EV_RFU1:
            Serial.println(F("EV_RFU1"));
            break;
        case EV_JOIN_FAILED:
            Serial.println(F("EV_JOIN_FAILED"));
            break;
        case EV_REJOIN_FAILED:
            Serial.println(F("EV_REJOIN_FAILED"));
            break;
            break;
        case EV_TXCOMPLETE:
            Serial.println(F("EV_TXCOMPLETE (includes waiting for RX windows)"));
            if(LMIC.dataLen) {
                // data received in rx slot after tx
                Serial.print(F("Data Received: "));
                Serial.write(LMIC.frame+LMIC.dataBeg, LMIC.dataLen);
                Serial.println();
            }
            // Schedule next transmission
            os_setTimedCallback(&sendjob, os_getTime()+sec2osticks(TX_INTERVAL), do_send);
            break;
        case EV_LOST_TSYNC:
            Serial.println(F("EV_LOST_TSYNC"));
            break;
        case EV_RESET:
            Serial.println(F("EV_RESET"));
            break;
        case EV_RXCOMPLETE:
            // data received in ping slot
            Serial.println(F("EV_RXCOMPLETE"));
            break;
        case EV_LINK_DEAD:
            Serial.println(F("EV_LINK_DEAD"));
            break;
        case EV_LINK_ALIVE:
            Serial.println(F("EV_LINK_ALIVE"));
            break;
         default:
            Serial.println(F("Unknown event"));
            break;
    }
}

void do_send(osjob_t* j){
    // Check if there is not a current TX/RX job running
    if (LMIC.opmode & OP_TXRXPEND) {
        Serial.println(F("OP_TXRXPEND, not sending"));
    } else {
        // Prepare upstream data transmission at the next possible time.
        LMIC_setTxData2(1, mydata, sizeof(mydata)-1, 0);
        Serial.println(F("Packet queued"));
    }
    // Next TX is scheduled after TX_COMPLETE event.
}

void setup() {
  uint8_t j,a, result;
  uint16_t data[6];
  float x;
  
  Serial.begin(115200);

 //Start Modbus   
    pinMode(MAX485_RE_NEG, OUTPUT);
    pinMode(MAX485_DE, OUTPUT);
  // Init in receive mode
    digitalWrite(MAX485_RE_NEG, 0);
    digitalWrite(MAX485_DE, 0);
    
  int err = SimpleDHTErrSuccess;
  if ((err = dht22.read2(pinDHT22, &temperature, &humidity, NULL)) != SimpleDHTErrSuccess) {
    Serial.print("Read DHT22 failed, err="); Serial.println(err);delay(2000);
    return;
  } 
  Serial.print("DHT OK: ");
    mySerial.begin(2400);

      
    Serial.println(F("Starting"));

    #ifdef VCC_ENABLE
    // For Pinoccio Scout boards
    pinMode(VCC_ENABLE, OUTPUT);
    digitalWrite(VCC_ENABLE, HIGH);
    delay(1000);
    #endif

    // LMIC init
    os_init();
    // Reset the MAC state. Session and pending data transfers will be discarded.
    LMIC_reset();

    // Set static session parameters. Instead of dynamically establishing a session
    // by joining the network, precomputed session parameters are be provided.
    #ifdef PROGMEM
    // On AVR, these values are stored in flash and only copied to RAM
    // once. Copy them to a temporary buffer here, LMIC_setSession will
    // copy them into a buffer of its own again.
    uint8_t appskey[sizeof(APPSKEY)];
    uint8_t nwkskey[sizeof(NWKSKEY)];
    memcpy_P(appskey, APPSKEY, sizeof(APPSKEY));
    memcpy_P(nwkskey, NWKSKEY, sizeof(NWKSKEY));
    LMIC_setSession (0x1, DEVADDR, nwkskey, appskey);
    #else
    // If not running an AVR with PROGMEM, just use the arrays directly 
    LMIC_setSession (0x1, DEVADDR, NWKSKEY, APPSKEY);
    #endif

    // Set up the channels used by the Things Network, which corresponds
    // to the defaults of most gateways. Without this, only three base
    // channels from the LoRaWAN specification are used, which certainly
    // works, so it is good for debugging, but can overload those
    // frequencies, so be sure to configure the full frequency range of
    // your network here (unless your network autoconfigures them).
    // Setting up channels should happen after LMIC_setSession, as that
    // configures the minimal channel set.
    LMIC_setupChannel(0, 903900000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
    LMIC_setupChannel(1, 904100000, DR_RANGE_MAP(DR_SF12, DR_SF7B), BAND_CENTI);      // g-band
    LMIC_setupChannel(2, 904300000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
    // TTN defines an additional channel at 869.525Mhz using SF9 for class B
    // devices' ping slots. LMIC does not have an easy way to define set this
    // frequency and support for class B is spotty and untested, so this
    // frequency is not configured here.

    // Disable link check validation
    LMIC_setLinkCheckMode(0);

    // Set data rate and transmit power (note: txpow seems to be ignored by the library)
    LMIC_setDrTxpow(DR_SF7,14);

    // Modbus slave ID 1
    node.begin(1, mySerial);
  // Callbacks allow us to configure the RS485 transceiver correctly
    node.preTransmission(preTransmission);
    node.postTransmission(postTransmission);
    
   result = node.readInputRegisters(0x0000, 14);
  if (result == node.ku8MBSuccess)
  {
    ((byte*)&x)[3]= node.getResponseBuffer(0x00)>> 8;
    ((byte*)&x)[2]= node.getResponseBuffer(0x00)& 0xff;
    ((byte*)&x)[1]= node.getResponseBuffer(0x01)>> 8;
    ((byte*)&x)[0]= node.getResponseBuffer(0x01)& 0xff;
    volt = x*100;
    Serial.println(volt);
    ((byte*)&x)[3]= node.getResponseBuffer(0x06)>> 8;
    ((byte*)&x)[2]= node.getResponseBuffer(0x06)& 0xff;
    ((byte*)&x)[1]= node.getResponseBuffer(0x07)>> 8;
    ((byte*)&x)[0]= node.getResponseBuffer(0x07)& 0xff;
    amp = x*100;
    Serial.println(amp);
    ((byte*)&x)[3]= node.getResponseBuffer(0x0C)>> 8;
    ((byte*)&x)[2]= node.getResponseBuffer(0x0C)& 0xff;
    ((byte*)&x)[1]= node.getResponseBuffer(0x0D)>> 8;
    ((byte*)&x)[0]= node.getResponseBuffer(0x0D)& 0xff;
    watt = x*100;
    Serial.println(watt);
  }

  result = node.readInputRegisters(0x0156, 2);
  if (result == node.ku8MBSuccess)
  {
    ((byte*)&x)[3]= node.getResponseBuffer(0x00)>> 8;
    ((byte*)&x)[2]= node.getResponseBuffer(0x00)& 0xff;
    ((byte*)&x)[1]= node.getResponseBuffer(0x01)>> 8;
    ((byte*)&x)[0]= node.getResponseBuffer(0x01)& 0xff;
    kwh = x*100; 
    Serial.print(x,2);
  }

    tempC_int = temperature*100;
    hum_int = humidity*100;
  
    int index = 0;
    memcpy(&mydata[index], (const void*)&send_code, sizeof(int8_t));
    index += sizeof(int8_t);
    memcpy(&mydata[index], (const void*)&tempC_int, sizeof(uint16_t));
    index += sizeof(uint16_t);
    memcpy(&mydata[index], (const void*)&hum_int, sizeof(uint16_t));
    index += sizeof(uint16_t);
     memcpy(&mydata[index], (const void*)&volt, sizeof(uint16_t));
    index += sizeof(uint16_t);
     memcpy(&mydata[index], (const void*)&amp, sizeof(uint16_t));
    index += sizeof(uint16_t);
     memcpy(&mydata[index], (const void*)&watt, sizeof(uint16_t));
    index += sizeof(uint16_t);         
    memcpy(&mydata[index], (const void*)&kwh, sizeof(uint16_t));
    index += sizeof(uint16_t);
    do_send(&sendjob);
}

void loop() {
  os_runloop_once();
}
