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

#ifndef rfquack_config_radio_h
#define rfquack_config_radio_h

#include "../defaults/radio.h"

#ifndef RFQUACK_REGISTER_HEX_FORMAT
#define RFQUACK_REGISTER_HEX_FORMAT "%x"
#endif

#ifndef RFQUACK_REGISTER_VALUE_HEX_FORMAT
#define RFQUACK_REGISTER_VALUE_HEX_FORMAT "0b%b"
#endif

/*****************************************************************************
 * Radio parameters
 *****************************************************************************/

#ifndef RFQUACK_RADIO_RX_QUEUE_LEN
#define RFQUACK_RADIO_RX_QUEUE_LEN RFQUACK_RADIO_RX_QUEUE_LEN_DEFAULT
#endif

#if defined(RFQUACK_RADIO_RF69)
#include <radio/rf69.h>
#elif defined(RFQUACK_RADIO_CC1120)
#include <radio/cc1120.h>
#else
#error "You must choose the radio by defining RFQUACK_RADIO_*" // TODO add more radios
#endif

#endif
