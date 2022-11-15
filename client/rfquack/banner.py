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

BANNER = """

                   ,-.
               ,--' ~.).
             ,'         `.
            ; (((__   __)))      welcome to rfquack!
            ;  ( (#) ( (#)
            |   \_/___\_/|              the versatile
           ,"  ,-'    `__".             rf-hacking tool that quacks!
          (   ( ._   ____`.)--._        _
           `._ `-.`-' \(`-'  _  `-. _,-' `-/`.
            ,')   `.`._))  ,' `.   `.  ,','  ;   ~~~
          .'  .     `--'  /     ).   `.      ;
         ;     `-        /     '  )         ;           ~~~~
         \                       ')       ,'    ~~  ~
          \                     ,'       ;           ~~
           \               `~~~'       ,'               ~~~  ~~    ~~~~~
            `.                      _,'             ~~~
        hjw   `.                ,--'
        ~~~~~~~~`-._________,--'~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

        ------------------------------------------------------------------------

        > q.radioA.set_modem_config(
                modulation="OOK",           # Set the modulation
                carrierFreq=434.437,        # Set the frequency in MHz
                bitRate=3.41296,            # Set the bitrate in KHz
                useCRC=False,               # Whatever to use the integrated CRC
                syncWords=b"\\x99\\x9A",      # Set the sync-words
                rxBandwidth=58)             # Set the RX Bandwidth
        
        > q.radioA.set_packet_len(
                isFixedPacketLen=True,      # Fixed packet mode 
                packetLen=102)              # Len set to 102

        > q.radioA.tx()                     # Enters TX mode.
        
        > q.radioA.send(
            data=b"\\xAA\\xBB\\xCC")           # Sends a packet
            
            
                                            # example: with rfm69
                                            # -------------------
        > q.radioA.set_register(            #  truly promiscuous mode:
            address=0x2e,                   #  1) set register 0x2e
            value=0b01000000                #     to 0b01000000
            )                               #
    > q.radioA.set_register(                #
            address=0x37,                   #  2) set register 0x37
            value=0b01000000                #     to 0b11000000
            )

        > q.packet_filter.add               # ignore packets
            pattern="^ab[cd]",              # not matching this regex
            negateRule=False                # do not invert rule.
        )

        > q.packet_modification.add(        # modify packets as follows:
            pattern="[ke]$",                #  if they end with 'k' or 'e'
            position=3,                     #  at position = 3 in the payload
            content=b'\\x29',                #  and position = indexof(0x29)
            operation=2                     #  content[position] |= 0x25
            operand=b'\\x25'
        )


        help:
        > q.packet_modification.help()      # use the helper function.
        > q.?                               # tab is your friend

        exit:   just type ctrl-d a couple of times!
"""
