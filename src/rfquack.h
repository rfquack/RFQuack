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

#include <modules/defaults/PacketRepeater.h>
#include "rfquack_common.h"
#include "rfquack_network.h"
#include "rfquack_radio.h"
#include "rfquack_transport.h"
#include "modules/ModulesDispatcher.h"

// Modules:
#include "modules/defaults/RadioModule.h"
#include "modules/defaults/PacketModificationModule.h"
#include "modules/defaults/PacketFilterModule.h"
#include "modules/defaults/RollJamModule.h"

/**
 * Global instances
 */

RFQRadio *rfqRadio; // Bridge between RFQuack and radio drivers.


RadioModule driverConfigModule("radioA", rfquack_WhichRadio_RADIOA);
PacketModificationModule packetModificationModule;
PacketFilterModule packetFilterModule;
RollJamModule rollJamModule;
PacketRepeaterModule packetRepeaterModule;


/*****************************************************************************
 * Body
 *****************************************************************************/

extern ModulesDispatcher modulesDispatcher;

void rfquack_setup(RadioA &radioA
#ifndef RFQUACK_SINGLE_RADIO
  , RadioB &radioB
#endif
) {
  rfquack_logging_setup();

  delay(100);

  randomSeed(micros());

  rfquack_network_setup();

  delay(100);

  rfquack_transport_setup();

  delay(100);

  rfquack_transport_connect();

  delay(100);

#ifdef RFQUACK_SINGLE_RADIO
  rfqRadio = new RFQRadio(&radioA);
  rfqRadio->begin(rfquack_WhichRadio_RADIOA);
#else
  rfqRadio = new RFQRadio(&radioA, &radioB);
  rfqRadio->begin(rfquack_WhichRadio_RADIOA);
  rfqRadio->begin(rfquack_WhichRadio_RADIOB);
#endif

  delay(100);

  // Register modules
  modulesDispatcher.registerModule(packetFilterModule);
  modulesDispatcher.registerModule(packetModificationModule);
  modulesDispatcher.registerModule(packetRepeaterModule);
  modulesDispatcher.registerModule(rollJamModule);
  modulesDispatcher.registerModule(driverConfigModule);

  delay(100);
}

void rfquack_loop() {
  rfquack_network_loop();

  rfqRadio->rxLoop();

  rfquack_transport_loop();
}

#endif
