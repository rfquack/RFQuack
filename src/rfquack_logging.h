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
  char c[17];
  sprintf(c, "[RFQ] %10lu ", millis());
  _logOutput->print(c);
}

void printNewline(Print *_logOutput) { _logOutput->print('\n'); }

/*
 * If the software serial pins are defined, then the user wants to use them to
 * receive serial logging data.
 *
 * Otherwise, if the transport is not already using the hardware serial, then
 * we use that for logging.
 *
 * In neither of these cases, we just redirect logging to a software serial with
 * default pins as defined in config/logging.h
 */
#if !defined(RFQUACK_LOG_SS_DISABLED)
#include <SoftwareSerial.h>
SoftwareSerial LogPrinter(RFQUACK_LOG_SS_RX_PIN, RFQUACK_LOG_SS_TX_PIN, false,
                          RFQUACK_LOG_SS_BLK_SIZE);
#elif !defined(RFQUACK_TRANSPORT_SERIAL)
#define LogPrinter Serial // Hardware serial
#endif

void rfquack_logging_setup() {
  LogPrinter.begin(RFQUACK_LOG_PRINTER_BAUD_RATE);

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
