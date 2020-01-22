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

#ifndef rfquack_config_general_h
#define rfquack_config_general_h

/*****************************************************************************
 * General parameters
 *****************************************************************************/

#include "../defaults/general.h"

#ifndef RFQUACK_UNIQ_ID
#define RFQUACK_UNIQ_ID "RFQuack"
#endif

#ifndef RFQUACK_CONN_WAIT_MS
#define RFQUACK_CONN_WAIT_MS RFQUACK_CONN_WAIT_MS_DEFAULT
#endif

#ifndef RFQUACK_RX_TIMEOUT_MS
#define RFQUACK_RX_TIMEOUT_MS RFQUACK_RX_TIMEOUT_MS_DEFAULT
#endif

#ifndef RFQUACK_STATS_LOOP_PERIOD_MS
#define RFQUACK_STATS_LOOP_PERIOD_MS RFQUACK_STATS_LOOP_PERIOD_MS_DEFAULT
#endif

#ifndef RFQUACK_MAX_PACKET_MODIFICATIONS
#define RFQUACK_MAX_PACKET_MODIFICATIONS RFQUACK_MAX_PACKET_MODIFICATIONS_DEFAULT
#endif

#ifndef RFQUACK_MAX_PACKET_FILTERS
#define RFQUACK_MAX_PACKET_FILTERS RFQUACK_MAX_PACKET_FILTERS_DEFAULT
#endif

#ifndef RFQUACK_MAX_MODULES
#define RFQUACK_MAX_MODULES RFQUACK_MAX_MODULES_DEFAULT
#endif

#endif
