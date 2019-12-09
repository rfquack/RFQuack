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

// protobuf
#include "pb.h"
#include "pb_decode.h"
#include "pb_encode.h"
#include "rfquack.pb.h"

// regex
#include "utils/re.h"

// configuration
#include "rfquack_config.h"
#include "rfquack_logging.h"

// Utility
#define PB_ENCODE_AND_SEND(fields, data, topic){ \
  uint8_t buf[RFQUACK_MAX_PB_MSG_SIZE]; \
  pb_ostream_t ostream = pb_ostream_from_buffer(buf, RFQUACK_MAX_PB_MSG_SIZE); \
  if (!pb_encode(&ostream, fields, &(data))) { \
    Log.error("Encoding " #fields " failed: %s", PB_GET_ERROR(&ostream)); \
  } else { \
    if (!rfquack_transport_send(topic, buf, ostream.bytes_written)) \
      Log.error(F("Failed sending to transport " #topic)); \
  } \
}

#define PB_DECODE(pkt, fields, payload, payload_length) { \
  pb_istream_t istream = pb_istream_from_buffer((uint8_t *) payload, payload_length); \
  if (!pb_decode(&istream, fields, &(pkt))) { \
    Log.error("Cannot decode fields: " #fields ", Packet: %s", PB_GET_ERROR(&istream)); \
    return; \
  } \
}

/*****************************************************************************
 * Types
 *****************************************************************************/

typedef void (*RFQuackCommandDispatcher)(char *, char *, int);

/**
 * @brief Array of packet modification rules along with compiled patterns
 */
typedef struct packet_modifications {
  /**
   * @brief set of packet modifications
   */
  rfquack_PacketModification rules[RFQUACK_MAX_PACKET_MODIFICATIONS];

  /**
   * @brief Pre-compiled patterns, one per rule
   */
  re_t patterns[RFQUACK_MAX_PACKET_MODIFICATIONS];

  /**
   * @brief Number of usable rules
   */
  uint8_t size = 0;
} packet_modifications_t;

/**
 * @brief Array of packet-filtering rules along with compiled patterns
 */
typedef struct packet_filters {
  /**
   * @brief set of packet filters
   */
  rfquack_PacketFilter filters[RFQUACK_MAX_PACKET_FILTERS];

  /**
   * @brief Pre-compiled patterns, one per rule
   */
  re_t patterns[RFQUACK_MAX_PACKET_FILTERS];

  /**
   * @brief Number of usable rules
   */
  uint8_t size = 0;
} packet_filters_t;

/*****************************************************************************
 * Variables
 *****************************************************************************/

extern rfquack_Status rfq;
extern packet_modifications_t pms;
extern packet_filters_t pfs;

/*****************************************************************************
 * Functions
 *****************************************************************************/

// /**
//  * @brief Check if packet matches a regular expression pattern
//  *
//  * @param pattern Compiled regular expression pattern.
//  * @param pkt Packet instance pointer.
//  *
//  * @return True if matches. False otherwise.
//  */
// static bool rfquack_packet_matches_p(re_t cp, rfquack_Packet *pkt) {
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

/**
 * @brief Check if packet matches an uncompiled regular expression pattern
 *
 * @param pattern Regular expression pattern as a null-terminated string.
 * @param pkt Packet instance pointer.
 *
 * @return True if matches. False otherwise.
 */
static bool rfquack_packet_matches(char *pattern, rfquack_Packet *pkt) {
  char str[RFQUACK_RADIO_MAX_MSG_LEN + 1];

  // copy octects in memory area and null terminate the string
  memcpy(str, pkt->data.bytes, pkt->data.size);
  str[pkt->data.size] = '\0';

#ifdef RFQUACK_DEV
  Log.verbose("Matching pattern '%s' (len: %d)", pattern, strlen(pattern));
#endif

  int m = re_match(pattern, str);

#ifdef RFQUACK_DEV
  Log.verbose("re_match('%s', '%s') = %d", pattern, str, m);
#endif

  return m != -1;
}

/**
 * @brief Reset packet modifications
 */
void rfquack_packet_modification_init() {
  Log.trace("Packet modification data initialized");
  pms.size = 0;
}

/**
 * @brief Reset packet filters
 */
void rfquack_packet_filter_init() {
  Log.trace("Packet filtering data initialized");
  pfs.size = 0;
}

/**
 * @brief Checks if a packet matches all the filters
 *
 * @param pkt Pointer to packet
 *
 * @return Whether the packet matches all the filters
 */
bool rfquack_packet_filter(rfquack_Packet *pkt) {
  if (pfs.size == 0) {
#ifdef RFQUACK_DEV
    Log.verbose("No filters");
#endif
    return true;
  }

  for (uint8_t i = 0; i < pfs.size; i++)
    if (!rfquack_packet_matches(pfs.filters[i].pattern, pkt)) {
      //
      // TODO understand why matching against a pre-compiled pattern fails
      // if (!rfquack_packet_matches_p(pfs.patterns[i], pkt)) {
      //

#ifdef RFQUACK_DEV
      rfquack_log_packet(pkt);
      Log.trace("Packet doesn't match pattern '%s'", pfs.filters[i].pattern);
#endif

      return false;
    }

#ifdef RFQUACK_DEV
    rfquack_log_packet(pkt);
    Log.verbose("Packet matches all %d patterns", pfs.size);
#endif

  return true;
}

/**
 * @brief Interpret a packet modification and apply it to a packet.
 *
 * @param idx Packet modification index;
 */
static void rfquack_apply_packet_modification(uint8_t idx,
                                              rfquack_Packet *pkt) {

  rfquack_PacketModification rule = pms.rules[idx];

  //
  // TODO understand why matching against a pre-compiled pattern fails
  // re_t cp = pms.patterns[idx];
  //
  // if (rule.has_pattern && !rfquack_packet_matches_p(cp, pkt)) {
  //
  if (rule.has_pattern && !rfquack_packet_matches(rule.pattern, pkt)) {
    return;
  }

  // either packet matches the signature or there's no signature (*)

  Log.verbose("Applying packet modification rule #%d", idx);

#ifdef RFQUACK_DEV
  rfquack_log_packet(pkt);
#endif

  // for all octects
  for (uint32_t i = 0; i < pkt->data.size; i++) {
    uint8_t octect = pkt->data.bytes[i];

    if (
      // Case 1: we have a position only, and it matches
      (!rule.has_content && rule.has_position && rule.position == i) ||

      // Case 2: we have both position and value, and both match
      (rule.has_content && rule.has_position && rule.content == octect &&
       rule.position == i) ||

      // Case 3: we have only value, and matches
      (rule.has_content && !rule.has_position && rule.content == octect)) {

      // Value assignment
      if (!rule.has_operation && rule.has_operand) {
        pkt->data.bytes[i] = rule.operand;

      } else {
        if (rule.has_operand)
          switch (rule.operation) {
          // packet[position] = packet[position] & operand
          case rfquack_PacketModification_Op_AND:
            pkt->data.bytes[i] &= rule.operand;
            break;

            // packet[position] = packet[position] | operand
          case rfquack_PacketModification_Op_OR:
            pkt->data.bytes[i] |= rule.operand;
            break;

            // packet[position] = packet[position] ^ operand
          case rfquack_PacketModification_Op_XOR:
            pkt->data.bytes[i] ^= rule.operand;
            break;

            // packet[position] = packet[position] << operand
          case rfquack_PacketModification_Op_SLEFT:
            pkt->data.bytes[i] <<= rule.operand;
            break;

            // packet[position] = packet[position] >> operand
          case rfquack_PacketModification_Op_SRIGHT:
            pkt->data.bytes[i] >>= rule.operand;
            break;

          case rfquack_PacketModification_Op_NOT:
            // doesn't make a lot of sense, but let's handle this case
            // so the compiler is hapy and doesn't throw a warning
            pkt->data.bytes[i] = ~rule.operand;
            break;
          }
        else
            // packet[position] = ~packet[position]
            if (rule.operation == rfquack_PacketModification_Op_NOT)
          pkt->data.bytes[i] = ~pkt->data.bytes[i];
      }
    }
  }

#ifdef RFQUACK_DEV
    rfquack_log_packet(pkt);
#endif
}

/**
 * @brief Apply packet modifications to a packet
 *
 * We're not responsible for weird packet modification combos: we'll just apply
 * them. So, write those wisely 😉
 */
void rfquack_apply_packet_modifications(rfquack_Packet *pkt) {
  // for each packet modification rule in positional order
  for (uint8_t i = 0; i < pms.size; i++)
    rfquack_apply_packet_modification(i, pkt);
}

#endif
