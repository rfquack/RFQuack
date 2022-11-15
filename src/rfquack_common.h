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

#ifndef rfquack_common_h
#define rfquack_common_h

// Arduino layer
#include <Arduino.h>

// Config constants
#include "rfquack_config.h"

// Protobuf

#include "pb.h"
#include "pb_decode.h"
#include "pb_encode.h"
#include <rfquack.pb.h>

// Regex
#include "utils/regex/re.h"
#include "rfquack_logging.h"

extern uint32_t rfquack_transport_send(const char *topic, const uint8_t *data, uint32_t len);


#define PB_ENCODE_AND_SEND(pbStruct, data, verb, moduleName, cmdValue) { \
  char topic[RFQUACK_MAX_TOPIC_LEN] = RFQUACK_OUT_TOPIC RFQUACK_TOPIC_SEP verb RFQUACK_TOPIC_SEP; \
  strcat(topic, moduleName); \
  strcat(topic, RFQUACK_TOPIC_SEP #pbStruct  RFQUACK_TOPIC_SEP cmdValue); \
  uint8_t buf[RFQUACK_MAX_PB_MSG_SIZE]; \
  pb_ostream_t ostream = pb_ostream_from_buffer(buf, RFQUACK_MAX_PB_MSG_SIZE); \
  if (!pb_encode(&ostream, pbStruct ##  _fields, &(data))) { \
    Log.error("Encoding " #pbStruct " failed: %s", PB_GET_ERROR(&ostream)); \
  } else { \
    if (!rfquack_transport_send(topic, buf, ostream.bytes_written)) \
      Log.error(F("Failed sending to transport ")); \
  } \
}

// Regex common

/**
 * @brief Check if packet matches an uncompiled regular expression pattern
 *
 * @param pattern Regular expression pattern as a null-terminated string.
 * @param pkt Packet instance pointer.
 *
 * @return True if matches. False otherwise.
 */
bool rfquack_packet_matches(char *pattern, rfquack_Packet *pkt) {
  char str[RFQUACK_RADIO_MAX_MSG_LEN * 2 + 2];

  // copy octects in memory area and null terminate the string
  for (uint16_t i = 0; i < pkt->data.size; i++) {
    sprintf(&str[i * 2], "%.2x", pkt->data.bytes[i]);
  }
  str[pkt->data.size * 2 + 2] = '\0';

  RFQUACK_LOG_TRACE(F("Matching pattern '%s' (len: %d)"), pattern, strlen(pattern));
  int m = re_match(pattern, str);
  RFQUACK_LOG_TRACE(F("re_match('%s', '%s') = %d"), pattern, str, m);
  return m != -1;
}


// /**
//  * @brief Check if packet matches a regular expression pattern
//  *
//  * @param pattern Compiled regular expression pattern.
//  * @param pkt Packet instance pointer.
//  *
//  * @return True if matches. False otherwise.
//  */
//  bool rfquack_packet_matchesP(re_t cp, rfquack_Packet *pkt) {
//
//   char str[RFQUACK_RADIO_MAX_MSG_LEN + 1];
//
//   // copy octects in memory area and null terminate the string
//   memcpy(str, pkt->data.bytes, pkt->data.size);
//   str[pkt->data.size] = '\0';
//
//   int m = re_matchp(cp, str);
//
//   Log.verbose("re_matchp('%s') = %d", str, m);
//
//   return m != -1;
// }

#endif
