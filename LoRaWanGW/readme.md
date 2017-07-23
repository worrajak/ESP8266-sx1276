LoRaWan Gateway



![ScreenShot](https://github.com/worrajak/ESP8266-sx1276/blob/master/LoRaWanGW/CircuitLoRaWan1.jpg?raw=true)

Config LoRamodem.h file
to 915MHz US band (Thailand 920MHz)  
```
int freqs [] = { 
	903900000, 									// Channel 0, 868.1 MHz
	904100000, 									// Channel 1, 868.3 MHz
	904300000, 									// Channel 2, 868.5 MHz
	923100000, 									// Channel 3, 867.1 MHz
	923300000, 
	923500000, 
	920100000, 
	920300000, 
	920500000, 
	920700000									// Channel, special for responses gateway (10%)
	// TTN defines an additional channel at 869.525Mhz using SF9 for class B. Not used
};
```
Config ESP-sc-gway.h file
changed username,password 

```
struct wpas {
	char login[32];				// Maximum Buffer Size (and allocated memory)
	char passw[64];
};

wpas wpa[] = {
  { "iPhone7","1234567890"}
  ,{ "HUAWEI","1234567890"}
  ,{ "TP-LINK","1234567890"}
};
```
