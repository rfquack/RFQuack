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

#define LogPrinter Serial // Secondary hardware serial is used for logging

#ifdef RFQUACK_LOG_ENABLED
#define RFQUACK_LOG_TRACE(...) {     Log.trace(__VA_ARGS__); }
#define RFQUACK_LOG_ERROR(...) {     Log.error(__VA_ARGS__); }
#define RFQUACK_LOG_WARN(...) {     Log.warning(__VA_ARGS__); }
#define RFQUACK_LOG_FATAL(...) {     Log.fatal(__VA_ARGS__); }
#else
#define RFQUACK_LOG_TRACE(...) {}
#define RFQUACK_LOG_WARN(...) {}
#define RFQUACK_LOG_ERROR(...) {}
#define RFQUACK_LOG_FATAL(...) {}
#endif

void rfquack_logging_setup() {
#ifdef RFQUACK_LOG_ENABLED
  LogPrinter.begin(RFQUACK_LOG_PRINTER_BAUD_RATE, SERIAL_8N1); //, 32,33);

  while (!LogPrinter)
    ;

  Log.begin(RFQUACK_LOG_LEVEL, &LogPrinter);
  Log.setPrefix(printTimestamp);
  Log.setSuffix(printNewline);
#endif
}

/*
 * Print raw data to HEX string for debugging.
 */
void rfquack_log_buffer(const char * prompt, const uint8_t *buf, const uint32_t len) {
  printTimestamp(&LogPrinter);
  LogPrinter.println(prompt);

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
#ifdef RFQUACK_LOG_ENABLED
  rfquack_log_buffer("Packet = ", pkt->data.bytes, pkt->data.size);
#endif
}

#endif
