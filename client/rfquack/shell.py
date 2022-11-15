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

import logging

from rfquack.src import rfquack_pb2
from rfquack.core import RFQuack

logger = logging.getLogger("rfquack.shell")


class RFQuackShell(object):
    """
    The RFQuack shell is a simple wrapper of IPython. It creates an interactive
    shell, through which the user can talk to the RFQuack dongle via a given
    transport.
    """

    def __init__(self, token, banner, transport, select_first_dongle):
        self._banner = banner
        self._transport = transport
        self._token = token
        self._select_first_dongle = select_first_dongle

    def __call__(self):
        from IPython.terminal.prompts import Prompts, Token
        from IPython.terminal.interactiveshell import TerminalInteractiveShell

        token = self._token

        class RFQuackShellPrompts(Prompts):
            def in_prompt_tokens(self, cli=None):
                return [(Token, token), (Token.Prompt, "> ")]

        TerminalInteractiveShell.prompts_class = RFQuackShellPrompts
        shell = TerminalInteractiveShell()
        shell.autocall = 0
        shell.show_banner(self._banner)

        q = RFQuack(self._transport, "rfquack", shell, self._select_first_dongle)
        shell_objs = dict(q=q, pb=rfquack_pb2)

        # For each message field add a fake object with the same name,
        # this to achieve a **very basic** autocomp.
        for message_type in rfquack_pb2.DESCRIPTOR.message_types_by_name.values():
            for message_field in message_type.fields:
                shell_objs[
                    message_field.name
                ] = "This is a fake variable, used for autocompletion"

        shell.push(shell_objs)

        shell.mainloop()
