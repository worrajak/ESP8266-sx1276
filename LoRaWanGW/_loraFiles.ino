// 1-channel LoRa Gateway for ESP8266
// Copyright (c) 2016 Maarten Westenberg version for ESP8266
// Verison 3.3.0
// Date: 2016-01-02
//
// 	based on work done by Thomas Telkamp for Raspberry PI 1ch gateway
//	and many others.
//
// All rights reserved. This program and the accompanying materials
// are made available under the terms of the MIT License
// which accompanies this distribution, and is available at
// https://opensource.org/licenses/mit-license.php
//
// Author: Maarten Westenberg (mw12554@hotmail.com)
//
// This file contains the LoRa filesystem specific code


// ============================================================================
// LORA SPIFFS FILESYSTEM FUNCTIONS
//
// The LoRa supporting functions are in the section below

// ----------------------------------------------------------------------------
// Directory listing. s is a string containing HTML/text code so far.
// The resulting directory listing is appended to s and returned.
// ----------------------------------------------------------------------------
String espDir(String s) {


	return s;
}

// ----------------------------------------------------------------------------
// Read the gateway configuration file
// ----------------------------------------------------------------------------
int readConfig(char *fn, struct espGwayConfig *c) {

	Serial.println(F("readConfig:: Starting"));

	if (!SPIFFS.exists(fn)) {
		Serial.print("ERROR:: readConfig, file does not exist ");
		Serial.println(fn);
		return(-1);
	}
	File f = SPIFFS.open(fn, "r");
	if (!f) {
		Serial.println("ERROR:: readConfig, file open failed");
		return(-1);
	}

	while (f.available()) {
		
		String id =f.readStringUntil('=');
		String val=f.readStringUntil('\n');
		
		if (id == "SSID") {									// WiFi SSID
			Serial.print(F("SSID=")); Serial.println(val);
			(*c).ssid = val;
		}
		if (id == "PASS") { 								// WiFi Password
			Serial.print(F("PASS=")); Serial.println(val); 
			(*c).pass = val;
		}
		if (id == "FREQ") { 								// Frequency
			Serial.print(F("FREQ=")); Serial.println(val); 
			//int i = sscanf(val.c_str(), "%d", (*c).freq);
			(*c).freq = (uint32_t) val.toInt();
		}
		if (id == "SF") { 									// Spreading Factor
			Serial.print(F("SF  =")); Serial.println(val);
			//int i = sscanf(val.c_str(), "%d", (*c).sf);
			(*c).sf = (uint32_t) val.toInt();
		}
		if (id == "FCNT") {									// Frame Counter
			Serial.print(F("FCNT=")); Serial.println(val);
			//int i = sscanf(val.c_str(), "%d", (*c).sf);
			(*c).fcnt = (uint32_t) val.toInt();
		}
	}

	f.close();
	return(1);
}

// ----------------------------------------------------------------------------
// Write the current gateway configuration to SPIFFS. First copy all the
// separate data items to the gwayConfig structure
//
// ----------------------------------------------------------------------------
int writeGwayCfg(char *fn) {

	gwayConfig.sf = (uint8_t) sf;
	gwayConfig.ssid = WiFi.SSID();
	//gwayConfig.pass = WiFi.PASS();					// XXX We should find a way to store the password too
	gwayConfig.freq = freq;
#if GATEWAYNODE == 1
	gwayConfig.fcnt = frameCount;
#endif	
	return(writeConfig(fn, &gwayConfig));
}

// ----------------------------------------------------------------------------
// Write the configuration ad found in the espGwayConfig structure
// to SPIFFS
// ----------------------------------------------------------------------------
int writeConfig(char *fn, struct espGwayConfig *c) {

	if (!SPIFFS.exists(fn)) {
		Serial.print("ERROR:: writeConfig, file does not exist, formatting ");
		SPIFFS.format();
		Serial.println(fn);
	}
	File f = SPIFFS.open(fn, "w");
	if (!f) {
		Serial.print("ERROR:: writeConfig, file open failed for file=");
		Serial.print(fn);
		Serial.println();
		return(-1);
	}

	f.print("SSID"); f.print('='); f.print((*c).ssid); f.print('\n'); 
	f.print("PASS"); f.print('='); f.print((*c).pass); f.print('\n');
	f.print("FREQ"); f.print('='); f.print((*c).freq); f.print('\n');
	f.print("SF");   f.print('='); f.print((*c).sf);   f.print('\n');
	f.print("FCNT"); f.print('='); f.print((*c).fcnt); f.print('\n');
	
	if (debug>=2){
		Serial.print(F("SSID=")); Serial.println((*c).ssid);
		Serial.print(F("PASS=")); Serial.println((*c).pass);
		Serial.print(F("FREQ=")); Serial.println((*c).freq);
		Serial.print(F("SF  =")); Serial.println((*c).sf);
		Serial.print(F("FCNT=")); Serial.println((*c).fcnt);
	}
	
	Serial.println(F("writeConfig:: done"));
	
	f.close();
	return(1);
}
