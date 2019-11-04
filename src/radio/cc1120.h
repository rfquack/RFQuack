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

#ifndef rfquack_radio_cc1120_h
#define rfquack_radio_cc1120_h

#define RFQUACK_RADIO "CC1120"
#define RFQUACK_RADIO_HAS_MODEM_CONFIG
#define RFQUACK_RADIO_PARTNO
//#undef RFQUACK_RADIO_SET_POWER    // it's part of the register config
//#undef RFQUACK_RADIO_SET_FREQ     // it's part of the register config
//#undef RFQUACK_RADIO_SET_RF       // it's for certain 2.4GHz radios only
//#undef RFQUACK_RADIO_SET_PREAMBLE // it's part of the register config
//#define RFQUACK_RADIO_TX_FIFO_REPEAT

// Modem configuration
#ifndef RFQUACK_RADIO_SYNC_WORDS
#define RFQUACK_RADIO_SYNC_WORDS { 0x45, 0x44, 0x43, 0x42 }
#endif

#ifndef RFQUACK_RADIO_MODEM_CONFIG_CHOICE_INDEX
#define RFQUACK_RADIO_MODEM_CONFIG_CHOICE_INDEX 5
#endif

#include <RH_CC1120.h>

#ifdef RFQUACK_RADIO_PIN_CS
#define RFMCC1120_CS RFQUACK_RADIO_PIN_CS
#else
#error "Please define RFQUACK_RADIO_PIN_CS"
#endif

#ifdef RFQUACK_RADIO_PIN_IRQ
#define RFMCC1120_IRQ RFQUACK_RADIO_PIN_IRQ
#else
#error "Please define RFQUACK_RADIO_PIN_IRQ"
#endif

#ifdef RFQUACK_RADIO_PIN_RST
#define RFMCC1120_RST RFQUACK_RADIO_PIN_RST
#else
#error "Please define RFQUACK_RADIO_PIN_RST"
#endif

/*
 * The length of the payload is limited to 255 bytes if AES is not enabled else
 * the message is limited to 64 bytes (i.e. max 65 bytes payload if Address
 * byte is enabled).
 */
#define RFQUACK_RADIO_MIN_MSG_LEN 1
#define RFQUACK_RADIO_MAX_MSG_LEN RH_CC1120_MAX_MESSAGE_LEN
#define MAX_MSG_LEN RFQUACK_RADIO_MAX_MSG_LEN

/*
 * Sync word size can be set from 1 to 8 bytes (i.e. 8 to 64 bits) via SyncSize
 * in RegSyncConfig. In Packet mode this field is also used for Sync word
 * generation in Tx mode.
 *
 * IMPORTANT: sync word choices containing 0x00 bytes are not allowed
 */
#define RFQUACK_MIN_SYNC_WORDS_LEN 1
#define RFQUACK_MAX_SYNC_WORDS_LEN 8

typedef RH_CC1120::ModemConfigChoice RFQRadioModemConfigChoice;
typedef uint16_t rfquack_register_address_t;
typedef uint8_t rfquack_register_value_t;
typedef RH_CC1120 RFQRadio;

#endif
