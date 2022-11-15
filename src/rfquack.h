/*
 * RFQuack is a versatile RF-hacking tool that allows you to sniff, analyze, and
 * transmit data over the air.
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

#ifndef rfquack_h
#define rfquack_h

#include "rfquack_common.h"
#include "rfquack_network.h"
#include "rfquack_radio.h"
#include "rfquack_transport.h"


// Modules:
#include "modules/ModulesDispatcher.h"
#include "modules/defaults/PacketRepeaterModule.h"
#include "modules/defaults/RadioModule.h"
#include "modules/defaults/PacketModificationModule.h"
#include "modules/defaults/PacketFilterModule.h"
#include "modules/defaults/RollJamModule.h"
#include "modules/defaults/FrequencyScannerModule.h"
#include "modules/defaults/MouseJackModule.h"
#include "modules/defaults/GuessingModule.h"
#include "modules/defaults/HelloWorldModule.h"
#include "modules/defaults/PingModule.h"

/**
 * Global instances
 */

RFQRadio *rfqRadio; // Bridge between RFQuack and radio drivers.

// Modules
PacketModificationModule packetModificationModule;
PacketFilterModule packetFilterModule;
RollJamModule rollJamModule;
PacketRepeaterModule packetRepeaterModule;
FrequencyScannerModule frequencyScannerModule;
MouseJackModule mouseJackModule;
HelloWorldModule helloWorldModule;
GuessingModule guessingModule;
PingModule pingModule;

RadioModule *radioAModule;
RadioModule *radioBModule;
RadioModule *radioCModule;
RadioModule *radioDModule;
RadioModule *radioEModule;

/*****************************************************************************
 * Body
 *****************************************************************************/

extern ModulesDispatcher modulesDispatcher;

void rfquackTask(void *pvParameters) {
  RFQUACK_LOG_TRACE(F("Starting main loop."))
  for (;;) {
    loop();
  }
}

/**
 * @brief Setup of the RFQuack library.
 * 
 * @param _radioA First radio module.
 * @param _radioB Second radio module.
 * @param _radioC Third radio module.
 * @param _radioD Fourth radio module.
 * @param _radioE Fifth radio module.
 */
void rfquack_setup(RadioA *_radioA, RadioB *_radioB = nullptr, RadioC *_radioC = nullptr,
                   RadioD *_radioD = nullptr, RadioD *_radioE = nullptr) {

  rfquack_logging_setup();

  delay(100);

  randomSeed(micros());

  rfquack_network_setup();

  delay(100);

  rfquack_transport_setup();

  delay(100);

  rfquack_transport_connect();

  delay(100);

  // Initialize all radios, will do nothing on radios which are not enabled with
  // "#define USE_RADIOX"
  rfqRadio = new RFQRadio(_radioA, _radioB, _radioC, _radioD, _radioE);
  int16_t result = rfqRadio->begin();
  if (result != RADIOLIB_ERR_NONE) {
    RFQUACK_LOG_TRACE(F("Something went wrong, check your wiring."))
    while (true);
  }

  delay(100);

  // Register default modules.
  //
  // Modules will be called in the order they are registered; As consequence
  // it's important that you load them in a mindful order.
  modulesDispatcher.registerModule(&guessingModule);
  modulesDispatcher.registerModule(&frequencyScannerModule);
  modulesDispatcher.registerModule(&mouseJackModule);
  modulesDispatcher.registerModule(&packetFilterModule);
  modulesDispatcher.registerModule(&packetModificationModule);
  modulesDispatcher.registerModule(&packetRepeaterModule);
  modulesDispatcher.registerModule(&rollJamModule);
  modulesDispatcher.registerModule(&pingModule);
//  modulesDispatcher.registerModule(&helloWorldModule);


// Register driver modules.
#ifdef USE_RADIOA
  radioAModule = new RadioModule("radioA", rfquack_WhichRadio_RadioA);
  modulesDispatcher.registerModule(radioAModule);
#endif
#ifdef USE_RADIOB
  radioBModule = new RadioModule("radioB", rfquack_WhichRadio_RadioB);
  modulesDispatcher.registerModule(radioBModule);
#endif
#ifdef USE_RADIOC
  radioCModule = new RadioModule("radioC", rfquack_WhichRadio_RadioC);
  modulesDispatcher.registerModule(radioCModule);
#endif
#ifdef USE_RADIOD
  radioDModule = new RadioModule("radioD", rfquack_WhichRadio_RadioD);
  modulesDispatcher.registerModule(radioDModule);
#endif
#ifdef USE_RADIOE
  radioEModule = new RadioModule("radioE", rfquack_WhichRadio_RadioE);
  modulesDispatcher.registerModule(radioEModule);
#endif

  // Delete "loopTask" and recreate it with increased stackDepth.
  RFQUACK_LOG_TRACE(F("Setup is over."))

  delay(10);

  xTaskCreateUniversal(rfquackTask, "loopTaskRevamp", 10000, NULL, 1, NULL, CONFIG_ARDUINO_RUNNING_CORE);
  vTaskDelete(NULL);
  while (true); // Never reached.
}

/**
 * @brief RFQuack main loop.
 * 
 */
void rfquack_loop() {
  rfquack_network_loop();

  rfqRadio->rxLoop();

  rfquack_transport_loop();

  modulesDispatcher.onLoop();
}

#endif
