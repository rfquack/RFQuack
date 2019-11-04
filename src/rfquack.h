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

// TODO probably some of this stuff should be moved into rfquack_transport.h

/*****************************************************************************
 * Variables
 *****************************************************************************/
rfquack_Status rfq;
packet_modifications_t pms;
packet_filters_t pfs;

/*****************************************************************************
 * Body
 *****************************************************************************/

void rfquack_update_stats() {}

/**
 * @brief Update statistics.
 */
void rfquack_update_status() {
  rfquack_update_radio_stats();
  rfquack_update_stats();
}

/*
 * Send statistics about TX/RX packets and tool state.
 */
void rfquack_send_stats() {
  rfquack_update_status();

  // The message to send is rfq.stats
  uint8_t buf[RFQUACK_MAX_PB_MSG_SIZE];
  pb_ostream_t ostream = pb_ostream_from_buffer(buf, sizeof(buf));

  if (!pb_encode(&ostream, rfquack_Stats_fields, &(rfq.stats))) {
    Log.error("Encoding failed: %s", PB_GET_ERROR(&ostream));
  } else {
    if (!rfquack_transport_send(
            RFQUACK_OUT_TOPIC RFQUACK_TOPIC_SEP RFQUACK_TOPIC_STATS, buf,
            ostream.bytes_written))
      Log.error("Failed sending to transport");
  }

  Log.trace("TX packets = %d (TX errors = %d), "
            "RX packets = %d (RX errors = %d)",
            "TX queue = %d, "
            "RX queue = %d",
            rfq.stats.tx_packets, rfq.stats.tx_failures, rfq.stats.rx_packets,
            rfq.stats.rx_failures, rfq.stats.tx_queue, rfq.stats.rx_queue);
}

/*
 * Switch tool to requested mode (RX, TX, repeat)
 */
uint32_t rfquack_change_mode(rfquack_Mode mode) {
  if (mode < _rfquack_Mode_MIN || mode > _rfquack_Mode_MAX) {
    Log.error("Invalid mode");
    return 0;
  }

  if (rfq.mode == mode)
    return 0;

  Log.trace("Mode: %d -> %d", rfq.mode, mode);

  rfq.mode = mode;

  rfquack_update_mode();

  return 1;
}

/*
 * Change TX repeat
 */
uint32_t rfquack_change_tx_repeat_default(uint32_t tx_repeat_default) {
  if (tx_repeat_default > RFQUACK_RADIO_TX_QUEUE_LEN) {
    Log.error("Cannot repeat %d times as the TX queue can hold %d packets",
              tx_repeat_default, RFQUACK_RADIO_TX_QUEUE_LEN);

    return 0;
  }

  rfq.tx_repeat_default = tx_repeat_default;

  return 1;
}

/*
 * Change modem config
 */
uint32_t rfquack_set_status(rfquack_Status *st) {
  uint32_t changes = 0;

  // modem configuration
  if (st->has_modemConfig)
    changes += rfquack_set_modem_config(&(st->modemConfig));

  // mode configuration
  if (st->has_mode)
    changes += rfquack_change_mode(st->mode);

  // TX repeat configuration
  changes += rfquack_change_tx_repeat_default(st->tx_repeat_default);

  return changes;
}

/**
 * @brief Send register value to the transport.
 *
 * @param addr Address of the register
 * @param value Value of the register
 */
void rfquack_send_register(rfquack_register_address_t addr,
                           rfquack_register_value_t value) {
  rfquack_Register reg;
  uint8_t buf[RFQUACK_MAX_PB_MSG_SIZE];
  pb_ostream_t ostream = pb_ostream_from_buffer(buf, RFQUACK_MAX_PB_MSG_SIZE);

  reg.address = addr;
  reg.value = value;
  reg.has_value = true;

  if (!pb_encode(&ostream, rfquack_Register_fields, &reg)) {
    Log.error("Encoding Register failed: %s", PB_GET_ERROR(&ostream));
  } else {
    if (!rfquack_transport_send(
            RFQUACK_OUT_TOPIC RFQUACK_TOPIC_SEP RFQUACK_TOPIC_REGISTER, buf,
            ostream.bytes_written))
      Log.error("Failed sending to transport");
  }
}

/*
 * Send tool status.
 */
void rfquack_send_status() {
  rfquack_update_status();

  // The message to send is rfq.stats
  uint8_t buf[RFQUACK_MAX_PB_MSG_SIZE];
  pb_ostream_t ostream = pb_ostream_from_buffer(buf, RFQUACK_MAX_PB_MSG_SIZE);

  if (!pb_encode(&ostream, rfquack_Status_fields, &rfq)) {
    Log.error("Encoding failed: %s", PB_GET_ERROR(&ostream));
  } else {
    if (!rfquack_transport_send(
            RFQUACK_OUT_TOPIC RFQUACK_TOPIC_SEP RFQUACK_TOPIC_STATUS, buf,
            ostream.bytes_written))
      Log.error("Failed sending to transport");
  }
}

static void rfquack_tx_packet(char *payload, int payload_length) {
  Log.trace("Request to transmit a packet");

  // init
  rfquack_Packet pkt = rfquack_Packet_init_default;

  // create stream from buffer
  pb_istream_t istream =
      pb_istream_from_buffer((uint8_t *)payload, payload_length);

  // attempt decoding
  if (!pb_decode(&istream, rfquack_Packet_fields, &pkt)) {
    Log.error("Cannot decode Packet: %s", PB_GET_ERROR(&istream));
    return;
  }

  if (!rfquack_send_packet(&pkt))
    Log.error("Failure transmitting %d bytes of payload", pkt.data.size);
}

static void rfquack_status(char *payload, int payload_length) {
  // init
  rfquack_Status status = rfquack_Status_init_default;

  // create stream from buffer
  pb_istream_t istream =
      pb_istream_from_buffer((uint8_t *)payload, payload_length);

  // attempt decoding
  if (!pb_decode(&istream, rfquack_Status_fields, &status)) {
    Log.error("Cannot decode Status: %s", PB_GET_ERROR(&istream));
    return;
  }

  uint32_t changes = rfquack_set_status(&status);

  if (changes > 0) {
    Log.trace("%d settings changed: re-initializing the radio", changes);
    rfquack_update_mode();
  } else
    Log.warning("No change in settings: doing nothing");

  return;
}

static void rfquack_modem_config_change(char *payload, int payload_length) {
  // init
  rfquack_ModemConfig mc = rfquack_ModemConfig_init_default;

  // create stream from buffer
  pb_istream_t istream =
      pb_istream_from_buffer((uint8_t *)payload, payload_length);

  // attempt decoding
  if (!pb_decode(&istream, rfquack_ModemConfig_fields, &mc)) {
    Log.error("Cannot decode ModemConfig: %s", PB_GET_ERROR(&istream));
    return;
  }

  rfquack_set_modem_config(&mc);
}

/**
 * @brief Reads register and replies with its store value.
 *
 * @param payload
 * @param payload_length
 */
static void rfquack_register_access(char *payload, int payload_length) {
  // init
  rfquack_Register reg;

  // create stream from buffer
  pb_istream_t istream =
      pb_istream_from_buffer((uint8_t *)payload, payload_length);

  if (!pb_decode(&istream, rfquack_Register_fields, &reg)) {
    Log.error("Cannot decode Register: %s", PB_GET_ERROR(&istream));

    return;
  }

  // most of the time, this is just 1 byte (uint8_t)
  rfquack_register_address_t addr = (rfquack_register_address_t)reg.address;
  rfquack_register_value_t value;

  // register set
  if (reg.has_value) {
    value = (rfquack_register_value_t)reg.value;
    uint8_t status = rfquack_write_register(addr, value);
    Log.trace("Register " RFQUACK_REGISTER_HEX_FORMAT
              " = " RFQUACK_REGISTER_VALUE_HEX_FORMAT " (status = %X)",
              addr, value, status);
  } else { // register get
    value = rfquack_read_register(addr);

    Log.trace("Reading register " RFQUACK_REGISTER_HEX_FORMAT
              " = " RFQUACK_REGISTER_VALUE_HEX_FORMAT,
              addr, value);

    rfquack_send_register(addr, value);

    return;
  }
}

/**
 * @brief Add packet-filtering rule to ruleset and compiles regex (if any).
 *
 */
static void rfquack_set_packet_filter(char *payload, int payload_length) {
  // init
  rfquack_PacketFilter pf;

  // create stream from buffer
  pb_istream_t istream =
      pb_istream_from_buffer((uint8_t *)payload, payload_length);

  if (!pb_decode(&istream, rfquack_PacketFilter_fields, &pf)) {
    Log.error("Cannot decode PacketFilter: %s", PB_GET_ERROR(&istream));

    return;
  }

  int idx = pfs.size;
  pfs.size++;

  // compile the pattern
  re_t cp = re_compile(pf.pattern);

  // add pattern to ruleset
  memcpy(&(pfs.patterns[idx]), &cp, sizeof(re_t));

  // add rule to ruleset
  memcpy(&(pfs.filters[idx]), &pf, sizeof(rfquack_PacketFilter));
  Log.trace("Adding pattern '%s' to filters", pfs.filters[idx].pattern);
}

/**
 * @brief Send a packet modification on the transport
 *
 * @param index Position in the list of packet modification list
 */
static void rfquack_send_packet_modification(uint8_t index) {
  uint8_t buf[RFQUACK_MAX_PB_MSG_SIZE];
  pb_ostream_t ostream = pb_ostream_from_buffer(buf, sizeof(buf));

  if (!pb_encode(&ostream, rfquack_PacketModification_fields,
                 &(pms.rules[index]))) {
    Log.error("Encoding failed: %s", PB_GET_ERROR(&ostream));
  } else {
    if (!rfquack_transport_send(RFQUACK_OUT_TOPIC RFQUACK_TOPIC_SEP
                                    RFQUACK_TOPIC_PACKET_MODIFICATION,
                                buf, ostream.bytes_written))
      Log.error("Failed sending to transport");
  }
}

/**
 * @brief Loop through all packet modifications and send them
 * to the nodes.
 */
static void rfquack_send_packet_modifications() {
  for (uint8_t i = 0; i < pms.size; i++) {
    rfquack_send_packet_modification(i);
  }
}

/**
 * @brief Send a packet filter on the transport
 *
 * @param index Position in the list of packet filter list
 */
static void rfquack_send_packet_filter(uint8_t index) {
  uint8_t buf[RFQUACK_MAX_PB_MSG_SIZE];
  pb_ostream_t ostream = pb_ostream_from_buffer(buf, sizeof(buf));

  if (!pb_encode(&ostream, rfquack_PacketFilter_fields,
                 &(pfs.filters[index]))) {
    Log.error("Encoding failed: %s", PB_GET_ERROR(&ostream));
  } else {
    if (!rfquack_transport_send(
            RFQUACK_OUT_TOPIC RFQUACK_TOPIC_SEP RFQUACK_TOPIC_PACKET_FILTER,
            buf, ostream.bytes_written))
      Log.error("Failed sending to transport");
    else
      Log.trace("Data successfully sent");
  }
}

/**
 * @brief Loop through all packet filters and send them
 * to the nodes.
 */
static void rfquack_send_packet_filters() {
  for (uint8_t i = 0; i < pfs.size; i++)
    rfquack_send_packet_filter(i);
}

/**
 * @brief Add packet-modification rule to ruleset and compiles regex (if any).
 *
 */
static void rfquack_set_packet_modification(char *payload, int payload_length) {
  // init
  rfquack_PacketModification pm;

  // create stream from buffer
  pb_istream_t istream =
      pb_istream_from_buffer((uint8_t *)payload, payload_length);

  if (!pb_decode(&istream, rfquack_PacketModification_fields, &pm)) {
    Log.error("Cannot decode PacketModification: %s", PB_GET_ERROR(&istream));

    return;
  }

  int idx = pms.size;
  pms.size++;

  if (pm.has_pattern) {
    // compile the pattern
    re_t cp = re_compile(pm.pattern);

    // add pattern to ruleset
    memcpy(&(pms.patterns[idx]), &cp, sizeof(re_t));
  }

  // add rule to ruleset
  memcpy(&(pms.rules[idx]), &pm, sizeof(rfquack_PacketModification));
}

/**
 * @brief Parse and dispatch commands coming from the transport, according to
 * the topic.
 */
void rfquack_dispatch_command(char *topic, char *payload, int payload_length) {
  Log.trace("Command received: '%s' (payload: %d bytes)", topic,
            payload_length);

#ifdef RFQUACK_DEV
  rfquack_log_buffer("Payload = ", (uint8_t *)payload, payload_length);
#endif

  /***********************************************************************
   * Statistics
   ***********************************************************************/

  // Get stats: "rfquack/in/get/stats"
  if (strcmp(topic, RFQUACK_IN_TOPIC RFQUACK_TOPIC_SEP RFQUACK_TOPIC_GET
                        RFQUACK_TOPIC_STATS) == 0) {
    Log.trace("Request for stats");
    rfquack_send_stats();
    return;
  }

  // Get status: "rfquack/in/get/status"
  if (strcmp(topic, RFQUACK_IN_TOPIC RFQUACK_TOPIC_SEP RFQUACK_TOPIC_GET
                        RFQUACK_TOPIC_SEP RFQUACK_TOPIC_STATUS) == 0) {
    rfquack_send_status();
    return;
  }

  // Set status: "rfquack/in/set/status"
  if (strcmp(topic, RFQUACK_IN_TOPIC RFQUACK_TOPIC_SEP RFQUACK_TOPIC_SET
                        RFQUACK_TOPIC_SEP RFQUACK_TOPIC_STATUS) == 0 &&
      payload_length > 0) {
    rfquack_status(payload, payload_length);
    return;
  }

  /***********************************************************************
   * Registers
   ***********************************************************************/

  // Set or get register: "rfquack/in/(set|get)/register"
  if ((strcmp(topic, RFQUACK_IN_TOPIC RFQUACK_TOPIC_SEP RFQUACK_TOPIC_SET
                         RFQUACK_TOPIC_SEP RFQUACK_TOPIC_REGISTER) == 0 ||
       strcmp(topic, RFQUACK_IN_TOPIC RFQUACK_TOPIC_SEP RFQUACK_TOPIC_GET
                         RFQUACK_TOPIC_SEP RFQUACK_TOPIC_REGISTER) == 0) &&
      payload_length > 0) {
    rfquack_register_access(payload, payload_length);
    return;
  }

  /***********************************************************************
   * Modem
   ***********************************************************************/

  // Set modem configuration: "rfquack/in/set/modem_config"
  if (strcmp(topic, RFQUACK_IN_TOPIC RFQUACK_TOPIC_SEP RFQUACK_TOPIC_SET
                        RFQUACK_TOPIC_SEP RFQUACK_TOPIC_MODEM_CONFIG) == 0 &&
      payload_length > 0) {
    rfquack_modem_config_change(payload, payload_length);
    return;
  }

  /***********************************************************************
   * Transmission
   ***********************************************************************/

  // Transmit (set) a packet: "rfquack/in/set/packet"
  if (strcmp(topic, RFQUACK_IN_TOPIC RFQUACK_TOPIC_SEP RFQUACK_TOPIC_SET
                        RFQUACK_TOPIC_SEP RFQUACK_TOPIC_PACKET) == 0 &&
      payload_length > 0) {
    rfquack_tx_packet(payload, payload_length);
    return;
  }

  /***********************************************************************
   * Packet format
   ***********************************************************************/

  // Set the packet format
  // "rfquack/in/set/packet_format"
  if (strcmp(topic,
             RFQUACK_IN_TOPIC RFQUACK_TOPIC_SEP RFQUACK_TOPIC_SET
                 RFQUACK_TOPIC_SEP RFQUACK_TOPIC_PACKET_FORMAT) == 0) {
    if (payload_length > 0)
      rfquack_set_packet_format(payload, payload_length);

    return;
  }

  /***********************************************************************
   * Packet manipulation
   ***********************************************************************/

  // Set a new packet modification or reset them all together:
  // "rfquack/in/set/packet_modification"
  if (strcmp(topic,
             RFQUACK_IN_TOPIC RFQUACK_TOPIC_SEP RFQUACK_TOPIC_SET
                 RFQUACK_TOPIC_SEP RFQUACK_TOPIC_PACKET_MODIFICATION) == 0) {
    if (payload_length > 0)
      rfquack_set_packet_modification(payload, payload_length);
    else
      rfquack_packet_modification_init();

    return;
  }

  // Get packet modifications: "rfquack/in/get/packet_modification"
  if (strcmp(topic,
             RFQUACK_IN_TOPIC RFQUACK_TOPIC_SEP RFQUACK_TOPIC_GET
                 RFQUACK_TOPIC_SEP RFQUACK_TOPIC_PACKET_MODIFICATION) == 0) {
    rfquack_send_packet_modifications();
    return;
  }

  /***********************************************************************
   * Packet filtering
   ***********************************************************************/

  // Set a new packet filter or reset them all together:
  // "rfquack/in/set/packet_filter"
  if (strcmp(topic, RFQUACK_IN_TOPIC RFQUACK_TOPIC_SEP RFQUACK_TOPIC_SET
                        RFQUACK_TOPIC_SEP RFQUACK_TOPIC_PACKET_FILTER) == 0) {
    if (payload_length > 0)
      rfquack_set_packet_filter(payload, payload_length);
    else
      rfquack_packet_filter_init();

    return;
  }

  // Get packet filters: "rfquack/in/get/packet_filter"
  if (strcmp(topic, RFQUACK_IN_TOPIC RFQUACK_TOPIC_SEP RFQUACK_TOPIC_GET
                        RFQUACK_TOPIC_SEP RFQUACK_TOPIC_PACKET_FILTER) == 0) {
    rfquack_send_packet_filters();
    return;
  }

  /***********************************************************************
   * Set promiscuous mode
   ***********************************************************************/
  // Set promiscuosu mode: "rfquack/in/set/promiscuous"
  if (strcmp(topic, RFQUACK_IN_TOPIC RFQUACK_TOPIC_SEP RFQUACK_TOPIC_SET
                        RFQUACK_TOPIC_SEP RFQUACK_TOPIC_PROMISCUOUS) == 0) {
    rfquack_set_promiscuous(true);
    return;
  }

  // Unset promiscuous mode: "rfquack/in/set/promiscuous"
  if (strcmp(topic, RFQUACK_IN_TOPIC RFQUACK_TOPIC_SEP RFQUACK_TOPIC_UNSET
                        RFQUACK_TOPIC_SEP RFQUACK_TOPIC_PROMISCUOUS) == 0) {
    rfquack_set_promiscuous(false);
    return;
  }

  /***********************************************************************
   * Reset the radio
   ***********************************************************************/
  // Get packet filters: "rfquack/in/set/radio_reset"
  if (strcmp(topic, RFQUACK_IN_TOPIC RFQUACK_TOPIC_SEP RFQUACK_TOPIC_SET
                        RFQUACK_TOPIC_SEP RFQUACK_TOPIC_RADIO_RESET) == 0) {
    rfquack_radio_setup();
    return;
  }

  Log.warning("Unhandled command");
}

/*
 * Initialize data structures
 */
void rfquack_init() {
  // we'll ignore the defaults, because we're using our constants
  rfquack_Stats stats = rfquack_Stats_init_zero;
  rfq.stats = stats;
  rfq.has_stats = true;
  rfq.mode = rfquack_Mode_IDLE; // safe default
  rfq.has_mode = true;

#ifdef RFQUACK_RADIO_HAS_MODEM_CONFIG
  rfquack_ModemConfig modemConfig = rfquack_ModemConfig_init_default;
  rfq.modemConfig = modemConfig;
  rfq.has_modemConfig = true;

  rfq.modemConfig.modemConfigChoiceIndex =
      RFQUACK_RADIO_MODEM_CONFIG_CHOICE_INDEX;

#ifdef RFQUACK_RADIO_SET_FREQ
  rfq.modemConfig.carrierFreq = RFQUACK_RADIO_CARRIER;
  rfq.modemConfig.has_carrierFreq = true;
#endif

#ifdef RFQUACK_RADIO_SET_POWER
  rfq.modemConfig.txPower = RFQUACK_RADIO_TX_POWER;
  rfq.modemConfig.has_txPower = true;

  rfq.modemConfig.isHighPowerModule = RFQUACK_RADIO_HIGHPOWER;
  rfq.modemConfig.has_isHighPowerModule = true;
#endif

#ifdef RFQUACK_RADIO_SET_PREAMBLE
  rfq.modemConfig.preambleLen = RFQUACK_RADIO_PREAMBLE_LEN;
  rfq.modemConfig.has_preambleLen = true;
#endif

  int8_t sw[] = RFQUACK_RADIO_SYNC_WORDS;
  uint32_t sws = sizeof(sw);
  memcpy(rfq.modemConfig.syncWords.bytes, sw, sws);
  rfq.modemConfig.syncWords.size = sws;

  Log.trace("Setting sync words length to %d", rfq.modemConfig.syncWords.size);

  rfq.modemConfig.has_syncWords = true;
#else
  rfq.has_modemConfig = false;
#endif // RFQUACK_RADIO_HAS_MODEM_CONFIG

  rfquack_packet_filter_init();
  rfquack_packet_modification_init();

  Log.trace("RFQuack data structure initialized: %s", RFQUACK_UNIQ_ID);
}

void rfquack_setup() {
  rfquack_logging_setup();

  delay(100);

  rfquack_init();

  delay(100);

  randomSeed(micros());

  rfquack_network_setup();

  delay(100);

  rfquack_transport_setup(rfquack_dispatch_command);

  delay(100);

  rfquack_transport_connect();

  delay(100);

  rfquack_radio_setup();

  delay(100);

  rfquack_send_status();
}

void rfquack_loop() {
  rfquack_network_loop();

#ifndef RFQUACK_ASYNC_RX // we poll only if we must
  // If in RX mode, receive data
  rfquack_rx_loop();
#endif

  // Flush the RX data if any
  rfquack_rx_flush_loop();

  rfquack_transport_loop();
}

#endif
