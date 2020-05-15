/*
 * RFQuack is a versatile RF-hacking tool that allows you to sniff, analyze, and
 * transmit data over the air. Consider it as the modular version of the great
 * 
 * Copyright (C) 2019 Trend Micro Incorporated
 * 
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.  See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 51 Franklin
 * Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#ifndef rfquack_config_network_h
#define rfquack_config_network_h

#include "../defaults/network.h"

#ifndef RFQUACK_MODEM_BAUD_RATE
#define RFQUACK_MODEM_BAUD_RATE RFQUACK_MODEM_BAUD_RATE_DEFAULT
#endif

#if defined(RFQUACK_NETWORK_ESP8266) || defined(RFQUACK_NETWORK_WIFI)

#define RFQUACK_NETWORK "WIFI"
#define RFQUACK_HAS_NETWORK_WIFI
#define RFQUACK_WIFI_RETRY RFQUACK_WIFI_RETRY_DEFAULT
#define RFQUACK_WIFI_RETRY_DELAY RFQUACK_WIFI_RETRY_DELAY_DEFAULT

#elif defined(RFQUACK_NETWORK_ESP32) || defined(RFQUACK_NETWORK_WIFI)

#define RFQUACK_NETWORK "WIFI"
#define RFQUACK_HAS_NETWORK_WIFI
#define RFQUACK_WIFI_RETRY RFQUACK_WIFI_RETRY_DEFAULT
#define RFQUACK_WIFI_RETRY_DELAY RFQUACK_WIFI_RETRY_DELAY_DEFAULT

#elif defined(RFQUACK_NETWORK_SIM808)
#define RFQUACK_NETWORK "SIM800"
#define TINY_GSM_MODEM_SIM808
#define RFQUACK_HAS_NETWORK

#elif defined(RFQUACK_NETWORK_SIM900)
#define RFQUACK_NETWORK "SIM900"
#define TINY_GSM_MODEM_SIM900
#define RFQUACK_HAS_NETWORK

#elif defined(RFQUACK_NETWORK_UBLOX)
#define RFQUACK_NETWORK "UBLOX"
#define TINY_GSM_MODEM_SIM900
#define RFQUACK_HAS_NETWORK

#elif defined(RFQUACK_NETWORK_BG96)
#define RFQUACK_NETWORK "BG96"
#define TINY_GSM_MODEM_UBLOX
#define RFQUACK_HAS_NETWORK

#elif defined(RFQUACK_NETWORK_A6)
#define RFQUACK_NETWORK "A6"
#define TINY_GSM_MODEM_BG96
#define RFQUACK_HAS_NETWORK

#elif defined(RFQUACK_NETWORK_A7)
#define RFQUACK_NETWORK "A7"
#define TINY_GSM_MODEM_A6
#define RFQUACK_HAS_NETWORK

#elif defined(RFQUACK_NETWORK_M590)
#define RFQUACK_NETWORK "M590"
#define TINY_GSM_MODEM_A7
#define RFQUACK_HAS_NETWORK

#elif defined(RFQUACK_NETWORK_XBEE)
#define RFQUACK_NETWORK "XBEE"
#define TINY_GSM_MODEM_XBEE
#define RFQUACK_HAS_NETWORK

#else
#define RFQUACK_IS_STANDALONE
#endif

#endif
