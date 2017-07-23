//
// Copyright (c) 2016 Maarten Westenberg version for ESP8266
// Verison 3.2.2
// Date: 2016-12-29
//
// 	based on work done by Thomas Telkamp for Raspberry PI 1ch gateway
//	and many others.
//
// All rights reserved. This program and the accompanying materials
// are made available under the terms of the MIT License
// which accompanies this distribution, and is available at
// https://opensource.org/licenses/mit-license.php
//
// Author: Maarten Westenberg
//
// This file contains a number of compile-time settings that can be set on (=1) or off (=0)
// The disadvantage of compile time is minor compared to the memory gain of not having
// too much code compiled and loaded on your ESP8266.
//
// ----------------------------------------------------------------------------------------

// The spreading factor is the most important parameter to set for a single channel
// gateway. It specifies the speed/datarate in which the gateway and node communicate.
// As the name says, in principle the single channel gateway listens to one channel/frequency
// and to one spreading factor only.
// NOTE: The frequency is set in the loraModem.h file and is default 868.100000 MHz.
#define _SPREADING SF7							// Send and receive on this Spreading Factor (only)

// Single channel gateways if they behave strict should only use one frequency channel and
// one spreading factor. However, the TTN backend replies on RX2 timeslot for spreading factors
// SF9-SF12 and frequency 869.525 MHz. 
// If the 1ch gateway is working for nodes that ONLY transmit and receive on the set
// and agreed frequency and spreading factor. make sure to set STRICT to 1.
// In this case, the frequency and spreading factor for downlink messages is adapted by this
// gateway
// NOTE: If your node has only one frequency enabled and one SF, you must set this to 1
//		in order to receive downlink messages
#define _STRICT_1CH	0							// 1 is strict, 0 is as sent by backend

// Gather statistics on sensor and Wifi status
#define STATISTICS 1

// Initial value of debug parameter. Can be hanged using the admin webserver
// For operational use, set initial DEBUG vaulue 0
#define DEBUG 1					

// Allows configuration through WifiManager AP setup. Must be 0 or 1					
#define WIFIMANAGER 0

// Define the name of the accesspoint if the gateway is in accesspoint mode (is
// getting WiFi SSID and password using WiFiManager)
#define AP_NAME "iPhone7pJAK"
#define AP_PASSWD "1234567890"

// Defines whether the gateway will also report sensor/status value on MQTT
// after all, a gateway can be a mote to the system as well
// Set its LoRa address and key below
// See spec. para 4.3.2
#define GATEWAYNODE 0	

// Define whether we want to manage the gateway over UDP (next to management thru webinterface).
// This will allow us to send messages over the UDP connection to manage the gateway and its 
// parameters. Sometimes the gateway is not accesible from remote, in this case we would allow it 
// to use the SERVER UDP connection to receive messages as well.
// NOTE: Be aware that these messages are NOT LoRa and NOT LoRa Gateway spec compliant.
//	However that should not interfere with regular gateway operation but instead offer 
//	functions to reset certain parameters from remote.
#define GATEWAYMGT 1

// Define the correct radio type that you are using
#define CFG_sx1276_radio		
//#define CFG_sx1272_radio

// Name of he configfile in SPIFFs	filesystem
// In this file we store the configuration and other relevant info that should
// survive a reboot of the gateway		
#define CONFIGFILE "/gwayConfig.txt"
								
// Wifi definitions
// WPA is an array with SSID and password records. Set WPA size to number of entries in array
// When using the WiFiManager, we will overwrite the first entry with the 
// accesspoint we last connected to with WifiManager
// NOTE: Structure needs at least one (empty) entry.
//		So WPASIZE must be >= 1
struct wpas {
	char login[32];				// Maximum Buffer Size (and allocated memory)
	char passw[64];
};

wpas wpa[] = {
  { "iPhone7pJAK","1234567890"}
  ,{ "HUAWEIJAK","1234567890"}
  ,{ "TP-LINK","123456789"}
  ,{ "PocketWifi","1234567890"}
};

// Set the Server Settings (IMPORTANT)


#define _LOCUDPPORT 1700						// Often 1700 or 1701 is used for upstream comms

#define _PULL_INTERVAL 30						// PULL_DATA messages to server to get downstream
#define _STAT_INTERVAL 60						// Send a 'stat' message to server
#define _NTP_INTERVAL 3600						// How often doe we want time NTP synchronization

// MQTT definitions
#define _TTNPORT 1700
//#define _TTNSERVER "router.eu.staging.thethings.network"
#define _TTNSERVER "router.us.thethings.network"

// Port is UDP port in this program
//#define _THINGPORT 1700						// ttb.things4u.eu
//#define _THINGSERVER "<your gateway server>"	// Server URL of the LoRa-udp.js program

// Gateway Ident definitions
#define _DESCRIPTION "ESP DIY Gateway"
#define _EMAIL "worrajak@gmail.com"
#define _PLATFORM "ESP8266 RF95"
#define _LAT 18.81200800
#define _LON 98.95424700
#define _ALT 20


								
// Definitions for the admin webserver
#define A_SERVER 1				// Define local WebServer only if this define is set
#define SERVERPORT 80			// local webserver port

#define A_MAXBUFSIZE 192		// Must be larger than 128, but small enough to work
#define _BAUDRATE 115200		// Works for debug messages to serial momitor (if attached).

// ntp
#define NTP_TIMESERVER "nl.pool.ntp.org"	// Country and region specific
#define NTP_TIMEZONES	2		// How far is our Timezone from UTC (excl daylight saving/summer time)
#define SECS_PER_HOUR	3600

#if !defined(CFG_noassert)
#define ASSERT(cond) if(!(cond)) gway_failed(__FILE__, __LINE__)
#else
#define ASSERT(cond) /**/
#endif
struct espGwayConfig {
	String ssid;				// String is more flexible and allows assignments
	String pass;
	uint32_t freq;				//=868100000 by default
	uint16_t fcnt;				//=0 as init value
	uint8_t sf;					//=9;
} gwayConfig;
