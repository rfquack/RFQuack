/*
 * RFQuack is a versatile RF-hacking tool that allows you to sniff, analyze, and
 * transmit data over the air.Consider it as the modular version of the great
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
#include "modules/defaults/PacketRepeater.h"
#include "modules/defaults/RadioModule.h"
#include "modules/defaults/PacketModificationModule.h"
#include "modules/defaults/PacketFilterModule.h"
#include "modules/defaults/RollJamModule.h"

/**
 * Global instances
 */

RFQRadio *rfqRadio; // Bridge between RFQuack and radio drivers.

// Modules
PacketModificationModule packetModificationModule;
PacketFilterModule packetFilterModule;
RollJamModule rollJamModule;
PacketRepeaterModule packetRepeaterModule;
RadioModule *radioAModule;
RadioModule *radioBModule;
RadioModule *radioCModule;
RadioModule *radioDModule;
RadioModule *radioEModule;

/*****************************************************************************
 * Body
 *****************************************************************************/

extern ModulesDispatcher modulesDispatcher;

void rfquack_setup(RadioA &_radioA, RadioB &_radioB, RadioC &_radioC, RadioD &_radioD, RadioE &_radioE) {
  rfquack_logging_setup();

  delay(100);

  randomSeed(micros());

  rfquack_network_setup();

  delay(100);

  rfquack_transport_setup();

  delay(100);

  rfquack_transport_connect();

  delay(100);

  // Initialize all radios, will do nothing on radios which are not enabled with "#define USE_RADIOX"
  rfqRadio = new RFQRadio(&_radioA, &_radioB, &_radioC, &_radioD, &_radioE);
  rfqRadio->begin();

  delay(100);

  // Register default modules
  modulesDispatcher.registerModule(&packetFilterModule);
  modulesDispatcher.registerModule(&packetModificationModule);
  modulesDispatcher.registerModule(&packetRepeaterModule);
  modulesDispatcher.registerModule(&rollJamModule);

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

  delay(100);
}

void rfquack_loop() {
  rfquack_network_loop();

  rfqRadio->rxLoop();

  rfquack_transport_loop();
}

#endif
