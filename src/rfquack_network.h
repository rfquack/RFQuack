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
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 51
 * Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef rfquack_network_h
#define rfquack_network_h

#include "rfquack_common.h"
#include "rfquack_logging.h"

/*****************************************************************************
 * Function signatures
 *****************************************************************************/
#if defined(RFQUACK_HAS_NETWORK) && !defined(RFQUACK_HAS_NETWORK_WIFI)
#define SerialAT Serial1

#include <TinyGsmClient.h>

/*****************************************************************************
 * Variables
 *****************************************************************************/
TinyGsm rfquack_modem(SerialAT);
TinyGsmClient rfquack_net(rfquack_modem);

TinyGsmClient * rfquack_network_client() {
  return &rfquack_net;
}

/*
 * Initialise the network connection (modem or wifi)
 */
void rfquack_network_setup() {
  // TODO replace with software serial?
  SerialAT.begin(RFQUACK_MODEM_BAUD_RATE);

  delay(10);

  // Restart takes quite some time
  // To skip it, call init() instead of restart()
  Log.trace("Initializing cellular modem...");
  rfquack_modem.restart();

  String modemInfo = rfquack_modem.getModemInfo();
  Log.trace("Cellular modem: %s", modemInfo.c_str());

#if defined(RFQUACK_SIM_PIN)
  rfquack_modem.simUnlock(RFQUACK_SIM_PIN);
#endif

  Log.trace("Waiting for cellular network...");

  if (!rfquack_modem.waitForNetwork()) {
    Log.error("Cellular network setup failed");
    while (true)
      ;
  }
  rfquack_modem.gprsConnect(RFQUACK_NETWORK_APN, RFQUACK_NETWORK_USER,
                            RFQUACK_NETWORK_PASS);

  Log.trace("Connected to cellular network");
}

#elif defined(RFQUACK_HAS_NETWORK_WIFI) || defined(TINY_GSM_MODEM_HAS_WIFI)

#if defined(RFQUACK_NETWORK_ESP8266)
#include <ESP8266WiFi.h>
#else
#include <WiFi.h>
#endif

WiFiClient rfquack_net;

/*****************************************************************************
 * Variables
 *****************************************************************************/
unsigned int rfquack_wifi_retry = 0;

#if !defined(RFQUACK_NETWORK_SSID) || !defined(RFQUACK_NETWORK_PASS)
#error "You must set both RFQUACK_NETWORK_SSID and RFQUACK_NETWORK_PASS"
#endif

WiFiClient * rfquack_network_client() {
  return &rfquack_net;
}

void rfquack_network_connect() {
  while (WiFi.status() != WL_CONNECTED) {
    Log.error("WiFi not connected: %d (retry: %d)", WiFi.status(),
              ++rfquack_wifi_retry);
		//WiFi.printDiag(Serial);
    delay(RFQUACK_WIFI_RETRY_DELAY);
  }
}

/*
 * Initialise the network connection
 */
void rfquack_network_setup() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(RFQUACK_NETWORK_SSID, RFQUACK_NETWORK_PASS);
}

void rfquack_network_loop() {
  if (WiFi.status() != WL_CONNECTED)
    rfquack_network_connect();
}

#elif defined(RFQUACK_IS_STANDALONE)

void rfquack_network_setup() {
  Log.trace("Standalone mode network setup: done!");
}

void rfquack_network_loop() {}

#else
#error "Network configuration unexpected"
#endif

#endif
