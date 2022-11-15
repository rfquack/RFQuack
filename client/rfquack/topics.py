# -*- coding: utf-8 -*-

"""
This is a Python implementation of a RFQuack client.

RFQuack is a versatile RF-analysis tool that allows you to sniff, analyze, and
transmit data over the air.

Copyright (C) 2019 Trend Micro Incorporated

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation; either version 2 of the License, or (at your option) any later
version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program; if not, write to the Free Software Foundation, Inc., 51 Franklin
Street, Fifth Floor, Boston, MA  02110-1301, USA.
"""

"""
    Modules
"""
TOPIC_MODULE_DRIVER = b"driver"
TOPIC_MODULE_PACKET_MODIFICATION = b"packet_modification"
TOPIC_MODULE_PACKET_FILTER = b"packet_filter"


TOPIC_PREFIX_ANY = b"any"
TOPIC_SEP = b"/"
TOPIC_IN = b"in"
TOPIC_OUT = b"out"
TOPIC_GET = b"get"
TOPIC_SET = b"set"
TOPIC_INFO = b"info"