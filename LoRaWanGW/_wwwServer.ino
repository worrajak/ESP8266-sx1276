// 1-channel LoRa Gateway for ESP8266
// Copyright (c) 2016 Maarten Westenberg version for ESP8266
// Verison 3.3.0
// Date: 2016-12-30
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
// This file contains the webserver code for the ESP Single Channel Gateway.

#if A_SERVER==1

// ================================================================================
// WEBSERVER DECLARATIONS 




// ================================================================================
// WEBSERVER FUNCTIONS 

// ----------------------------------------------------------------------------
// Output the 4-byte IP address for easy printing
// ----------------------------------------------------------------------------
String printIP(IPAddress ipa) {
	String response;
	response+=(IPAddress)ipa[0]; response+=".";
	response+=(IPAddress)ipa[1]; response+=".";
	response+=(IPAddress)ipa[2]; response+=".";
	response+=(IPAddress)ipa[3];
	return (response);
}

// ----------------------------------------------------------------------------
// stringTime
// Only when RTC is present we print real time values
// t contains number of milli seconds since system started that the event happened.
// So a value of 100 wold mean that the event took place 1 minute and 40 seconds ago
// ----------------------------------------------------------------------------
String stringTime(unsigned long t) {
	String response;
	String Days[7]={"Sunday","Monday","Tuesday","Wednesday","Thursday","Friday","Saturday"};

	if (t==0) { response = " -none- "; return(response); }
	
	// now() gives seconds since 1970
	time_t eventTime = now() - ((millis()-t)/1000);
	byte _hour   = hour(eventTime);
	byte _minute = minute(eventTime);
	byte _second = second(eventTime);
	
	response += Days[weekday(eventTime)-1]; response += " ";
	response += day(eventTime); response += "-";
	response += month(eventTime); response += "-";
	response += year(eventTime); response += " ";
	if (_hour < 10) response += "0";
	response += _hour; response +=":";
	if (_minute < 10) response += "0";
	response += _minute; response +=":";
	if (_second < 10) response += "0";
	response += _second;
	return (response);
}


// ----------------------------------------------------------------------------
// WIFI SERVER
//
// This funtion implements the WiFI Webserver (very simple one). The purpose
// of this server is to receive simple admin commands, and execute these
// results are sent back to the web client.
// Commands: DEBUG, ADDRESS, IP, CONFIG, GETTIME, SETTIME
// The webpage is completely built response and then printed on screen.
// ----------------------------------------------------------------------------
String WifiServer(char *cmd, char *arg) {

	String response;
	char *dup, *pch;

	yield();	
	if (debug >=2) { Serial.print(F("WifiServer new client")); }

	// DEBUG settings; These can be used as a single argument
	if (strcmp(cmd, "DEBUG")==0) {									// Set debug level 0-2
		debug=atoi(arg); response+=" debug="; response+=arg;
	}
	if (strcmp(cmd, "DELAY")==0) {									// Set debug level 0-2
		txDelay+=atoi(arg)*1000; response+=" delay="; response+=arg;
	}
	
	// SF; Handle Spreading Factor Settings
	if (strcmp(cmd, "SF")==0) {
		uint8_t sfi = sf;
		if (atoi(arg) == 1) {
			if (sf==SF12) sfi=SF7; else sfi++;
		}	
		else if (atoi(arg) == -1) {
			if (sf==SF7) sfi=SF12; else sfi--;
		}
		sf = (sf_t)sfi;
		rxLoraModem();												// Reset the radion with the new spreading factor
		writeGwayCfg(CONFIGFILE);									// Save configuration to file
	}
	
	// FREQ; Handle Frequency  Settings
	if (strcmp(cmd, "FREQ")==0) {
		uint8_t nf = sizeof(freqs)/sizeof(int);						// Number of elements in array
		
		// Compute frequency index
		uint8_t fri=0;
		// We assume we will always find an index, and index >=0 and index < length
		for (int i=0; i<nf; i++) { if (freqs[i] == freq) { fri = i; break; }}
		
		if (atoi(arg) == 1) {
			if (fri==(nf-1)) fri=0; else fri++;
		}	
		else if (atoi(arg) == -1) {
			Serial.println("down");
			if (fri==0) fri=(nf-1); else fri--;
		}

		freq = freqs[fri];
		rxLoraModem();												// Reset the radion with the new frequency
		writeGwayCfg(CONFIGFILE);									// Save configuration to file
	}
	
	// Handle IP settings
	if (strcmp(cmd, "IP")==0) {										// List local IP address
		response+=" local IP="; 
		response+=(IPAddress) WiFi.localIP()[0]; response += ".";
		response+=(IPAddress) WiFi.localIP()[1]; response += ".";
		response+=(IPAddress) WiFi.localIP()[2]; response += ".";
		response+=(IPAddress) WiFi.localIP()[3];
	}

	if (strcmp(cmd, "GETTIME")==0) { response += "gettime tbd"; }	// Get the local time
	if (strcmp(cmd, "SETTIME")==0) { response += "settime tbd"; }	// Set the local time
	if (strcmp(cmd, "HELP")==0)    { response += "Display Help Topics"; }
#if GATEWAYNODE == 1
	if (strcmp(cmd, "FCNT")==0)   { 
		frameCount=0; 
		rxLoraModem();												// Reset the radion with the new frequency
		writeGwayCfg(CONFIGFILE);
	}
#endif
	if (strcmp(cmd, "RESET")==0)   { 
		response += "Resetting Statistics"; 
		cp_nb_rx_rcv = 0;
		cp_nb_rx_ok = 0;
		cp_up_pkt_fwd = 0;
	}
#if WIFIMANAGER==1
	if (strcmp(cmd, "NEWSSID")==0) { 
		WiFiManager wifiManager;
		strcpy(wpa[0].login,""); 
		strcpy(wpa[0].passw,"");
		WiFi.disconnect();
		wifiManager.autoConnect(AP_NAME, AP_PASSWD );
	}
#endif

	// Do webserver work, fill the webpage
	delay(15);	
	response +="<!DOCTYPE HTML>";
	response +="<HTML><HEAD>";
	response +="<TITLE>ESP8266 1ch Gateway</TITLE>";
	response +="</HEAD>";
	response +="<BODY>";
		
	response +="<h1>ESP Gateway Config:</h1>";
	response +="Version: "; response+=VERSION;
	response +="<br>ESP is alive since "; response+=stringTime(1); 
	response +="<br>Current time is    "; response+=stringTime(millis()); 
	response +="<br>";

	response +="<h2>Statistics</h2>";

	delay(1);
	response +="<table style=\"max_width: 100%; min-width: 60%; border: 1px solid black; border-collapse: collapse;\" class=\"config_table\">";
	response +="<tr>";
	response +="<th style=\"background-color: green; color: white;\">Counter</th>";
	response +="<th style=\"background-color: green; color: white;\">Value</th>";
	response +="</tr>";
	response +="<tr><td style=\"border: 1px solid black;\">Packages Uplink Total</td><td style=\"border: 1px solid black;\">"; response +=cp_nb_rx_rcv; response+="</tr>";
	response +="<tr><td style=\"border: 1px solid black;\">Packages Uplink OK </td><td style=\"border: 1px solid black;\">"; response +=cp_nb_rx_ok; response+="</tr>";
	response +="<tr><td style=\"border: 1px solid black;\">Packages Downlink</td><td style=\"border: 1px solid black;\">"; response +=cp_up_pkt_fwd; response+="</tr>";
	
	response +="<tr><td style=\"border: 1px solid black;\">Reset Statistics</td>";
	response +="<td style=\"border: 1px solid black;\"><a href=\"/RESET\">RESET</a></td>";
	response +="</tr>";
	response +="</table>";

	response +="<h2>WiFi Config</h2>";

	response +="<table style=\"width:300px; max_width: 100%; min-width: 60%; border: 1px solid black; border-collapse: collapse;\" class=\"config_table\">";
	response +="<tr>";
	response +="<th style=\"background-color: green; color: white;\">Parameter</th>";
	response +="<th style=\"background-color: green; color: white;\">Value</th>";
	response +="</tr>";
	response +="<tr><td style=\"border: 1px solid black;\">WiFi SSID</td><td style=\"border: 1px solid black;\">"; response+=WiFi.SSID(); response+="</tr>";
	response +="<tr><td style=\"border: 1px solid black;\">IP Address</td><td style=\"border: 1px solid black;\">"; response+=printIP((IPAddress)WiFi.localIP()); response+="</tr>";
	response +="<tr><td style=\"border: 1px solid black;\">IP Gateway</td><td style=\"border: 1px solid black;\">"; response+=printIP((IPAddress)WiFi.gatewayIP()); response+="</tr>";
	response +="<tr><td style=\"border: 1px solid black;\">NTP Server</td><td style=\"border: 1px solid black;\">"; response+=NTP_TIMESERVER; response+="</tr>";
	response +="<tr><td style=\"border: 1px solid black;\">LoRa Router</td><td style=\"border: 1px solid black;\">"; response+=_TTNSERVER; response+="</tr>";
	response +="<tr><td style=\"border: 1px solid black;\">LoRa Router IP</td><td style=\"border: 1px solid black;\">"; response+=printIP((IPAddress)ttnServer); response+="</tr>";

#ifdef _THINGSERVER
	response +="<tr><td style=\"border: 1px solid black;\">LoRa Router 2</td><td style=\"border: 1px solid black;\">"; response+=_THINGSERVER; 
		response += ":"; response += String(_THINGPORT); response+="</tr>";
	response +="<tr><td style=\"border: 1px solid black;\">LoRa Router 2 IP</td><td style=\"border: 1px solid black;\">"; response+=printIP((IPAddress)thingServer); response+="</tr>";
#endif
	response +="</table>";
	
	response +="<h2>System Status</h2>";
	
	response +="<table style=\"max_width: 100%; min-width: 60%; border: 1px solid black; border-collapse: collapse;\" class=\"config_table\">";
	response +="<tr>";
	response +="<th style=\"background-color: green; color: white;\">Parameter</th>";
	response +="<th style=\"background-color: green; color: white;\">Value</th>";
	response +="</tr>";
	response +="<tr><td style=\"border: 1px solid black;\">Free heap</td><td style=\"border: 1px solid black;\">"; response+=ESP.getFreeHeap(); response+="</tr>";
	response +="<tr><td style=\"border: 1px solid black;\">ESP Chip ID</td><td style=\"border: 1px solid black;\">"; response+=ESP.getChipId(); response+="</tr>";
	response +="</table>";

	response +="<h2>LoRa Settings</h2>";
	
	response +="<table style=\"max_width: 100%; min-width: 60%; border: 1px solid black; border-collapse: collapse;\" class=\"config_table\">";
	response +="<tr>";
	response +="<th style=\"background-color: green; color: white;\">Counter</th>";
	response +="<th style=\"background-color: green; color: white;\">Value</th>";
	response +="<th colspan=\"2\" style=\"background-color: green; color: white;\">Set</th>";
	response +="</tr>";
	
	response +="<tr><td style=\"border: 1px solid black;\">Gateway ID</td><td style=\"border: 1px solid black;\">";	
	  if (MAC_array[0]< 0x10) response +='0'; response +=String(MAC_array[0],HEX);	// The MAC array is always returned in lowercase
	  if (MAC_array[1]< 0x10) response +='0'; response +=String(MAC_array[1],HEX);
	  if (MAC_array[2]< 0x10) response +='0'; response +=String(MAC_array[2],HEX);
	  response +="ffff"; 
	  if (MAC_array[3]< 0x10) response +='0'; response +=String(MAC_array[3],HEX);
	  if (MAC_array[4]< 0x10) response +='0'; response +=String(MAC_array[4],HEX);
	  if (MAC_array[5]< 0x10) response +='0'; response +=String(MAC_array[5],HEX);
	response+="</tr>";
	
#if GATEWAYMGT==1
	response +="<tr><td style=\"border: 1px solid black;\">SF Setting</td><td style=\"border: 1px solid black;\">"; response += sf;
	response +="<td style=\"border: 1px solid black;\"><a href=\"SF=1\">UP</a></td>";
	response +="<td style=\"border: 1px solid black;\"><a href=\"SF=-1\">DOWN</a></td>";
	response +="</tr>";
	
	response +="<tr><td style=\"border: 1px solid black;\">Frequency</td>";
	  response +="<td style=\"border: 1px solid black;\">"; 
	  response+=freq; 
	response+="</td>";
	response +="<td style=\"border: 1px solid black;\"><a href=\"FREQ=1\">UP</a></td>";
	response +="<td style=\"border: 1px solid black;\"><a href=\"FREQ=-1\">DOWN</a></td>";
	response +="</tr>";
	
	response +="<tr><td style=\"border: 1px solid black;\">Timing Correction (uSec)</td><td style=\"border: 1px solid black;\">"; response += txDelay; 
	response+="</td>";
	response +="<td style=\"border: 1px solid black;\"><a href=\"DELAY=1\">UP</a></td>";
	response +="<td style=\"border: 1px solid black;\"><a href=\"DELAY=-1\">DOWN</a></td>";
	response+="</tr>";
	
#endif	

#if GATEWAYNODE==1
	response +="<tr><td style=\"border: 1px solid black;\">Framecounter Internal Sensor</td>";
	response +="<td style=\"border: 1px solid black;\">";
	response +=frameCount;
	response +="</td><td colspan=\"2\" style=\"border: 1px solid black;\">";
	response +="<a href=\"/FCNT\">RESET</a></td>";
	response +="</tr>";
#endif

			
	response +="</table>";
	response +="<br>";
	response +="<table>";
	


	response +="<tr><td>";
	response +="Debug level is: "; 
	response += debug; 
	response +=" set to: ";
	response +="</td><td>";
	response +=" <a href=\"DEBUG=0\">0</a>";
	response +=" <a href=\"DEBUG=1\">1</a>";
	response +=" <a href=\"DEBUG=2\">2</a><br>";
	response +="</td></tr>";
	

	
#if WIFIMANAGER==1
	response +="<tr><td>";
	response +="Click <a href=\"/NEWSSID\">here</a> to reset accesspoint<br>";
	response +="</td><td></td></tr>";
#endif

	response += "</table>";
	
	response +="<nbsp><nbsp><br><br />";
	response +="Click <a href=\"/HELP\">here</a> to explain Help and REST options<br>";
	response +="</BODY></HTML>";

	delay(5);
	free(dup);									// free the memory used, before jumping to other page
	return (response);
}

// ----------------------------------------------------------------------------
// SetupWWW function called by main setup() program to setup webserver
// It does actually not much more than installaing the callback handlers
// for messages sent to the webserver
// ----------------------------------------------------------------------------
void setupWWW() {	
	server.on("/", []() {
		webPage = WifiServer("","");
		server.send(200, "text/html", webPage);					// Send the webPage string
	});
	server.on("/HELP", []() {
		webPage = WifiServer("HELP","");
		server.send(200, "text/html", webPage);					// Send the webPage string
	});
	server.on("/RESET", []() {
		webPage = WifiServer("RESET","");
		server.send(200, "text/html", webPage);				// Send the webPage string
	});
	server.on("/NEWSSID", []() {
		webPage = WifiServer("NEWSSID","");
		server.send(200, "text/html", webPage);				// Send the webPage string
	});
	server.on("/DEBUG=0", []() {
		webPage = WifiServer("DEBUG","0");
		server.send(200, "text/html", webPage);				// Send the webPage string
	});
	server.on("/DEBUG=1", []() {
		webPage = WifiServer("DEBUG","1");
		server.send(200, "text/html", webPage);				// Send the webPage string
	});
	server.on("/DEBUG=2", []() {
		webPage = WifiServer("DEBUG","2");
		server.send(200, "text/html", webPage);				// Send the webPage string
	});
	server.on("/DELAY=1", []() {
		webPage = WifiServer("DELAY","1");
		server.send(200, "text/html", webPage);				// Send the webPage string
	});
	server.on("/DELAY=-1", []() {
		webPage = WifiServer("DELAY","-1");
		server.send(200, "text/html", webPage);				// Send the webPage string
	});
	server.on("/SF=1", []() {
		webPage = WifiServer("SF","1");
		server.send(200, "text/html", webPage);				// Send the webPage string
	});
	server.on("/SF=-1", []() {
		webPage = WifiServer("SF","-1");
		server.send(200, "text/html", webPage);				// Send the webPage string
	});
	server.on("/FREQ=1", []() {
		webPage = WifiServer("FREQ","1");
		server.send(200, "text/html", webPage);				// Send the webPage string
	});
	server.on("/FREQ=-1", []() {
		webPage = WifiServer("FREQ","-1");
		server.send(200, "text/html", webPage);				// Send the webPage string
	});
		server.on("/FCNT", []() {
		webPage = WifiServer("FCNT","");
		server.send(200, "text/html", webPage);				// Send the webPage string
	});
	server.begin();											// Start the webserver
	Serial.print(F("Admin Server started on port "));
	Serial.println(SERVERPORT);
}




#endif

