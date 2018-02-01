#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>
#include <Wire.h> 
#include <SoftwareSerial.h>

byte buf[10];
byte data_temp[4];
byte data_humid[4];
#include <SimpleDHT.h>

int pinDHT22 = 2;
SimpleDHT22 dht22;    // what digital pin we're connected to d2
int err = SimpleDHTErrSuccess;
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321

// LoRaWAN NwkSKey, network session key
// This is the default Semtech key, which is used by the prototype TTN
// network initially.
static const PROGMEM u1_t NWKSKEY[16] = { 0x00, 0xA3, 0xAB, 0x6A, 0x57, 0x73, 0x4F, 0xE6, 0xCF, 0xB9, 0xA2, 0xA4, 0x08, 0xC2, 0x63, 0xF2 };

// LoRaWAN AppSKey, application session key
// This is the default Semtech key, which is used by the prototype TTN
// network initially.
static const u1_t PROGMEM APPSKEY[16] = { 0x00, 0x21, 0x8B, 0xFE, 0xD3, 0x8E, 0x62, 0x8E, 0xC5, 0xD7, 0x0B, 0xDD, 0x0B, 0x43, 0x83, 0xF9 };

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
unsigned long starttime;

// Schedule TX every this many seconds (might become longer due to duty
// cycle limitations).

const unsigned TX_INTERVAL = 120;
unsigned long sampletime_ms = 20000;  // 30 Seconds
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

  byte present = 0;
  byte type_s;
  byte addr[8];
  float celsius, fahrenheit;
  
  uint16_t tempC_int = 0;
  uint8_t hum_int = 0;
  uint16_t adc0 = 0;
  uint16_t adc1 = 0;

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

void setup() {
  byte i;
  uint8_t j,a, result;
  float x;
  
   Serial.begin(115200);
    delay(50);
   Serial.println(F("Starting"));
       
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

    // Set data rate and transmit power (note: txpow seems to be ignored by the library)
    LMIC_setDrTxpow(DR_SF12,14);

   starttime = millis();
}

void loop() {

  bool newdata = false;
  unsigned long start = millis();
  
    sensorValue0 = analogRead(sensorPin0)*0.0048;
    sensorValue1 = analogRead(sensorPin1)*0.0048;

      delay(2000);

 if ((millis() - starttime) > sampletime_ms) {
   if ((err = dht22.read2(pinDHT22, &temperature, &humidity, NULL)) != SimpleDHTErrSuccess) {
      Serial.print("Read DHT22 failed, err="); Serial.println(err);delay(2000);
      return;
    } 
   Serial.print("Temp ");Serial.print(temperature,1);Serial.print("C %RH ");Serial.println(humidity,1);

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

    sensorValue0_int = (sensorValue0*100)*5.466;
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

    LMIC_setTxData2(i, mydata, sizeof(mydata)-1, 0);

    starttime = millis();
 }

      os_runloop_once();
}


