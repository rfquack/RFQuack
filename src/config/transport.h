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
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.  See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 51 Franklin
 * Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#ifndef rfquack_config_transport_h
#define rfquack_config_transport_h

#include "../utils/utils.h"
#include "../defaults/transport.h"

/*****************************************************************************
 * Protobuf message serialization settings
 *****************************************************************************/

#ifndef RFQUACK_MAX_PB_MSG_SIZE
#define RFQUACK_MAX_PB_MSG_SIZE RFQUACK_MAX_PB_MSG_SIZE_DEFAULT
#endif

/*****************************************************************************
 * Generic Transport Configuration
 *****************************************************************************/
/* Topics */
#ifndef RFQUACK_TOPIC_PREFIX
#define RFQUACK_TOPIC_PREFIX RFQUACK_TOPIC_PREFIX_DEFAULT
#endif

#ifndef RFQUACK_TOPIC_IN
#define RFQUACK_TOPIC_IN RFQUACK_TOPIC_IN_DEFAULT
#endif

#ifndef RFQUACK_TOPIC_SET
#define RFQUACK_TOPIC_SET RFQUACK_TOPIC_SET_DEFAULT
#endif

#ifndef RFQUACK_TOPIC_UNSET
#define RFQUACK_TOPIC_UNSET RFQUACK_TOPIC_UNSET_DEFAULT
#endif

#ifndef RFQUACK_TOPIC_GET
#define RFQUACK_TOPIC_GET RFQUACK_TOPIC_GET_DEFAULT
#endif

#ifndef RFQUACK_TOPIC_OUT
#define RFQUACK_TOPIC_OUT RFQUACK_TOPIC_OUT_DEFAULT
#endif

#ifndef RFQUACK_TOPIC_STATS
#define RFQUACK_TOPIC_STATS RFQUACK_TOPIC_STATS_DEFAULT
#endif

#ifndef RFQUACK_TOPIC_STATUS
#define RFQUACK_TOPIC_STATUS RFQUACK_TOPIC_STATUS_DEFAULT
#endif

#ifndef RFQUACK_TOPIC_MODEM_CONFIG
#define RFQUACK_TOPIC_MODEM_CONFIG RFQUACK_TOPIC_MODEM_CONFIG_DEFAULT
#endif

#ifndef RFQUACK_TOPIC_PACKET
#define RFQUACK_TOPIC_PACKET RFQUACK_TOPIC_PACKET_DEFAULT
#endif

#ifndef RFQUACK_TOPIC_PACKET_FORMAT
#define RFQUACK_TOPIC_PACKET_FORMAT RFQUACK_TOPIC_PACKET_FORMAT_DEFAULT
#endif

#ifndef RFQUACK_TOPIC_MODE
#define RFQUACK_TOPIC_MODE RFQUACK_TOPIC_MODE_DEFAULT
#endif

#ifndef RFQUACK_TOPIC_REGISTER
#define RFQUACK_TOPIC_REGISTER RFQUACK_TOPIC_REGISTER_DEFAULT
#endif

#ifndef RFQUACK_TOPIC_PACKET_FILTER
#define RFQUACK_TOPIC_PACKET_FILTER RFQUACK_TOPIC_PACKET_FILTER_DEFAULT
#endif

#ifndef RFQUACK_TOPIC_RADIO_RESET
#define RFQUACK_TOPIC_RADIO_RESET RFQUACK_TOPIC_RADIO_RESET_DEFAULT
#endif

#ifndef RFQUACK_TOPIC_PROMISCUOUS
#define RFQUACK_TOPIC_PROMISCUOUS RFQUACK_TOPIC_PROMISCUOUS_DEFAULT
#endif

#ifndef RFQUACK_MAX_VERB_LEN
#define RFQUACK_MAX_VERB_LEN RFQUACK_MAX_VERB_LEN_DEFAULT
#endif

#ifndef RFQUACK_MAX_TOPIC_LEN
#define RFQUACK_MAX_TOPIC_LEN RFQUACK_MAX_TOPIC_LEN_DEFAULT
#endif

#ifndef RFQUACK_MAX_TOKEN_LEN
#define RFQUACK_MAX_TOKEN_LEN RFQUACK_MAX_TOKEN_LEN_DEFAULT
#endif

#ifndef RFQUACK_TOPIC_MAX_TOPIC_ARGS
#define RFQUACK_TOPIC_MAX_TOPIC_ARGS RFQUACK_TOPIC_MAX_TOPIC_ARGS_DEFAULT
#endif

#ifndef RFQUACK_TOPIC_RULES
#define RFQUACK_TOPIC_RULES RFQUACK_TOPIC_RULES_DEFAULT

#endif


/*
 * Inbound packets
 */
// Example:  rfquack/in
#define RFQUACK_IN_TOPIC RFQUACK_TOPIC_PREFIX RFQUACK_TOPIC_SEP RFQUACK_TOPIC_IN

// Example:  rfquack/in/#
#define RFQUACK_IN_TOPIC_WILDCARD RFQUACK_IN_TOPIC RFQUACK_TOPIC_SEP "#"

/*
 * Outbound packets
 */
// Example:  rfquack/out
#define RFQUACK_OUT_TOPIC RFQUACK_TOPIC_PREFIX RFQUACK_TOPIC_SEP RFQUACK_TOPIC_OUT

/*****************************************************************************
 * MQTT Broker Configuration
 *****************************************************************************/

#ifndef RFQUACK_MQTT_MAX_PACKET_SIZE
#define RFQUACK_MQTT_MAX_PACKET_SIZE RFQUACK_MQTT_MAX_PACKET_SIZE_DEFAULT
#endif

#ifndef RFQUACK_MQTT_RETRY_DELAY
#define RFQUACK_MQTT_RETRY_DELAY RFQUACK_MQTT_RETRY_DELAY_DEFAULT
#endif

#if defined(RFQUACK_TRANSPORT_MQTT)

#define RFQUACK_TRANSPORT "MQTT"

#if !defined(RFQUACK_MQTT_BROKER_HOST)
#error "Define RFQUACK_MQTT_BROKER_HOST \"hostname\""
#endif

#ifndef RFQUACK_MQTT_BROKER_PORT
#define RFQUACK_MQTT_BROKER_PORT RFQUACK_MQTT_BROKER_PORT_DEFAULT
#endif

#ifndef RFQUACK_MQTT_TRANSPORT_RECONNECT_EVERY_MS
#define RFQUACK_MQTT_TRANSPORT_RECONNECT_EVERY_MS                              \
  RFQUACK_MQTT_TRANSPORT_RECONNECT_EVERY_MS_DEFAULT
#endif

/* QoS */
#ifndef RFQUACK_MQTT_QOS
#define RFQUACK_MQTT_QOS RFQUACK_MQTT_QOS_DEFAULT
#endif

#ifndef RFQUACK_MQTT_SOCKET_TIMEOUT
#define RFQUACK_MQTT_SOCKET_TIMEOUT RFQUACK_MQTT_SOCKET_TIMEOUT_DEFAULT
#endif

#ifndef RFQUACK_MQTT_KEEPALIVE
#define RFQUACK_MQTT_KEEPALIVE RFQUACK_MQTT_KEEPALIVE_DEFAULT
#endif

#ifndef RFQUACK_MQTT_CLEAN_SESSION
#define RFQUACK_MQTT_CLEAN_SESSION RFQUACK_MQTT_CLEAN_SESSION_DEFAULT
#endif

#elif defined(RFQUACK_TRANSPORT_SERIAL)

#define RFQUACK_TRANSPORT "SERIAL"

#ifndef RFQUACK_SERIAL_BAUD_RATE
#define RFQUACK_SERIAL_BAUD_RATE RFQUACK_SERIAL_BAUD_RATE_DEFAULT
#endif

#ifndef RFQUACK_SERIAL_TOPIC_DATA_SEPARATOR_CHAR
#define RFQUACK_SERIAL_TOPIC_DATA_SEPARATOR_CHAR                                    \
  RFQUACK_SERIAL_TOPIC_DATA_SEPARATOR_CHAR_DEFAULT
#endif

#ifndef RFQUACK_SERIAL_MAX_PACKET_SIZE
#define RFQUACK_SERIAL_MAX_PACKET_SIZE RFQUACK_SERIAL_MAX_PACKET_SIZE_DEFAULT
#endif

// Base64 overhead
#define RFQUACK_SERIAL_B64_MAX_PACKET_SIZE CEILING(RFQUACK_SERIAL_MAX_PACKET_SIZE * 0.35)

#ifndef RFQUACK_SERIAL_PREFIX_IN_CHAR
#define RFQUACK_SERIAL_PREFIX_IN_CHAR RFQUACK_SERIAL_PREFIX_IN_CHAR_DEFAULT
#endif

#ifndef RFQUACK_SERIAL_PREFIX_OUT_CHAR
#define RFQUACK_SERIAL_PREFIX_OUT_CHAR RFQUACK_SERIAL_PREFIX_OUT_CHAR_DEFAULT
#endif

#ifndef RFQUACK_SERIAL_SUFFIX_IN_CHAR
#define RFQUACK_SERIAL_SUFFIX_IN_CHAR RFQUACK_SERIAL_SUFFIX_IN_CHAR_DEFAULT
#endif

#ifndef RFQUACK_SERIAL_SUFFIX_OUT_CHAR
#define RFQUACK_SERIAL_SUFFIX_OUT_CHAR RFQUACK_SERIAL_SUFFIX_OUT_CHAR_DEFAULT
#endif

#else

#error                                                                         \
    "Please select one transport between RFQUACK_TRANSPORT_SERIAL and RFQUACK_TRANSPORT_MQTT"

#endif

#endif
