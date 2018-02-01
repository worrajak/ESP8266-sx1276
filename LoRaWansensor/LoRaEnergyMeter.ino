#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>
#include <ModbusMaster.h>
#include <SoftwareSerial.h>
SoftwareSerial mySerial(3, 4); // RX, TX

#define MAX485_DE      6
#define MAX485_RE_NEG  6

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
byte data_temp[4];
byte data_humid[4];

/*
#include <SimpleDHT.h>
int pinDHT22 = 2;
SimpleDHT22 dht22;    // what digital pin we're connected to d2
int err = SimpleDHTErrSuccess;
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321

#include <DHT.h>
#define DHTPIN 2
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);
*/
// LoRaWAN NwkSKey, network session key
// This is the default Semtech key, which is used by the prototype TTN
// network initially.
static const PROGMEM u1_t NWKSKEY[16] = { 0x00, 0x00, 0x89, 0x0C, 0x6F, 0xEC, 0xF7, 0xB4, 0x25, 0x55, 0x44, 0xBC, 0xE2, 0x52, 0x15, 0xAE };

// LoRaWAN AppSKey, application session key
// This is the default Semtech key, which is used by the prototype TTN
// network initially.
static const u1_t PROGMEM APPSKEY[16] = { 0x00, 0x9D, 0x3C, 0xD7, 0xA5, 0xF4, 0xA8, 0x88, 0xD9, 0xD6, 0x78, 0xA6, 0xBC, 0x62, 0x85, 0x8B };

// LoRaWAN end-device address (DevAddr)
// See http://thethingsnetwork.org/wiki/AddressSpace
static const u4_t DEVADDR = 0x00000000 ; // <-- Change this address for every node!

// These callbacks are only used in over-the-air activation, so they are
// left empty here (we cannot leave them out completely unless
// DISABLE_JOIN is set in config.h, otherwise the linker will complain).
void os_getArtEui (u1_t* buf) { }
void os_getDevEui (u1_t* buf) { }
void os_getDevKey (u1_t* buf) { }

static char mydata[33];

int i = 33; 
/*
 * pf ch1,id,2 -> 4
 * f ch2,id,2 -> 4
 * analog ch3,id,2 -> 4
 * analog ch4,id,2 -> 4
 * volt ch5,id,2 -> 4
 * amp ch6,id,2 -> 4 
 * watt ch7,id,2 -> 4
 * kwh ch8,id,2 -> 4
 * +1 = 33 
 */

static osjob_t sendjob;
unsigned long starttime,starttime_meter;

// Schedule TX every this many seconds (might become longer due to duty
// cycle limitations).
const unsigned TX_INTERVAL = 120;
unsigned long sampletime_ms = 30000;  
unsigned long sampletime_meter = 20000;  

  int8_t code_digital_in = 0x00;
  int8_t code_digital_out = 0x01;  
  int8_t code_analog_in = 0x02;
  int8_t code_analog_out = 0x03;  
  int8_t code_temp = 0x67;
  int8_t code_humid = 0x68;

 int8_t ch0=0x00,ch1=0x01,ch2=0x02,ch3=0x03,ch4=0x04,ch5=0x05,ch6=0x06,ch7=0x07,ch8=0x08,ch9=0x09,ch10=0x0a,ch11=0x0b,ch12=0x0c;

  int sensorPin0 = A0;
  int sensorPin1 = A1;
  float sensorValue0 = 0.0;int sensorValue0_int = 0;
  float sensorValue1 = 0.0;int sensorValue1_int = 0;
  
  float temperature = 0;
  float humidity = 0;
  uint16_t tempC_int = 0;
  uint16_t hum_int = 0;
  uint16_t adc0 = 0;
  uint16_t adc1 = 0;

  uint16_t volt = 0;
  uint16_t amp = 0;
  uint16_t watt = 0;
  uint16_t kwh = 0;
  uint16_t freq = 0;
  uint16_t power_f = 0;    
  uint8_t  watt_flag=1;

  float x,V,A,W,Wh,F,PF = 0;  

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

float reform_uint16_2_float32(uint16_t u1, uint16_t u2)
{  
  uint32_t num = ((uint32_t)u1 & 0xFFFF) << 16 | ((uint32_t)u2 & 0xFFFF);
    float numf;
    memcpy(&numf, &num, 4);
    return numf;
}

float getRTU(uint16_t m_startAddress){
  uint8_t m_length =2;
  uint16_t result;
  float x;

  node.preTransmission(preTransmission);
  node.postTransmission(postTransmission);
  result = node.readInputRegisters(m_startAddress, m_length);
  if (result == node.ku8MBSuccess) {
     return reform_uint16_2_float32(node.getResponseBuffer(0),node.getResponseBuffer(1));
  }
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
  uint8_t j,a, result;
  uint16_t data[6];
  float x;
  Serial.begin(115200);
    delay(50);

 //Start Modbus   
    pinMode(MAX485_RE_NEG, OUTPUT);
    pinMode(MAX485_DE, OUTPUT);
  // Init in receive mode
    digitalWrite(MAX485_RE_NEG, 0);
    digitalWrite(MAX485_DE, 0);
 
    mySerial.begin(2400);

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
    //LMIC_setupChannel(1, 923300000, DR_RANGE_MAP(DR_SF12, DR_SF7B), BAND_CENTI);      // g-band
    //LMIC_setupChannel(2, 923500000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
    // TTN defines an additional channel at 869.525Mhz using SF9 for class B
    // devices' ping slots. LMIC does not have an easy way to define set this
    // frequency and support for class B is spotty and untested, so this
    // frequency is not configured here.

    // Disable link check validation
    LMIC_setLinkCheckMode(0);
    
    LMIC.dn2Dr = DR_SF9;
    // Set data rate and transmit power (note: txpow seems to be ignored by the library)

    forceTxSingleChannelDr();    
    //LMIC_setDrTxpow(DR_SF12,14);

    // Modbus slave ID 1
    node.begin(1, mySerial);
  // Callbacks allow us to configure the RS485 transceiver correctly
 
    starttime = millis();
    Serial.println(F("Starting"));
}

void loop() {
  uint8_t j,a, result;

  unsigned long start = millis();

//  if ((millis() - starttime_meter) > sampletime_meter) {
    V = getRTU(0x0000); 
    delay(100);
    A = getRTU(0x0006);
    delay(100); 
    W = getRTU(0x000C);
    delay(100);      
    Wh = getRTU(0x0156);
    delay(100); 
    PF = getRTU(0x001E);
    delay(100);
    F = getRTU(0x0046);
    delay(100);        
    sensorValue0 = analogRead(sensorPin0)*0.0048;
    sensorValue1 = analogRead(sensorPin1)*0.0048;
    delay(100); 
//    starttime_meter = millis();
//  }
    
if ((millis() - starttime) > sampletime_ms) {
    int index = 0;

    power_f = PF*100;
    data_temp[0] = power_f >> 8;
    data_temp[1] = power_f;

    memcpy(&mydata[index], (const void*)&ch1, sizeof(int8_t));
    index += sizeof(int8_t);   
    memcpy(&mydata[index], (const void*)&code_analog_in, sizeof(int8_t));
    index += sizeof(int8_t);              
    memcpy(&mydata[index], (const void*)&data_temp, sizeof(uint16_t));
    index += sizeof(uint16_t);

    freq = F*100;
    data_temp[0] = freq >> 8;
    data_temp[1] = freq;

    memcpy(&mydata[index], (const void*)&ch2, sizeof(int8_t));
    index += sizeof(int8_t);   
    memcpy(&mydata[index], (const void*)&code_analog_in, sizeof(int8_t));
    index += sizeof(int8_t);              
    memcpy(&mydata[index], (const void*)&data_temp, sizeof(uint16_t));
    index += sizeof(uint16_t);
    
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

    volt = V*100;
    data_temp[0] = volt >> 8;
    data_temp[1] = volt;
    
    memcpy(&mydata[index], (const void*)&ch5, sizeof(int8_t));
    index += sizeof(int8_t);   
    memcpy(&mydata[index], (const void*)&code_analog_in, sizeof(int8_t));
    index += sizeof(int8_t);
    memcpy(&mydata[index], (const void*)&data_temp, sizeof(uint16_t));
    index += sizeof(uint16_t);

    amp = A*100;
    data_temp[0] = amp >> 8;
    data_temp[1] = amp;

    memcpy(&mydata[index], (const void*)&ch6, sizeof(int8_t));
    index += sizeof(int8_t);   
    memcpy(&mydata[index], (const void*)&code_analog_in, sizeof(int8_t));
    index += sizeof(int8_t);
    memcpy(&mydata[index], (const void*)&data_temp, sizeof(uint16_t));
    index += sizeof(uint16_t);

    watt = W*100;
    data_temp[0] = watt >> 8;
    data_temp[1] = watt;

    memcpy(&mydata[index], (const void*)&ch7, sizeof(int8_t));
    index += sizeof(int8_t);   
    memcpy(&mydata[index], (const void*)&code_analog_in, sizeof(int8_t));
    index += sizeof(int8_t);    
    memcpy(&mydata[index], (const void*)&data_temp, sizeof(uint16_t));
    index += sizeof(uint16_t);

    kwh = Wh*100;
    data_temp[0] = kwh >> 8;
    data_temp[1] = kwh;

    memcpy(&mydata[index], (const void*)&ch8, sizeof(int8_t));
    index += sizeof(int8_t);   
    memcpy(&mydata[index], (const void*)&code_analog_in, sizeof(int8_t));
    index += sizeof(int8_t);              
    memcpy(&mydata[index], (const void*)&data_temp, sizeof(uint16_t));
    index += sizeof(uint16_t);

    LMIC_setTxData2(i, mydata, sizeof(mydata)-1, 0);

    starttime = millis();
 }
  os_runloop_once();
}
