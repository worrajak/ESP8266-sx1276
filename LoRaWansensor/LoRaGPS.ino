#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>
#include <TinyGPS.h>
#include <Wire.h> 
#include <SoftwareSerial.h>
TinyGPS gps;
SoftwareSerial gpsNSS(3, 4);
static void gpsdump(TinyGPS &gps);
static bool feedgps();
/*
#include <SimpleDHT.h>
int pinDHT22 = 2;
SimpleDHT22 dht22;    // what digital pin we're connected to d2
int err = SimpleDHTErrSuccess;
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321

#include "DHT.h"
#define DHTPIN 2     // what digital pin we're connected to
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
DHT dht(DHTPIN, DHTTYPE);
*/
byte buf[10];
byte data_temp[4];
byte data_humid[4];

// LoRaWAN NwkSKey, network session key
// This is the default Semtech key, which is used by the prototype TTN
// network initially.
static const PROGMEM u1_t NWKSKEY[16] = { 0x00, 0x3C, 0xA6, 0x47, 0xE0, 0x73, 0x79, 0x80, 0x96, 0x5C, 0x7C, 0x08, 0x32, 0x06, 0x47, 0xAD };

// LoRaWAN AppSKey, application session key
// This is the default Semtech key, which is used by the prototype TTN
// network initially.
static const u1_t PROGMEM APPSKEY[16] = { 0x00, 0x02, 0x4A, 0x50, 0x04, 0x14, 0x2A, 0x81, 0x16, 0x22, 0x16, 0xA4, 0xAE, 0x87, 0xB0, 0xB3 };

// LoRaWAN end-device address (DevAddr)
// See http://thethingsnetwork.org/wiki/AddressSpace
static const u4_t DEVADDR = 0x00000000 ; // <-- Change this address for every node!

// These callbacks are only used in over-the-air activation, so they are
// left empty here (we cannot leave them out completely unless
// DISABLE_JOIN is set in config.h, otherwise the linker will complain).
void os_getArtEui (u1_t* buf) { }
void os_getDevEui (u1_t* buf) { }
void os_getDevKey (u1_t* buf) { }

static char mydata[27];

int i = 27; 
/*
 * temperature ch1,id,2 -> 4
 * humidity ch1,id,1 -> 3
 * analog ch1,id,2 -> 4
 * analog ch2,id,2 -> 4
 * gps ch1,id,9 -> 11
 * +1 = 27 
 */

static osjob_t sendjob;
unsigned long starttime,starttime_DHT;

const unsigned TX_INTERVAL = 120;
unsigned long sampletime_ms = 30000;  // 30 Seconds
unsigned long sampletime_DHT = 20000;  
int TX_COMPLETE = 0;

  int8_t code_digital_in = 0x00;
  int8_t code_digital_out = 0x01;  
  int8_t code_analog_in = 0x02;
  int8_t code_analog_out = 0x03;  
  int8_t code_temp = 0x67;
  int8_t code_humid = 0x68;
  int8_t code_gps = 0x88;

  int8_t ch0=0x00,ch1=0x01,ch2=0x02,ch3=0x03,ch4=0x04,ch5=0x05,ch6=0x06,ch7=0x07,ch8=0x08,ch9=0x09;

  int sensorPin0 = A0;
  int sensorPin1 = A1;
  float sensorValue0 = 0.0;int sensorValue0_int = 0;
  float sensorValue1 = 0.0;int sensorValue1_int = 0;

  uint16_t data[6];
  
  float temperature = 0;
  float humidity = 0;
  uint16_t tempC_int = 0;
  uint8_t hum_int = 0;
  uint16_t adc0 = 0;
  uint16_t adc1 = 0;

  int32_t lat32;
  int32_t lon32;
  int32_t alt32;
  byte gpsb[10];

  float lat=0.0, lon=0.0, alt=0.0;
  unsigned long age=0;

  int channel = 0;
  int dr = DR_SF12;  
  
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
        case EV_TXCOMPLETE:
            Serial.println(F("EV_TXCOMPLETE (includes waiting for RX windows)"));
              TX_COMPLETE = 0;
            if(LMIC.dataLen) {
                // data received in rx slot after tx
                Serial.print(F("Data Received: "));
                Serial.write(LMIC.frame+LMIC.dataBeg, LMIC.dataLen);
                Serial.println();
            }
            // Schedule next transmission
            os_setTimedCallback(&sendjob, os_getTime()+sec2osticks(TX_INTERVAL), do_send);
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

void forceTxSingleChannelDr() {
    for(int i=0; i<9; i++) { // For EU; for US use i<71
        if(i != channel) {
            LMIC_disableChannel(i);
        }
    }
    // Set data rate (SF) and transmit power for uplink
    LMIC_setDrTxpow(dr, 14);
}

void setup() {
  byte i;
  uint8_t j,a, result;
  float x;
   
    Serial.begin(115200);
    gpsNSS.begin(9600);  //38400
    
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
    LMIC_setupChannel(0, 923200000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
//    LMIC_setupChannel(1, 923400000, DR_RANGE_MAP(DR_SF12, DR_SF7B), BAND_CENTI);      // g-band
//    LMIC_setupChannel(2, 923600000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
//    LMIC_setupChannel(3, 923800000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band    
//    LMIC_setupChannel(4, 924000000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band    
//    LMIC_setupChannel(5, 924200000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band 
//    LMIC_setupChannel(6, 924400000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band          
//    LMIC_setupChannel(7, 924600000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band    
    // TTN defines an additional channel at 869.525Mhz using SF9 for class B
    // devices' ping slots. LMIC does not have an easy way to define set this
    // frequency and support for class B is spotty and untested, so this
    // frequency is not configured here.

    // Disable link check validation
    LMIC_setLinkCheckMode(0);

    //LMIC.dn2Dr = DR_SF9;
    // Set data rate and transmit power (note: txpow seems to be ignored by the library)

    forceTxSingleChannelDr();    
    //LMIC_setDrTxpow(DR_SF12,14);

    starttime = millis();
}

void loop() {
  bool newdata = false;
  unsigned long start = millis();

  while (millis() - start < 2000){
    if (feedgps())
      newdata = true;
  }
      gpsdump(gps);
/*
  if ((millis() - starttime_DHT) > sampletime_DHT) {
    starttime_DHT = millis();
  }  
*/
    sensorValue0 = analogRead(sensorPin0)*0.0048;
    sensorValue1 = analogRead(sensorPin1)*0.0048; 
      delay(2000);

 if ((millis() - starttime) > sampletime_ms) {
/*
if ((err = dht22.read2(pinDHT22, &temperature, &humidity, NULL)) != SimpleDHTErrSuccess) {
      Serial.print("Read DHT22 failed, err="); Serial.println(err);delay(2000);
      return;
    } 
*/
    tempC_int = temperature*10;
    hum_int = humidity*2; 
    data_temp[0] = tempC_int >> 8;
    data_temp[1] = tempC_int;
    data_humid[0] =  hum_int ;
   
    int index = 0;
    memcpy(&mydata[index], (const void*)&ch1, sizeof(int8_t));
    index += sizeof(int8_t);
    memcpy(&mydata[index], (const void*)&code_temp, sizeof(int8_t));
    index += sizeof(int8_t);
    memcpy(&mydata[index], (const void*)&data_temp, sizeof(uint16_t));
    index += sizeof(uint16_t);

    memcpy(&mydata[index], (const void*)&ch2, sizeof(int8_t));
    index += sizeof(int8_t);
    memcpy(&mydata[index], (const void*)&code_humid, sizeof(int8_t));
    index += sizeof(int8_t);
    memcpy(&mydata[index], (const void*)&data_humid, sizeof(int8_t));
    index += sizeof(int8_t);

    sensorValue0_int = sensorValue0*100;
    data_temp[0] = sensorValue0_int >> 8;
    data_temp[1] = sensorValue0_int;

    memcpy(&mydata[index], (const void*)&ch3, sizeof(int8_t));
    index += sizeof(int8_t);   
    memcpy(&mydata[index], (const void*)&code_analog_in, sizeof(int8_t));
    index += sizeof(int8_t);
    memcpy(&mydata[index], (const void*)&data_temp, sizeof(uint16_t));
    index += sizeof(uint16_t);

    sensorValue1_int = sensorValue1*100;
    data_temp[0] = sensorValue1_int >> 8;
    data_temp[1] = sensorValue1_int;
    
    memcpy(&mydata[index], (const void*)&ch4, sizeof(int8_t));
    index += sizeof(int8_t);   
    memcpy(&mydata[index], (const void*)&code_analog_in, sizeof(int8_t));
    index += sizeof(int8_t);
    memcpy(&mydata[index], (const void*)data_temp, sizeof(uint16_t));
    index += sizeof(uint16_t); 

    lat32 = lat;
    lon32 = lon;
    alt32 = alt;

    gpsb[0]=lat32 >> 16;
    gpsb[1]=lat32 >> 8;
    gpsb[2]=lat32;

    gpsb[3]=lon32 >> 16;
    gpsb[4]=lon32 >> 8;
    gpsb[5]=lon32;

    gpsb[6]=alt32 >> 16;
    gpsb[7]=alt32 >> 8;
    gpsb[8]=alt32;
  
    memcpy(&mydata[index], (const void*)&ch5, sizeof(int8_t));
    index += sizeof(int8_t);   
    memcpy(&mydata[index], (const void*)&code_gps, sizeof(uint8_t));
    index += sizeof(uint8_t);
    memcpy(&mydata[index], (const void*)&gpsb, sizeof(uint64_t));
    index += sizeof(uint64_t);     

    LMIC_setTxData2(i, mydata, sizeof(mydata)-1, 0);

    starttime = millis();
 }
    os_runloop_once();
}

static void gpsdump(TinyGPS &gps)
{
  float flat, flon;
  unsigned long age, date, time, chars = 0;
  unsigned short sentences = 0, failed = 0;
  
  gps.f_get_position(&flat, &flon, &age);
  lat = flat * 10000;
  lon = flon * 10000;
  alt = gps.f_altitude() * 100;
}

static bool feedgps()
{
  while (gpsNSS.available())
  {
    if (gps.encode(gpsNSS.read()))
      return true;
  }
  return false;
}
