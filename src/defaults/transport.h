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

#ifndef rfquack_defaults_transport_h
#define rfquack_defaults_transport_h

#define RFQUACK_MAX_PACKET_SIZE_DEFAULT 512

#define RFQUACK_SERIAL_BAUD_RATE_DEFAULT 9600

/*****************************************************************************
 * Protobuf message serialization settings
 *****************************************************************************/

#define RFQUACK_MAX_PB_MSG_SIZE_DEFAULT 256

/*****************************************************************************
 * MQTT Configuration
 *****************************************************************************/

#define RFQUACK_MQTT_BROKER_PORT_DEFAULT 1883

#define RFQUACK_MQTT_TRANSPORT_RECONNECT_EVERY_MS_DEFAULT 5000L

#define RFQUACK_MQTT_QOS_DEFAULT 0

#define RFQUACK_MQTT_RETAIN_DEFAULT false

#define RFQUACK_MQTT_CLEAN_SESSION_DEFAULT true

#define RFQUACK_MQTT_SOCKET_TIMEOUT_DEFAULT 20000L // msec

#define RFQUACK_MQTT_KEEPALIVE_DEFAULT 5 // sec

#define RFQUACK_MQTT_RETRY_DELAY_DEFAULT 1000

#define RFQUACK_MQTT_MAX_PACKET_SIZE_DEFAULT RFQUACK_MAX_PACKET_SIZE_DEFAULT

/*****************************************************************************
 * Topic Configuration
 *****************************************************************************/

#define RFQUACK_TOPIC_SEP "/"

#define RFQUACK_TOPIC_PREFIX_DEFAULT "rfquack"

#define RFQUACK_TOPIC_IN_DEFAULT "in"

#define RFQUACK_TOPIC_SET_DEFAULT "set"

#define RFQUACK_TOPIC_UNSET_DEFAULT "unset"

#define RFQUACK_TOPIC_GET_DEFAULT "get"

#define RFQUACK_TOPIC_OUT_DEFAULT "out"

#define RFQUACK_TOPIC_STATS_DEFAULT "stats"

#define RFQUACK_TOPIC_STATUS_DEFAULT "status"

#define RFQUACK_TOPIC_MODEM_CONFIG_DEFAULT "modem_config"

#define RFQUACK_TOPIC_PACKET_DEFAULT "packet"

#define RFQUACK_TOPIC_MODE_DEFAULT "mode"

#define RFQUACK_TOPIC_REGISTER_DEFAULT "register"

#define RFQUACK_TOPIC_PACKET_MODIFICATION "packet_modification"

#define RFQUACK_TOPIC_PACKET_FILTER_DEFAULT "packet_filter"

#define RFQUACK_TOPIC_PACKET_FORMAT_DEFAULT "packet_format"

#define RFQUACK_TOPIC_RADIO_RESET_DEFAULT "radio_reset"

#define RFQUACK_TOPIC_PROMISCUOUS_DEFAULT "promiscuous"

#define RFQUACK_MAX_TOPIC_LEN_DEFAULT 64

/*****************************************************************************
 * Serial Configuration
 *****************************************************************************/

#define RFQUACK_SERIAL_MAX_PACKET_SIZE_DEFAULT RFQUACK_MAX_PACKET_SIZE_DEFAULT

#define RFQUACK_SERIAL_PREFIX_IN_CHAR_DEFAULT '>'
#define RFQUACK_SERIAL_PREFIX_OUT_CHAR_DEFAULT '<'
#define RFQUACK_SERIAL_SUFFIX_OUT_CHAR_DEFAULT '\0'
#define RFQUACK_SERIAL_SUFFIX_IN_CHAR_DEFAULT '\0'
#define RFQUACK_SERIAL_TOPIC_DATA_SEPARATOR_CHAR_DEFAULT '~'

#endif
