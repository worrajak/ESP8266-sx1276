// 1-channel LoRa Gateway for ESP8266
// Copyright (c) 2016, 2017 Maarten Westenberg version for ESP8266
// Verison 3.3.0
// Date: 2017-01-02
//
//   based on work done by Thomas Telkamp for Raspberry PI 1-ch gateway
//  and many others.
//
// All rights reserved. This program and the accompanying materials
// are made available under the terms of the MIT License
// which accompanies this distribution, and is available at
// https://opensource.org/licenses/mit-license.php
//
// Author: Maarten Westenberg (mw12554@hotmail.com)
//
// The protocols and specifications used for this 1ch gateway: 
// 1. LoRA Specification version V1.0 and V1.1 for Gateway-Node communication
//  
// 2. Semtech Basic communication protocol between Lora gateway and server version 3.0.0
//  https://github.com/Lora-net/packet_forwarder/blob/master/PROTOCOL.TXT
//
// Notes: 
// - Once call gethostbyname() to get IP for services, after that only use IP
//   addresses (too many gethost name makes ESP unstable)
// - Only call yield() in main stream (not for background NTP sync). 
//
// ----------------------------------------------------------------------------------------

//

void Line_Notify(String message) ;

#define LINE_TOKEN "nHOj3o9PXeXhcABjGZ3hsFIbu3dpjGRM9GdjH8n7T4c" //ESPHome 

String message = "Hello world";

#define VERSION " ! V. 3.3.0, 161230"

#include "ESP-sc-gway.h"            // This file contains configuration of GWay

#if WIFIMANAGER>0
#include <WiFiManager.h>            // Library for ESP WiFi config through an AP
#endif

#include <Esp.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstdlib>
#include <sys/time.h>
#include <cstring>
#include <SPI.h>
#include <TimeLib.h>              // http://playground.arduino.cc/code/time
#include <ESP8266WiFi.h>
#include <DNSServer.h>              // Local DNSserver
#include <ESP8266WebServer.h>
#include "FS.h"
#include <WiFiUdp.h>

#if GATEWAYNODE==1
#include "AES-128_V10.h"
#endif

extern "C" {
#include "user_interface.h"
#include "lwip/err.h"
#include "lwip/dns.h"
}
#include <pins_arduino.h>
#include <ArduinoJson.h>
#include <SimpleTimer.h>
#include <gBase64.h>              // https://github.com/adamvr/arduino-base64 (I changed the name)

#include "loraModem.h"

int debug=1;                  // Debug level! 0 is no msgs, 1 normal, 2 is extensive

// You can switch webserver off if not necessary but probably better to leave it in.
#if A_SERVER==1
#include <Streaming.h>                  // http://arduiniana.org/libraries/streaming/
  String webPage;
  ESP8266WebServer server(SERVERPORT);
#endif
using namespace std;

byte currentMode = 0x81;
//uint8_t message[256];             // May not be necessary if we declare these int functions
char b64[256];
bool sx1272 = true;               // Actually we use sx1276/RFM95

uint32_t cp_nb_rx_rcv;
uint32_t cp_nb_rx_ok;
uint32_t cp_nb_rx_bad;
uint32_t cp_nb_rx_nocrc;
uint32_t cp_up_pkt_fwd;

enum sf_t { SF7=7, SF8, SF9, SF10, SF11, SF12 };

uint8_t MAC_array[6];
char MAC_char[19];

/*******************************************************************************
 *
 * Configure these values only if necessary!
 *
 *******************************************************************************/

// SX1276 - ESP8266 connections
int ssPin = 16;                 // GPIO16, D0
int dio0  = 15;                 // GPIO15, D8
int dio1  = 4;                  // GPIO4,  D2 XXX Not necessary in single channel gateway
int dio2  = 0;                  // GPIO3, !! NOT CONNECTED IN THIS VERSION
int RST   = 0;                  // 0, not connected

// Set spreading factor (SF7 - SF12)
sf_t sf       = _SPREADING ;

// Set location, description and other configuration parameters
// Defined in ESP-sc_gway.h
//
float lat     = _LAT;           // Configuration specific info...
float lon     = _LON;
int   alt     = _ALT;
char platform[24] = _PLATFORM;        // platform definition
char email[40]    = _EMAIL;           // used for contact email
char description[64]= _DESCRIPTION;       // used for free form description 

// define servers

IPAddress ntpServer;              // IP address of NTP_TIMESERVER
IPAddress ttnServer;              // IP Address of thethingsnetwork server
IPAddress thingServer;

WiFiUDP Udp;
uint32_t stattime = 0;              // last time we sent a stat message to server
uint32_t pulltime = 0;              // last time we sent a pull_data request to server
uint32_t lastTmst = 0;

SimpleTimer timer;                // Timer is needed for delayed sending

#define TX_BUFF_SIZE  2048            // Upstream buffer to send to MQTT
#define RX_BUFF_SIZE  1024            // Downstream received from MQTT
#define STATUS_SIZE   512           // This should(!) be enough based on the static text part.. was 1024

uint8_t buff_up[TX_BUFF_SIZE];          // buffer to compose the upstream packet to backend server
uint8_t buff_down[RX_BUFF_SIZE];        // Buffer for downstream
uint16_t lastToken = 0x00;

#if GATEWAYNODE==1
uint16_t frameCount=0;              // We write this to SPIFF file
#endif

// ----------------------------------------------------------------------------
// DIE is not use actively in the source code anymore.
// It is replaced by a Serial.print command so we know that we have a problem
// somewhere.
// There are at least 3 other ways to restart the ESP. Pick one if you want.
// ----------------------------------------------------------------------------
void die(const char *s)
{
    Serial.println(s);
  delay(50);
  // system_restart();            // SDK function
  // ESP.reset();       
  abort();                  // Within a second
}

// ----------------------------------------------------------------------------
// gway_failed is a function called by ASSERT.
// ----------------------------------------------------------------------------
void gway_failed(const char *file, uint16_t line) {
  Serial.print(F("Program failed in file: "));
  Serial.print(file);
  Serial.print(F(", line: "));
  Serial.print(line);
}

// ----------------------------------------------------------------------------
// Print leading '0' digits for hours(0) and second(0) when
// printing values less than 10
// ----------------------------------------------------------------------------
void printDigits(int digits)
{
    // utility function for digital clock display: prints preceding colon and leading 0
    if(digits < 10)
        Serial.print(F("0"));
    Serial.print(digits);
}

// ----------------------------------------------------------------------------
// Print utin8_t values in HEX with leading 0 when necessary
// ----------------------------------------------------------------------------
void printHexDigit(uint8_t digit)
{
    // utility function for printing Hex Values with leading 0
    if(digit < 0x10)
        Serial.print('0');
    Serial.print(digit,HEX);
}


// ----------------------------------------------------------------------------
// Print the current time
// ----------------------------------------------------------------------------
void printTime() {
  char *Days [] ={"Sunday","Monday","Tuesday","Wednesday","Thursday","Friday","Saturday"};
  Serial.print(Days[weekday()-1]);
  Serial.print(F(" "));
  printDigits(hour());
  Serial.print(F(":"));
  printDigits(minute());
  Serial.print(F(":"));
  printDigits(second());
  return;
}


// ----------------------------------------------------------------------------
// Convert a float to string for printing
// f is value to convert
// p is precision in decimal digits
// val is character array for results
// ----------------------------------------------------------------------------
void ftoa(float f, char *val, int p) {
  int j=1;
  int ival, fval;
  char b[6];
  
  for (int i=0; i< p; i++) { j= j*10; }

  ival = (int) f;               // Make integer part
  fval = (int) ((f- ival)*j);         // Make fraction. Has same sign as integer part
  if (fval<0) fval = -fval;         // So if it is negative make fraction positive again.
                        // sprintf does NOT fit in memory
  strcat(val,itoa(ival,b,10));
  strcat(val,".");              // decimal point
  
  itoa(fval,b,10);
  for (int i=0; i<(p-strlen(b)); i++) strcat(val,"0");
  // Fraction can be anything from 0 to 10^p , so can have less digits
  strcat(val,b);
}

// =============================================================================
// NTP TIME functions

const int NTP_PACKET_SIZE = 48;         // Fixed size of NTP record
byte packetBuffer[NTP_PACKET_SIZE];

// ----------------------------------------------------------------------------
// Send the request packet to the NTP server.
//
// ----------------------------------------------------------------------------
void sendNTPpacket(IPAddress& timeServerIP) {
  // Zeroise the buffer.
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  packetBuffer[0] = 0b11100011;         // LI, Version, Mode
  packetBuffer[1] = 0;            // Stratum, or type of clock
  packetBuffer[2] = 6;            // Polling Interval
  packetBuffer[3] = 0xEC;           // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52; 

  Udp.beginPacket(timeServerIP, (int) 123); // NTP Server and Port

  if ((Udp.write((char *)packetBuffer, NTP_PACKET_SIZE)) != NTP_PACKET_SIZE) {
    die("sendNtpPacket:: Error write");
  }
  else {
    // Success
  }
  Udp.endPacket();
}


// ----------------------------------------------------------------------------
// Get the NTP time from one of the time servers
// Note: As this function is called from SyncINterval in the background
//  make sure we have no blocking calls in this function
// ----------------------------------------------------------------------------
time_t getNtpTime()
{
  WiFi.hostByName(NTP_TIMESERVER, ntpServer);
  for (int i = 0 ; i < 4 ; i++) {         // 5 retries.
    sendNTPpacket(ntpServer);
    uint32_t beginWait = millis();
    while (millis() - beginWait < 6000) 
  {
      if (Udp.parsePacket()) {
        Udp.read(packetBuffer, NTP_PACKET_SIZE);
        // Extract seconds portion.
        unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
        unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
        unsigned long secSince1900 = highWord << 16 | lowWord;
        Udp.flush();
        return secSince1900 - 2208988800UL + NTP_TIMEZONES * SECS_PER_HOUR;       
    // UTC is 1 TimeZone correction when no daylight saving time
      }
      //delay(10);
    }
  }
  return 0;                   // return 0 if unable to get the time
}

// ----------------------------------------------------------------------------
// Set up regular synchronization of NTP server and the local time.
// ----------------------------------------------------------------------------
void setupTime() {
  setSyncProvider(getNtpTime);
  setSyncInterval(_NTP_INTERVAL);
}



// ============================================================================
// UDP AND WLAN FUNCTIONS

// ----------------------------------------------------------------------------
// GET THE DNS SERVER IP address
// ----------------------------------------------------------------------------
IPAddress getDnsIP() {
  ip_addr_t dns_ip = dns_getserver(0);
  IPAddress dns = IPAddress(dns_ip.addr);
  return((IPAddress) dns);
}

// ----------------------------------------------------------------------------
// config.txt is a text file that contains line(!) with WPA configuration items
// Each line contains an SSID and a Password for an access point
//
// ----------------------------------------------------------------------------
int WlanReadWpa() {

  //const char wpafile[] = "/config.txt";
  //if (!SPIFFS.exists(wpafile)) {
  //  Serial.println("ERROR:: WlanReadWpa, file does not exist");
  //  return(-1);
  //}
  //File f = SPIFFS.open(wpafile, "r");
  //if (!f) {
  //  Serial.println("ERROR:: WlanReadWpa, file open failed");
  //  return(-1);
  //}
  
  readConfig( CONFIGFILE, &gwayConfig);

  if (gwayConfig.sf != (uint8_t) 0) sf = (sf_t) gwayConfig.sf;
  if (gwayConfig.freq != (uint8_t) 0) freq = gwayConfig.freq;
#if GATEWAYNODE == 1
  if (gwayConfig.fcnt != (uint8_t) 0) frameCount = gwayConfig.fcnt;
#endif  
#if WIFIMANAGER > 0
  String ssid=gwayConfig.ssid;
  String pass=gwayConfig.pass;


  char ssidBuf[ssid.length()+1];
  ssid.toCharArray(ssidBuf,ssid.length()+1);
  char passBuf[pass.length()+1];
  pass.toCharArray(passBuf,pass.length()+1);
  Serial.print(F("WlanReadWpa: ")); Serial.print(ssidBuf); Serial.print(F(", ")); Serial.println(passBuf);
  
  strcpy(wpa[0].login, ssidBuf);        // XXX changed from wpa[0][0] = ssidBuf
  strcpy(wpa[0].passw, passBuf);
  
  Serial.print(F("WlanReadWpa: <")); 
  Serial.print(wpa[0].login);         // XXX
  Serial.print(F(">, <")); 
  Serial.print(wpa[0].passw);
  Serial.println(F(">"));
#endif

}

// ----------------------------------------------------------------------------
// Print the WPA data of last WiFiManager to file
// ----------------------------------------------------------------------------
int WlanWriteWpa( char* ssid, char *pass) {

  Serial.print(F("WlanWriteWpa:: ssid=")); Serial.print(ssid);
  Serial.print(F(", pass=")); Serial.print(pass); Serial.println();
  
  // Version 3.3 use of config file
  String s((char *) ssid);
  gwayConfig.ssid = s;
  
  String p((char *) pass);
  gwayConfig.pass = p;
  
  writeConfig( CONFIGFILE, &gwayConfig);
}

// ----------------------------------------------------------------------------
// Function to join the Wifi Network
//  It is a matter of returning to the main loop() asap and make sure in next loop
//  the reconnect is done first thing.
// ----------------------------------------------------------------------------
int WlanConnect() {

  // We start by connecting to a WiFi network
  wifi_station_set_hostname( "espgway" );
#if WIFIMANAGER==1
  WiFiManager wifiManager;
#endif
  unsigned char agains = 0;
  unsigned char wpa_index = (WIFIMANAGER >0 ? 0 : 1); // Skip over first record for WiFiManager
  Serial.print(F("WlanConnect:: wpa_index=")); Serial.println(wpa_index);
  int ledStatus = LOW;

  while (WiFi.status() != WL_CONNECTED)
  {
  // Start with well-known access points in the list
  
  char *ssid    = wpa[wpa_index].login;
  char *password  = wpa[wpa_index].passw;
  
  Serial.print(wpa_index); Serial.print(F(". WiFi connect to: ")); Serial.println(ssid);
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(agains*400);
    agains++;
    digitalWrite(BUILTIN_LED, ledStatus);   // Write LED high/low
    ledStatus = (ledStatus == HIGH) ? LOW : HIGH;
    if (debug>=2) Serial.print(".");
    yield();
    
    // If after 10 times there is still no connection, we probably wait forever
    // So restart the WiFI.begin process!!
    if (agains == 10) {
      agains = 0;
      WiFi.disconnect();
      yield();
      delay(500);
      break;
    }
  }
  wpa_index++;
  //if (wpa_index >= WPASIZE) { break; }
  if (wpa_index >= (sizeof(wpa)/sizeof(wpa[0]))) { break; }
  }
  
  // Still not connected?
  if (WiFi.status() != WL_CONNECTED) {
#if WIFIMANAGER==1
  Serial.println(F("Starting Access Point Mode"));
  Serial.print(F("Connect Wifi to accesspoint: "));
  Serial.print(AP_NAME);
  Serial.print(F(" and connect to IP: 192.168.4.1"));
  Serial.println();
    wifiManager.autoConnect(AP_NAME, AP_PASSWD );
  //wifiManager.startConfigPortal(AP_NAME, AP_PASSWD );
  // At this point, there IS a Wifi Access Point found and connected
  // We must connect to the local SPIFFS storage to store the access point
  //String s = WiFi.SSID();
  //char ssidBuf[s.length()+1];
  //s.toCharArray(ssidBuf,s.length()+1);
  // Now look for the password
  struct station_config sta_conf;
  wifi_station_get_config(&sta_conf);
  
  //WlanWriteWpa(ssidBuf, (char *)sta_conf.password);
  WlanWriteWpa((char *)sta_conf.ssid, (char *)sta_conf.password);
#else
  return(-1);
#endif
  }

  Serial.print(F("WiFi connected. local IP address: ")); 
  Serial.println(WiFi.localIP());
  yield();
  return(0);
}


// ----------------------------------------------------------------------------
// Read DOWN a package from UDP socket, can come from any server
// Messages are received when server responds to gateway requests from LoRa nodes 
// (e.g. JOIN requests etc.) or when server has downstream data.
// We repond only to the server that sent us a message!
// ----------------------------------------------------------------------------
int readUdp(int packetSize, uint8_t * buff_down)
{
  uint8_t protocol;
  uint16_t token;
  uint8_t ident; 
  char LoraBuffer[64];            //buffer to hold packet to send to LoRa node
  
  if (packetSize > RX_BUFF_SIZE) {
  Serial.print(F("readUDP:: ERROR package of size: "));
  Serial.println(packetSize);
  Udp.flush();
  return(-1);
  }
  
  Udp.read(buff_down, packetSize);
  IPAddress remoteIpNo = Udp.remoteIP();
  unsigned int remotePortNo = Udp.remotePort();

  uint8_t * data = buff_down + 4;
  protocol = buff_down[0];
  token = buff_down[2]*256 + buff_down[1];
  ident = buff_down[3];
  
  // now parse the message type from the server (if any)
  switch (ident) {
  case PKT_PUSH_DATA: // 0x00 UP
    if (debug >=1) {
      Serial.print(F("PKT_PUSH_DATA:: size ")); Serial.print(packetSize);
      Serial.print(F(" From ")); Serial.print(remoteIpNo);
      Serial.print(F(", port ")); Serial.print(remotePortNo);
      Serial.print(F(", data: "));
      for (int i=0; i<packetSize; i++) {
        Serial.print(buff_down[i],HEX);
        Serial.print(':');
      }
      Serial.println();
    }
  break;
  case PKT_PUSH_ACK:  // 0x01 DOWN
    if (debug >= 1) {
      Serial.print(F("PKT_PUSH_ACK:: size ")); Serial.print(packetSize);
      Serial.print(F(" From ")); Serial.print(remoteIpNo);
      Serial.print(F(", port ")); Serial.print(remotePortNo);
      Serial.print(F(", token: "));
      Serial.println(token, HEX);
      Serial.println();
    }
  break;
  case PKT_PULL_DATA: // 0x02 UP
    Serial.print(F(" Pull Data"));
    Serial.println();
  break;
  case PKT_PULL_RESP: // 0x03 DOWN
  
    lastTmst = micros();          // Store the tmst this package was received
    // Send to the LoRa Node first (timing) and then do messaging
    if (sendPacket(data, packetSize-4) < 0) {
      return(-1);
    }
    
    // Now respond with an PKT_PULL_ACK; 0x04 UP
    buff_up[0]=buff_down[0];
    buff_up[1]=buff_down[1];
    buff_up[2]=buff_down[2];
    buff_up[3]=PKT_PULL_ACK;
    buff_up[4]=0;
    
    // Only send the PKT_PULL_ACK to the UDP socket that just sent the data!!!
    Udp.beginPacket(remoteIpNo, remotePortNo);
    if (Udp.write((char *)buff_up, 4) != 4) {
      Serial.println("PKT_PULL_ACK:: Error writing Ack");
    }
    else {
      if (debug>=1) {
        Serial.print(F("PKT_PULL_ACK:: tmst="));
        Serial.println(micros());
      }
    }
    //yield();
    Udp.endPacket();
    
    if (debug >=1) {
      Serial.print(F("PKT_PULL_RESP:: size ")); Serial.print(packetSize);
      Serial.print(F(" From ")); Serial.print(remoteIpNo);
      Serial.print(F(", port ")); Serial.print(remotePortNo); 
      Serial.print(F(", data: "));
      data = buff_down + 4;
      data[packetSize] = 0;
      Serial.print((char *)data);
      Serial.println(F("..."));
    }
    
  break;
  case PKT_PULL_ACK:  // 0x04 DOWN; the server sends a PULL_ACK to confirm PULL_DATA receipt
    if (debug >= 2) {
      Serial.print(F("PKT_PULL_ACK:: size ")); Serial.print(packetSize);
      Serial.print(F(" From ")); Serial.print(remoteIpNo);
      Serial.print(F(", port ")); Serial.print(remotePortNo); 
      Serial.print(F(", data: "));
      for (int i=0; i<packetSize; i++) {
        Serial.print(buff_down[i],HEX);
        Serial.print(':');
      }
      Serial.println();
    }
  break;
  default:
#if GATEWAYMGT==1
    // For simplicity, we send the first 4 bytes too
    gateway_mgt(packetSize, buff_down);
#else
#endif

    Serial.print(F(", ERROR ident not recognized: "));
    Serial.println(ident);
  break;
  }
  
  // For downstream messages, fill the buff_down buffer
  
  return packetSize;
}


// ----------------------------------------------------------------------------
// Send UP an UDP/DGRAM message to the MQTT server
// If we send to more than one host (not sure why) then we need to set sockaddr 
// before sending.
// ----------------------------------------------------------------------------
void sendUdp(uint8_t * msg, int length) {
  int l;
  lastToken = msg[2]*256+msg[1];
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println(F("sendUdp: ERROR not connected to WLAN"));
    Udp.flush();

    if (WlanConnect() < 0) {
      Serial.print(F("sendUdp: ERROR connecting to WiFi"));
      yield();
      return;
    }
    if (debug>=1) Serial.println(F("WiFi reconnected"));  
    delay(10);
  }

  //send the update
  
  Udp.beginPacket(ttnServer, (int) _TTNPORT);
  if ((l = Udp.write((char *)msg, length)) != length) {
    Serial.println("sendUdp:: Error write");
  }
  else {
    if (debug>=2) {
      Serial.print(F("sendUdp 1: sent "));
      Serial.print(l);
      Serial.println(F(" bytes"));
    }
  }
  yield();
  Udp.endPacket();
  
#ifdef _THINGSERVER
  delay(1);

  Udp.beginPacket(thingServer, (int) _THINGPORT);
    if ((l = Udp.write((char *)msg, length)) != length) {
    Serial.println("sendUdp:: Error write");
  }
  else {
    if (debug>=2) {
      Serial.print(F("sendUdp 2: sent "));
      Serial.print(l);
      Serial.println(F(" bytes"));
    }
  }
  yield();
  Udp.endPacket();
#endif

  return;
}


// ----------------------------------------------------------------------------
// connect to UDP – returns true if successful or false if not
// ----------------------------------------------------------------------------
bool UDPconnect() {

  bool ret = false;
  unsigned int localPort = _LOCUDPPORT;     // To listen to return messages from WiFi
  if (debug>=1) {
    Serial.print(F("Local UDP port "));
    Serial.println(localPort);
  }
  
  if (Udp.begin(localPort) == 1) {
    if (debug>=1) Serial.println(F("Connection successful"));
    ret = true;
  }
  else{
    //Serial.println("Connection failed");
  }
  return(ret);
}




// ----------------------------------------------------------------------------
// Send UP periodic Pull_DATA message to server to keepalive the connection
// and to invite the server to send downstream messages when these are available
// *2, par. 5.2
//  - Protocol Version (1 byte)
//  - Random Token (2 bytes)
//  - PULL_DATA identifier (1 byte) = 0x02
//  - Gateway unique identifier (8 bytes) = MAC address
// ----------------------------------------------------------------------------
void pullData() {

    uint8_t pullDataReq[12];            // status report as a JSON object
    int pullIndex=0;
  int i;
  
    // pre-fill the data buffer with fixed fields
    pullDataReq[0]  = PROTOCOL_VERSION;           // 0x01
  uint8_t token_h = (uint8_t)rand();            // random token
    uint8_t token_l = (uint8_t)rand();            // random token
    pullDataReq[1]  = token_h;
    pullDataReq[2]  = token_l;
    pullDataReq[3]  = PKT_PULL_DATA;            // 0x02
  
  // READ MAC ADDRESS OF ESP8266, and return unique Gateway ID consisting of MAC address and 2bytes 0xFF
    pullDataReq[4]  = MAC_array[0];
    pullDataReq[5]  = MAC_array[1];
    pullDataReq[6]  = MAC_array[2];
    pullDataReq[7]  = 0xFF;
    pullDataReq[8]  = 0xFF;
    pullDataReq[9]  = MAC_array[3];
    pullDataReq[10] = MAC_array[4];
    pullDataReq[11] = MAC_array[5];

    pullIndex = 12;                     // 12-byte header
  
    pullDataReq[pullIndex] = 0;               // add string terminator, for safety

    if (debug>= 2) {
    Serial.print(F("PKT_PULL_DATA request: <"));
    Serial.print(pullIndex);
    Serial.print(F("> "));
    for (i=0; i<pullIndex; i++) {
      Serial.print(pullDataReq[i],HEX);       // DEBUG: display JSON stat
      Serial.print(':');
    }
    Serial.println();
  }
    //send the update
    sendUdp(pullDataReq, pullIndex);
}


// ----------------------------------------------------------------------------
// Send UP periodic status message to server even when we do not receive any
// data. 
// Parameter is socketr to TX to
// ----------------------------------------------------------------------------
void sendstat() {

    uint8_t status_report[STATUS_SIZE];           // status report as a JSON object
    char stat_timestamp[32];                // XXX was 24
    time_t t;
  char clat[10]={0};
  char clon[10]={0};

    int stat_index=0;
  
    // pre-fill the data buffer with fixed fields
    status_report[0]  = PROTOCOL_VERSION;         // 0x01
    status_report[3]  = PKT_PUSH_DATA;            // 0x00
  
  // READ MAC ADDRESS OF ESP8266, and return unique Gateway ID consisting of MAC address and 2bytes 0xFF
    status_report[4]  = MAC_array[0];
    status_report[5]  = MAC_array[1];
    status_report[6]  = MAC_array[2];
    status_report[7]  = 0xFF;
    status_report[8]  = 0xFF;
    status_report[9]  = MAC_array[3];
    status_report[10] = MAC_array[4];
    status_report[11] = MAC_array[5];

    uint8_t token_h   = (uint8_t)rand();          // random token
    uint8_t token_l   = (uint8_t)rand();          // random token
    status_report[1]  = token_h;
    status_report[2]  = token_l;
    stat_index = 12;                    // 12-byte header
  
    t = now();                        // get timestamp for statistics
  
  // XXX Using CET as the current timezone. Change to your timezone 
  sprintf(stat_timestamp, "%04d-%02d-%02d %02d:%02d:%02d CET", year(),month(),day(),hour(),minute(),second());
  yield();
  
  ftoa(lat,clat,4);                   // Convert lat to char array with 4 decimals
  ftoa(lon,clon,4);                   // As Arduino CANNOT prints floats
  
  // Build the Status message in JSON format, XXX Split this one up...
  delay(1);
  
    int j = snprintf((char *)(status_report + stat_index), STATUS_SIZE-stat_index, 
    "{\"stat\":{\"time\":\"%s\",\"lati\":%s,\"long\":%s,\"alti\":%i,\"rxnb\":%u,\"rxok\":%u,\"rxfw\":%u,\"ackr\":%u.0,\"dwnb\":%u,\"txnb\":%u,\"pfrm\":\"%s\",\"mail\":\"%s\",\"desc\":\"%s\"}}", 
    stat_timestamp, clat, clon, (int)alt, cp_nb_rx_rcv, cp_nb_rx_ok, cp_up_pkt_fwd, 0, 0, 0,platform,email,description);
    
  yield();                        // Give way to the internal housekeeping of the ESP8266
  if (debug >=1) { delay(1); }
    stat_index += j;
    status_report[stat_index] = 0;              // add string terminator, for safety

    if (debug>=2) {
    Serial.print(F("stat update: <"));
    Serial.print(stat_index);
    Serial.print(F("> "));
    Serial.println((char *)(status_report+12));     // DEBUG: display JSON stat
  }
  
    //send the update
  // delay(1);
    sendUdp(status_report, stat_index);
  return;
}



// ============================================================================
// MAIN PROGRAM CODE (SETUP AND LOOP)


// ----------------------------------------------------------------------------
// Setup code (one time)
// ----------------------------------------------------------------------------
void setup () {

  Serial.begin(_BAUDRATE);            // As fast as possible for bus
  Serial.flush();
  delay(1000);
  
  wifi_station_set_hostname( "espgway" );

  if (SPIFFS.begin()) Serial.println(F("SPIFFS loaded success"));

  // The following section can normnally be left OFF. Only for those suffering
  // from exception problems due to am erro in the WiFi code of the ESP8266
#define WIFIBUG 0                 // System no recovers from Wifi errors
#if WIFIBUG==1
  uint16_t s,res;
  os_printf("Erasing sectors\n");
  for (s=0x70;s<=0x7F; s++)
  {
    res = spi_flash_erase_sector(s);
    os_printf("Sector erased 0x%02X. Res %d\n",s,res);
  }
#endif

  delay(2000);
  yield();
    
  if (debug>=1) {
    Serial.print(F("debug=")); 
    Serial.println(debug);
    yield();
  }
  
  WlanReadWpa();                // Read the last Wifi settings from SPIFFS into memory
  
  // Setup WiFi UDP connection. Give it some time ..
  while (WlanConnect() < 0) {
    Serial.print(F("Error Wifi network connect "));
    Serial.println();
    yield();
  }
  
  Serial.print(F("Wlan Connected to "));
  Serial.print(WiFi.SSID());
  Serial.println();
 //Line_Notify("RMUTL connected :"+WiFi.SSID()); 
  delay(200);
  // If we are here we are connected to WLAN
  // So now test the UDP function
  if (!UDPconnect()) {
    Serial.println(F("Error UDPconnect"));
  }
  delay(500);

   
  WiFi.macAddress(MAC_array);
    for (int i = 0; i < sizeof(MAC_array); ++i){
      sprintf(MAC_char,"%s%02x:",MAC_char,MAC_array[i]);
    }
  Serial.print("MAC: ");
    Serial.println(MAC_char);
    Line_Notify("GW @RMUTL connected :"+WiFi.SSID()+" MAC addr:"+MAC_char); 
  
    pinMode(ssPin, OUTPUT);
    pinMode(dio0, INPUT);
    pinMode(RST, OUTPUT);
  
  SPI.begin();
  delay(1000);
    initLoraModem();
  delay(1000);
  
  // We choose the Gateway ID to be the Ethernet Address of our Gateway card
    // display results of getting hardware address
  // 
    Serial.print("Gateway ID: ");
  printHexDigit(MAC_array[0]);
    printHexDigit(MAC_array[1]);
    printHexDigit(MAC_array[2]);
  printHexDigit(0xFF);
  printHexDigit(0xFF);
    printHexDigit(MAC_array[3]);
    printHexDigit(MAC_array[4]);
    printHexDigit(MAC_array[5]);

    Serial.print(", Listening at SF");
  Serial.print(sf);
  Serial.print(" on ");
  Serial.print((double)freq/1000000);
  Serial.println(" Mhz.");

  WiFi.hostByName(_TTNSERVER, ttnServer);         // Use DNS to get server IP once
  delay(500);

//#if defined(_THINGSERVER)
//  WiFi.hostByName(_THINGSERVER, thingServer);
//  delay(500);
//#endif
  
  setupTime();                      // Set NTP time host and interval
  setTime((time_t)getNtpTime());
  while (timeStatus() == timeNotSet) {
    Serial.println(F("setupTime:: Time not set (yet)"));
    delay(500);
    setTime((time_t)getNtpTime());
  }
  Serial.print("Time: "); printTime();
  Serial.println();

  writeGwayCfg( CONFIGFILE );
  Serial.println(F("Gateway configuration saved"));

#if A_SERVER==1 
  // Setup the webserver
  setupWWW();
#endif

  Serial.println(F("--------------------------------------"));
  delay(100);                     // Wait after setup
}


// ----------------------------------------------------------------------------
// LOOP
// This is the main program that is executed time and time again.
// We need to give way to the backend WiFi processing that 
// takes place somewhere in the ESP8266 firmware and therefore
// we include yield() statements at important points.
//
// Note: If we spend too much time in user processing functions
//  and the backend system cannot do its housekeeping, the watchdog
// function will be executed which means effectively that the 
// program crashes.
//
// NOTE: For ESP make sure not to do lage array declarations in loop();
// ----------------------------------------------------------------------------
void loop ()
{
  uint32_t nowseconds;
  int buff_index=0;
  int packetSize;
  
  // Receive Lora messages if there are any
    if ((buff_index = receivePacket(buff_up)) >= 0) { // read is successful
    yield();
    // rxpk PUSH_DATA received from node is rxpk (*2, par. 3.2)
    sendUdp(buff_up, buff_index);         // We can send to multiple sockets if necessary
  //  Line_Notify("GW @RMUTL ra-02 get message");
  }
  else {
    // No LoRa message received
  }
  
  yield();
  
  // Receive UDP PUSH_ACK messages from server. (*2, par. 3.3)
  // This is important since the TTN broker will return confirmation
  // messages on UDP for every message sent by the gateway. So we have to consume them..
  // As we do not know when the server will respond, we test in every loop.
  //

  while( (packetSize = Udp.parsePacket()) > 0) {    // Length of UDP message waiting
    yield();
    // Packet may be PKT_PUSH_ACK (0x01), PKT_PULL_ACK (0x03) or PKT_PULL_RESP (0x04)
    // This command is found in byte 4 (buff_down[3])
    if (readUdp(packetSize, buff_down) < 0) {
      Serial.println(F("readUDP error"));
    }
  }
  
  yield();

  // stat PUSH_DATA message (*2, par. 4)
  //  
  nowseconds = (uint32_t) millis() /1000;
    if (nowseconds - stattime >= _STAT_INTERVAL) {    // Wake up every xx seconds
        sendstat();                   // Show the status message and send to server
    
    // If the 1ch gateway is a sensor itself, send the sensor values
    // could be battery but also other status info or sensor info
#if GATEWAYNODE==1
    yield();
    if ((buff_index = sensorPacket(buff_up)) >= 0) {
      yield();
      sendUdp(buff_up, buff_index);
    }
#endif
    stattime = nowseconds;
    }
  
  yield();
  
  // send PULL_DATA message (*2, par. 4)
  //
  nowseconds = (uint32_t) millis() /1000;
    if (nowseconds - pulltime >= _PULL_INTERVAL) {    // Wake up every xx seconds
        pullData();                   // Send PULL_DATA message to server           
    pulltime = nowseconds;
    }

  // Handle the WiFi server part of this sketch. Mainly used for administration of the node
#if A_SERVER==1
  server.handleClient();
#endif  
  
  yield();
}

void Line_Notify(String message) {
  WiFiClientSecure client;

  if (!client.connect("notify-api.line.me", 443)) {
    Serial.println("connection failed");
    return;   
  }

  String req = "";
  req += "POST /api/notify HTTP/1.1\r\n";
  req += "Host: notify-api.line.me\r\n";
  req += "Authorization: Bearer " + String(LINE_TOKEN) + "\r\n";
  req += "Cache-Control: no-cache\r\n";
  req += "User-Agent: ESP8266\r\n";
  req += "Content-Type: application/x-www-form-urlencoded\r\n";
  req += "Content-Length: " + String(String("message=" + message).length()) + "\r\n";
  req += "\r\n";
  req += "message=" + message;
  // Serial.println(req);
  client.print(req);
    
  delay(20);

  // Serial.println("-------------");
  while(client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") {
      break;
    }
    //Serial.println(line);
  }
  // Serial.println("-------------");
}
