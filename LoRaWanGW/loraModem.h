// 1-channel LoRa Gateway for ESP8266
// Copyright (c) 2016 Maarten Westenberg version for ESP8266
// Verison 3.3.0
// Date: 2016-12-30
//
// 	based on work done by Thomas Telkamp for Raspberry PI 1ch gateway
//	and many other contributors.
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

// Our code should correct the server timing
long txDelay= 0000;								// extra delay time on top of server TMST


// Frequencies
// Set center frequency. If in doubt, choose the first one, comment all others
// Each "real" gateway should support the first 3 frequencies according to LoRa spec.

int freqs [] = { 
  903900000,                  // Channel 0, 868.1 MHz
  904100000,                  // Channel 1, 868.3 MHz
  904300000,                  // Channel 2, 868.5 MHz
  923100000,                  // Channel 3, 867.1 MHz
  923300000, 
  923500000, 
  920100000, 
  920300000, 
  920500000, 
  920700000                 // Channel, special for responses gateway (10%)
  // TTN defines an additional channel at 869.525Mhz using SF9 for class B. Not used
};
uint32_t  freq = freqs[0];

// ============================================================================
// Set all definitions for Gateway
// ============================================================================	
// Register definitions. These are the addresses of the TFM95, SX1276 that we 
// need to set in the program.

#define REG_FIFO                    0x00
#define REG_OPMODE                  0x01
#define REG_PAC                     0x09
#define REG_PARAMP                  0x0A
#define REG_LNA                     0x0C
#define REG_FIFO_ADDR_PTR           0x0D
#define REG_FIFO_TX_BASE_AD         0x0E
#define REG_FIFO_RX_BASE_AD         0x0F

#define REG_FIFO_RX_CURRENT_ADDR    0x10
#define REG_IRQ_FLAGS_MASK          0x11
#define REG_IRQ_FLAGS               0x12
#define REG_RX_NB_BYTES             0x13
#define REG_PKT_SNR_VALUE			0x19
#define REG_PKT_RSSI				0x1A
#define REG_MODEM_CONFIG1           0x1D
#define REG_MODEM_CONFIG2           0x1E
#define REG_SYMB_TIMEOUT_LSB  		0x1F

#define REG_PAYLOAD_LENGTH          0x22
#define REG_MAX_PAYLOAD_LENGTH 		0x23
#define REG_HOP_PERIOD              0x24
#define REG_MODEM_CONFIG3           0x26

#define REG_INVERTIQ				0x33
#define REG_DET_TRESH				0x37				// SF6
#define REG_SYNC_WORD				0x39

#define REG_DIO_MAPPING_1           0x40
#define REG_DIO_MAPPING_2           0x41
#define REG_VERSION	  				0x42

#define REG_PADAC					0x5A
#define REG_PADAC_SX1272			0x5A
#define REG_PADAC_SX1276			0x4D

// ----------------------------------------
// Used by REG_PAYLOAD_LENGTH to set receive patyload length
#define PAYLOAD_LENGTH              0x40

// ----------------------------------------
// opModes
#define SX72_MODE_SLEEP             0x80
#define SX72_MODE_STANDBY           0x81
#define SX72_MODE_FSTX              0x82
#define SX72_MODE_TX                0x83	// 0x80 | 0x03
#define SX72_MODE_RX_CONTINUOS      0x85

// ----------------------------------------
// LMIC Constants for radio registers
#define OPMODE_LORA      			0x80
#define OPMODE_MASK      			0x07
#define OPMODE_SLEEP     			0x00
#define OPMODE_STANDBY   			0x01
#define OPMODE_FSTX      			0x02
#define OPMODE_TX        			0x03
#define OPMODE_FSRX      			0x04
#define OPMODE_RX        			0x05
#define OPMODE_RX_SINGLE 			0x06
#define OPMODE_CAD       			0x07



// ----------------------------------------
// LOW NOISE AMPLIFIER

#define LNA_MAX_GAIN                0x23
#define LNA_OFF_GAIN                0x00
#define LNA_LOW_GAIN		    	0x20

// CONF REG
#define REG1                        0x0A
#define REG2                        0x84

// ----------------------------------------
// MC1 sx1276 RegModemConfig1
#define SX1276_MC1_BW_125           0x70
#define SX1276_MC1_BW_250           0x80
#define SX1276_MC1_BW_500           0x90
#define SX1276_MC1_CR_4_5           0x02
#define SX1276_MC1_CR_4_6           0x04
#define SX1276_MC1_CR_4_7           0x06
#define SX1276_MC1_CR_4_8           0x08
#define SX1276_MC1_IMPLICIT_HEADER_MODE_ON  0x01

#define SX72_MC1_LOW_DATA_RATE_OPTIMIZE     0x01 	// mandated for SF11 and SF12

// ----------------------------------------
// MC2 definitions
#define SX72_MC2_FSK                0x00
#define SX72_MC2_SF7                0x70		// SF7 == 0x07, so (SF7<<4) == SX7_MC2_SF7
#define SX72_MC2_SF8                0x80
#define SX72_MC2_SF9                0x90
#define SX72_MC2_SF10               0xA0
#define SX72_MC2_SF11               0xB0
#define SX72_MC2_SF12               0xC0

// ----------------------------------------
// MC3
#define SX1276_MC3_LOW_DATA_RATE_OPTIMIZE  0x08
#define SX1276_MC3_AGCAUTO                 0x04

// ----------------------------------------
// FRF
#define REG_FRF_MSB					0x06
#define REG_FRF_MID					0x07
#define REG_FRF_LSB					0x08

#define FRF_MSB						0xD9		// 868.1 Mhz
#define FRF_MID						0x06
#define FRF_LSB						0x66

// ----------------------------------------
// DIO function mappings           		     D0D1D2D3
#define MAP_DIO0_LORA_RXDONE   		0x00  // 00------
#define MAP_DIO0_LORA_TXDONE   		0x40  // 01------
#define MAP_DIO1_LORA_RXTOUT   		0x00  // --00----
#define MAP_DIO1_LORA_NOP      		0x30  // --11----
#define MAP_DIO2_LORA_NOP      		0xC0  // ----11--

#define MAP_DIO0_FSK_READY     		0x00  // 00------ (packet sent / payload ready)
#define MAP_DIO1_FSK_NOP       		0x30  // --11----
#define MAP_DIO2_FSK_TXNOP     		0x04  // ----01--
#define MAP_DIO2_FSK_TIMEOUT   		0x08  // ----10--

// ----------------------------------------
// Bits masking the corresponding IRQs from the radio
#define IRQ_LORA_RXTOUT_MASK 		0x80
#define IRQ_LORA_RXDONE_MASK 		0x40
#define IRQ_LORA_CRCERR_MASK 		0x20
#define IRQ_LORA_HEADER_MASK 		0x10
#define IRQ_LORA_TXDONE_MASK 		0x08
#define IRQ_LORA_CDDONE_MASK 		0x04
#define IRQ_LORA_FHSSCH_MASK 		0x02
#define IRQ_LORA_CDDETD_MASK 		0x01


// ----------------------------------------
// Definitions for UDP message arriving from server
#define PROTOCOL_VERSION			0x01
#define PKT_PUSH_DATA				0x00
#define PKT_PUSH_ACK				0x01
#define PKT_PULL_DATA				0x02
#define PKT_PULL_RESP				0x03
#define PKT_PULL_ACK				0x04

#define MGT_RESET					0x15	// Not a LoRa Gateway Spec message
#define MGT_SET_SF					0x16
#define MGT_SET_FREQ				0x17

