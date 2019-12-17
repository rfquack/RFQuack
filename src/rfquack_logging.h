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

#ifndef rfquack_logging_h
#define rfquack_logging_h

#include <stdio.h>

#include "rfquack_common.h"

void printTimestamp(Print *_logOutput) {
  char c[20];
  sprintf(c, "[RFQK] %10lu ", millis());
  _logOutput->print(c);
}

void printNewline(Print *_logOutput) { _logOutput->print('\n'); }

/*
 * If the board has extra hardware serials in addition to the default hardware
 * serial port that is connected to the USB/Serial converter, we will use that
 * on the default pins. We do this when RFQUACK_LOG_SS_DISABLE (disable the
 * software serial logging output) is defined, which means that the user would
 * like to use the hardware serial to get the logging output.
 *
 * Else, if the software serial pins are defined and RFQUACK_LOG_SS_DISABLED is
 * not defined, then the user wants to use them to receive serial logging data.
 *
 * Otherwise, if the primary hardware serial (that is usually connected to the
 * USB/Serial converter) is not taken by the transport, then we use that one
 * for logging.
 *
 * If none of these cases apply, we just redirect the logging output to a
 * software serial with default pins as defined in config/logging.h
 *
 * Overall the user can have:
 *
 *  - serial transport (via the USB/Serial converter port), plus logging via:
 *    - via software serial: they must not define RFQUACK_LOG_SS_DISABLED
 *    - via hardware serial: they must define RFQUACK_LOG_SS_DISABLED and have a board that has a Serial1 hardware serial
 *
 *  - other (non-serial) transport, they will get the logging via the USB/Serial converter port
 */
#if !defined(RFQUACK_LOG_SS_DISABLED)
#include <SoftwareSerial.h>
SoftwareSerial LogPrinter(RFQUACK_LOG_SS_RX_PIN, RFQUACK_LOG_SS_TX_PIN, false,
                          RFQUACK_LOG_SS_BLK_SIZE);
#elif !defined(RFQUACK_TRANSPORT_SERIAL)
#define LogPrinter Serial // Main hardware serial is free for logging
#elif defined(RFQUACK_TRANSPORT_SERIAL)
#define LogPrinter Serial1 // Secondary hardware serial is used for logging
#endif

#ifdef RFQUACK_LOG_ENABLED
#define RFQUACK_LOG_TRACE(...) {     Log.trace(__VA_ARGS__); }
#else
#define RFQUACK_LOG_TRACE(...) {}
#endif

void rfquack_logging_setup() {
  LogPrinter.begin(RFQUACK_LOG_PRINTER_BAUD_RATE, SERIAL_8N1); //, 32,33);


  while (!LogPrinter)
    ;

  Log.begin(RFQUACK_LOG_LEVEL, &LogPrinter);
  Log.setPrefix(printTimestamp);
  Log.setSuffix(printNewline);
}

/*
 * Print raw data to HEX string for debugging.
 */
void rfquack_log_buffer(const char * prompt, const uint8_t *buf, const uint32_t len) {
  printTimestamp(&LogPrinter);
  LogPrinter.print(prompt);

  char octect[4];

  for (uint16_t i = 0; i < len; i++) {
    sprintf(octect, "%.2X", buf[i]);

    if (i % 16 == 15)
      LogPrinter.println(octect);
    else {
      LogPrinter.print(octect);
      LogPrinter.print(' ');
    }
  }

  LogPrinter.println("");
}

void rfquack_log_packet(rfquack_Packet *pkt) {
  rfquack_log_buffer("Packet = ", pkt->data.bytes, pkt->data.size);
}

#endif
