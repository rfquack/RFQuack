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

import sys
import logging

import click
import coloredlogs
import serial.serialutil

from rfquack.transport import RFQuackMQTTTransport, RFQuackSerialTransport
from rfquack.shell import RFQuackShell

from rfquack.banner import BANNER

logger = logging.getLogger("rfquack.cli")

LOG_LEVELS = ("CRITICAL", "ERROR", "WARNING", "INFO", "DEBUG", "NOTSET")

CONTEXT_SETTINGS = dict(help_option_names=["-h", "--help"])


@click.group(context_settings=CONTEXT_SETTINGS)
@click.option("--loglevel", "-l", type=click.Choice(LOG_LEVELS), default="WARNING")
def cli(loglevel):
    coloredlogs.install(level=loglevel)


@cli.command()
@click.option("--client_id", "-i", default="RFQuackShell")
@click.option("--host", "-H", default="localhost")
@click.option("--port", "-P", type=int, default=1883)
@click.option("--username", "-u")
@click.option("--password", "-p")
@click.option("--certificate_authority", "-a")
@click.option("--certificate", "-c")
@click.option("--private_key", "-k")
def mqtt(
    client_id,
    host,
    port,
    username,
    password,
    certificate_authority,
    certificate,
    private_key,
):
    """
    RFQuack client with MQTT transport.
    """
    kwargs = dict(
        client_id=client_id,
        host=host,
        port=port,
        username=username,
        password=password,
        certificate_authority=certificate_authority,
        certificate=certificate,
        private_key=private_key,
    )

    transport = RFQuackMQTTTransport(**kwargs)

    s = RFQuackShell(
        "RFQuack({}, {}:{})".format(client_id, host, port), BANNER, transport, False
    )
    s()


@cli.command()
@click.option("--baudrate", "-b", type=int, default=115200)
@click.option("--bytesize", "-s", type=int, default=serial.serialutil.EIGHTBITS)
@click.option(
    "--parity",
    "-p",
    type=click.Choice(serial.serialutil.PARITY_NAMES),
    default=serial.serialutil.PARITY_NONE,
)
@click.option(
    "--stopbits",
    "-S",
    type=click.Choice(
        map(
            str,
            (
                serial.serialutil.STOPBITS_ONE,
                serial.serialutil.STOPBITS_ONE_POINT_FIVE,
                serial.serialutil.STOPBITS_TWO,
            ),
        )
    ),
    default=str(serial.serialutil.STOPBITS_ONE),
)
@click.option("--timeout", "-t", type=int, default=None)
@click.option("--port", "-P", required=True)
def tty(baudrate, bytesize, parity, stopbits, timeout, port):
    """
    RFQuack client with serial transport.
    """
    kwargs = dict(
        baudrate=baudrate,
        port=port,
        bytesize=bytesize,
        parity=parity,
        stopbits=int(stopbits),
        timeout=timeout,
    )

    transport = RFQuackSerialTransport(**kwargs)
    s = RFQuackShell("RFQuack({})".format(port), BANNER, transport, True)

    s()


def main():
    cli()


if __name__ == "__main__":
    sys.exit(main())
