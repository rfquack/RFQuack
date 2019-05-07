#! /usr/bin/env python

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

import io
import os
import sys
import base64
import logging
import threading
import binascii

import click
import serial
import serial.threaded
import serial.serialutil
import coloredlogs
import paho.mqtt.client as paho_mqtt

from colorama import Fore, Back, Style

import rfquack_pb2


HERE = os.path.dirname(os.path.abspath(__file__))
BANNER_FILE = os.path.join(HERE, '..', '..', 'banner.txt')
LOG_LEVELS = (
        'CRITICAL',
        'ERROR',
        'WARNING',
        'INFO',
        'DEBUG',
        'NOTSET')
logger = logging.getLogger('RFQuack')


def hexelify(blob):
    return ' '.join(['0x{:02X}'.format(o) for o in blob])


class RFQuackTransport(object):
    """
    Every RFQuack transport is based on messages, which are composed by a topic
    and an optional (binary) payload. The payload is a serialized Protobuf
    message.

    The concept of topics is borrowed by MQTT. Indeed, the most obvious
    implementation of the RFQuack transport is on top of MQTT. Each topic is
    like a path, formed by three parts, split by a separator, usually a slash.

        <prefix>/<way>/<set|get>/<command>

    The <prefix> (e.g., rfquack) is used to distinguis correct messages.

    The direction, or <way> (e.g., in, out), indicates that indicates whether
    the message is inbound or outbound. Inbound messages are going from this
    client to the RFQuack dongle. Outbound messages come from the RFQuack
    dongle and are directed to this client.

    The <get|set> part indicates whether a message is intended to set or get a
    value. The semantic of this part is implemented in the RFQuack firmware.
    For example, if we need to set a register to a value, we will use a topic
    such as 'rfquack/in/set/<command>'.

    The command must be the last part, and tells what command is carried by the
    message. Following the above example: 'rfquack/in/set/register'

    Once a messaage is received, it is dispatched to the correct handler,
    together with its payload, which must be deserialized according to the
    right Protobuf message class. An easy way is to map each <command> to a
    distinct Protobuf message class.
    """

    TOPIC_PREFIX = "rfquack"
    TOPIC_SEP = "/"
    TOPIC_IN = "in"
    TOPIC_OUT = "out"
    TOPIC_GET = "get"
    TOPIC_SET = "set"
    TOPIC_STATS = "stats"
    TOPIC_STATUS = "status"
    TOPIC_MODEM_CONFIG = "modem_config"
    TOPIC_PACKET = "packet"
    TOPIC_MODE = "mode"
    TOPIC_REGISTER = "register"
    TOPIC_PACKET_MODIFICATION = "packet_modification"
    TOPIC_PACKET_FILTER = "packet_filter"
    TOPIC_RADIO_RESET = "radio_reset"

    # from node to client
    OUT_TYPE_MAP = dict(
        stats=rfquack_pb2.Stats,
        status=rfquack_pb2.Status,
        packet=rfquack_pb2.Packet,
        register=rfquack_pb2.Register,
        packet_filter=rfquack_pb2.PacketFilter,
        packet_modification=rfquack_pb2.PacketModification
    )

    # from client to node
    IN_TYPE_MAP = dict(
        packet=rfquack_pb2.Packet
    )

    def __init__(self, *args, **kwargs):
        raise NotImplementedError('You must override the constructor')

    def ready(self):
        return self._ready

    def _message_parser(self, topic, payload):
        try:
            prefix, way, cmd = topic.split(self.TOPIC_SEP)
        except Exception:
            logger.warning('Cannot parse topic: must be <prefix>/<way>/<cmd>')
            return

        logger.debug('Message on topic "{}"'.format(topic))

        if prefix != self.TOPIC_PREFIX:
            logger.warning(
                'Invalid prefix: {} should be {}'
                .format(prefix, self.TOPIC_PREFIX))
            return

        if way != self.TOPIC_OUT:
            return

        klass = self.OUT_TYPE_MAP.get(cmd, None)

        if klass is None:
            logger.warning(
                'Ignoring "{}": doesn\'t match any known command'.format(cmd))
            return

        pb_msg = klass()
        try:
            pb_msg.ParseFromString(payload)
            logger.debug('{} -> {}: {}'.format(topic, klass, str(pb_msg)))
        except Exception as e:
            logger.error('Cannot deserialize data: {}'.format(e))
            return

        if self._on_message_callback:
            self._on_message_callback(cmd=cmd, msg=pb_msg)

    def _send(self, command, payload):
        raise NotImplementedError()

    def verbose(self):
        pass

    def quiet(self):
        pass


class RFQuackSerialProtocol(serial.threaded.FramedPacket):
    """
    The RFQuack serial protocol is very simple. Each incoming message is:

        <PREFIX><TOPIC><SEPARATOR><DATA><SUFFIX>

    where:

        <PREFIX> = '<'
        <TOPIC> = <prefix>/<way>/<command>
        <SEPARATOR> = '~'
        <DATA> = Base64(<serialized protobuf data>)
        <SUFFIX> = '\0'

    For instance:

        <rfquack/<way>/<command>~Base64(<serialized protobuf data>)\0

    Outgoing messages have the same exact format, with a different prefix:

        <PREFIX> = '>'

    Assumption: there's nothing else on the serial bus.

    """
    SERIAL_PREFIX_IN = b'<'  # packet for us
    SERIAL_PREFIX_OUT = b'>' # packet for the dongle
    SERIAL_SUFFIX = b'\0'
    SERIAL_SEPARATOR = b'~'
    callback = None

    def __init__(self):
        super(RFQuackSerialProtocol, self).__init__()
        self._verbose = True

        # really print anything that is received
        self._debug = False

        self.init_parser()

        # holds out-of-packet data
        self.line = bytearray()

    def connection_made(self, transport):
        super(RFQuackSerialProtocol, self).connection_made(transport)
        logger.info('Port opened')

    def connection_lost(self, exc):
        try:
            super(RFQuackSerialProtocol, self).connection_lost(exc)
        except Exception as e:
            logger.error(e)

    def token_search(self, token, buf, byte):
        # if we're still looking for this token
        if self.idx < len(token):
            # if the char is the expected one
            if byte == token[self.idx]:
                # save it in the token buffer
                buf.extend(byte)

                # and increment the prefix index
                self.idx += 1
                return False
            else:
                # otherwise, reset the state machine
                self.idx = 0
                self.token = bytearray()
                return False
        elif self.idx == len(token):
            return True

        return False

    def init_parser(self):
        # buffer for the data enclosed between prefix and suffix
        self.packet = bytearray()

        # buffers for the token we're currently looking for
        self.prefix_token = bytearray()

        # indicates whether we've found the prefix
        self.prefix_found = False

    def data_received(self, data):
        """Find data enclosed in tokens, call handle_packet"""
        if self._debug:
            print "DATA CHUNK RECEIVED = '{}'".format(data)

        # for each byte in the recv buffer
        for byte in serial.iterbytes(data):
            if not self.prefix_found:
                if byte == self.SERIAL_PREFIX_IN:
                    self.prefix_found = True
            else:
                if byte != self.SERIAL_SUFFIX:
                    self.packet.extend(byte)
                else:
                    self.handle_packet(bytes(self.packet))
                    self.init_parser()

    def handle_packet(self, packet):
        if not len(packet):
            return

        if self._debug:
            logger.debug('Packet = "{}"'.format(packet))

        if self.SERIAL_SEPARATOR not in packet:
            return

        parts = packet.split(self.SERIAL_SEPARATOR)

        if len(parts) == 2:
            topic, payload_b64 = parts

            payload = base64.b64decode(payload_b64)

            logger.debug(
                    '{} bytes received on topic: "{}" = "{}"'.format(
                        len(payload),
                        topic,
                        binascii.hexlify(payload)))

            if self.callback:
                try:
                    self.callback(topic, payload)
                except Exception as e:
                    logger.error('Cannot parse message: {}'.format(e))
        else:
            logger.error('Unexpected data format: {}'.format(packet))

    def write_packet(self, topic, payload):
        data = b'{prefix}{topic}{sep}{payload}{suffix}'.format(
                prefix=self.SERIAL_PREFIX_OUT,
                topic=topic,
                sep=self.SERIAL_SEPARATOR,
                payload=base64.b64encode(payload),
                suffix=self.SERIAL_SUFFIX)
        if self._verbose:
            logger.debug('Writing packet = {}'.format(data))
        return self.transport.write(data)

    def handle_out_of_packet_data(self, byte):
        """Accumulate bytes until a terminator is found, look for the
        begin-of-log-line tokens, and consider it a packet"""
        self.line.extend(byte)

        for tok in self.SERIAL_LOG_TOKENS:
            if tok in self.line and \
                    self.line.endswith(self.SERIAL_LOG_NEWLINE):
                        s = self.line.index(tok)
                        self.handle_log(bytes(self.line[s:]))
                        self.line = bytearray()

    def handle_log(self, line):
        # TODO find the right way to print in IPython
        if self._verbose:
            print '\033[94m' + line,


class RFQuackSerialTransport(RFQuackTransport):
    """
    The RFQuack serial transport implementation consumes data from the serial
    port using a separate thread, and parses the data according to the
    `RFQuackSerialProtocol` class.
    """

    def __init__(self, *args, **kwargs):
        """
        Keyword arguments are passed straight to the `Serial` class constructor
        """
        super(RFQuackTransport, self).__init__()

        if 'port' not in kwargs:
            raise ValueError('Please specify the port')

        self.args = args
        self.kwargs = kwargs
        self.ser = None
        self._on_message_callback = None

    def init(self, *args, **kwargs):
        self._on_message_callback = kwargs.get('on_message_callback')
        self.ser = serial.Serial(**self.kwargs)

        class _RFQuackSerialProtocol(RFQuackSerialProtocol):
            callback = self._on_message

        self._reader = serial.threaded.ReaderThread(
                self.ser,
                _RFQuackSerialProtocol)

        self._reader.start()
        self._transport, self._protocol = self._reader.connect()
        self._ready = True

    def debug(self):
        self._protocol._debug = True

    def verbose(self):
        self._protocol._verbose = True

    def quiet(self):
        self._protocol._verbose = False
        self._protocol._debug = False

    def _on_message(self, topic, payload):
        self._message_parser(topic, payload)

    def end(self):
        self._ready = False
        self._reader.stop()

    def _send(self, command, payload):
        topic = self.TOPIC_SEP.join(
            (self.TOPIC_PREFIX, self.TOPIC_IN, command))
        logger.debug('{} ({} bytes)'.format(topic, len(payload)))
        logger.debug('payload = {}'.format(hexelify(bytearray(payload))))

        self._protocol.write_packet(topic, payload)


class RFQuackMQTTTransport(RFQuackTransport):
    """
    MQTT transport implements the RFQuack protocol by mapping topics and
    payloads onto valid MQTT messages.

    * TODO dispatch nodes by client-id so that multiple dongles can share the
    same broker
    """
    QOS = 2
    RETAIN = False

    DEFAULT_SUSCRIBE = RFQuackTransport.TOPIC_SEP.join((
        RFQuackTransport.TOPIC_PREFIX,
        RFQuackTransport.TOPIC_OUT,
        '#'
    ))  # subscribe to all, dispatch later

    def __init__(
            self, client_id, username=None, password=None, host='localhost',
            port=1883):

        self._userdata = {}
        self._username = username
        self._password = password
        self._mqtt = dict(
            host=host,
            port=port)
        self._client = paho_mqtt.Client(
                client_id=client_id,
                userdata=self._userdata)
        self._ready = False
        self._on_packet = None

    def init(self, *args, **kwargs):
        self._client.on_message = self._on_message
        self._client.on_connect = self._on_connect
        self._client.on_subscribe = self._on_subscribe

        if kwargs.get('on_message_callback'):
            self._on_message_callback = kwargs.get('on_message_callback')

        if self._username:
            self._client.username_pw_set(
                self._username, self._password)

        self._client.connect_async(
            self._mqtt.get('host'),
            self._mqtt.get('port')
        )

        logger.info('Transport initialized')

        self._client.loop_start()

        self._ready = True

    def end(self):
        self._ready = False
        self._client.loop_stop()

    def _on_connect(self, client, userdata, flags, rc):
        if self.DEFAULT_SUSCRIBE:
            self._client.subscribe(
                self.DEFAULT_SUSCRIBE, qos=self.QOS)

        logger.info('Connected to broker. Feed = {}'.format(
            self.MQTT_DEFAULT_SUSCRIBE))

    def _on_subscribe(self, client, userdata, mid, granted_qos):
        logger.info('Transport pipe initialized (QoS = {}): mid = {}'.format(
            granted_qos[0], mid))

    def _on_message(self, client, userdata, msg):
        self._message_parser(msg.topic, msg.payload)

    def _send(self, command, payload):
        topic = self.TOPIC_SEP.join(
            (self.TOPIC_PREFIX, self.TOPIC_IN, command))
        logger.debug('{} ({} bytes)'.format(topic, len(payload)))

        self._client.publish(
            topic,
            payload=payload,
            qos=self.QOS,
            retain=self.RETAIN)


class RFQuack(object):
    """
    The RFQuack object connects to a given transport and abstracts the
    invocation of commands to the RFQuack dongle by means of function calls.
    """

    def __init__(self, transport, shell):
        self._mode = 'IDLE'
        self._ready = False
        self._transport = transport
        self._shell = shell

        self.data = {}

        self._init()

    def _init(self):
        """
        - reset packet modifications
        - put the tool in sending mode
        """
        self._transport.init(on_message_callback=self._recv)

    def _recv(self, *args, **kwargs):
        cmd = kwargs.get('cmd')
        msg = kwargs.get('msg')

        if cmd in self.data:
            self.data[cmd].append(msg)
        else:
            self.data[cmd] = [msg]

        out = ''

        # suppress output for certain data
        if not isinstance(msg, rfquack_pb2.Packet):
            out = str(msg)

        # type-specific output
        if isinstance(msg, rfquack_pb2.Register):
            fmt = '0x{addr:02X} = 0b{value:08b} (0x{value:02X}, {value})'
            out = fmt.format(
                    **dict(addr=msg.address, value=msg.value))

        if out:
            sys.stdout.write('\n')
            sys.stdout.write(Fore.YELLOW + out)
            sys.stdout.write('\n')

    def exit(self):
        self._transport.end()

    def ready(self):
        return self._transport.ready()

    def verbose(self):
        self._transport.verbose()

    def debug(self):
        self._transport.debug()

    def quiet(self):
        self._transport.quiet()

    def _make_payload(self, klass, **fields):
        if not self.ready():
            return

        obj = klass()
        fieldNames = [x.camelcase_name for x in klass.DESCRIPTOR.fields]

        for name, value in fields.items():
            if name not in fieldNames:
                logger.warning(
                        'Skipping {} as it does not belong to: {}'.format(
                            name, fieldNames))
                continue
            try:
                setattr(obj, name, value)
                logger.info('{} = {}'.format(name, value))
            except TypeError as e:
                logger.error('Wrong type for {}: {}'.format(name, e))
            except Exception as e:
                logger.error('Cannot set field {}: {}'.format(name, e))

        payload = obj.SerializeToString()

        return payload

    def set_modem_config(self, **fields):
        klass = rfquack_pb2.ModemConfig
        payload = self._make_payload(klass, **fields)
        self._transport._send(
                command=self._transport.TOPIC_SEP.join((
                    self._transport.TOPIC_SET,
                    self._transport.TOPIC_MODEM_CONFIG)),
                payload=payload)

    def set_mode(self, mode, repeat=0):
        if not self.ready():
            return

        logger.debug('Setting mode to {}'.format(mode))

        try:
            rfquack_pb2.Mode.Value(mode)
        except ValueError:
            logger.warning(
                'No such mode "{}": '
                'please select any of {}'.
                format(mode, rfquack_pb2.Mode.keys()))
            return

        status = rfquack_pb2.Status()
        status.mode = rfquack_pb2.Mode.Value(mode)
        status.tx_repeat_default = repeat

        payload = status.SerializeToString()
        self._transport._send(
                command=self._transport.TOPIC_SEP.join((
                    self._transport.TOPIC_SET,
                    self._transport.TOPIC_STATUS)),
                payload=payload)

        self._mode = mode

    def reset(self):
        if not self.ready():
            return

        logger.debug('Resetting the radio')

        self._transport._send(
                command=self._transport.TOPIC_SEP.join((
                    self._transport.TOPIC_SET,
                    self._transport.TOPIC_RADIO_RESET)),
                payload='')

        self._mode = 'IDLE'  # don't send the idle command, just pretend

    def rx(self):
        self.set_mode('RX')

    def tx(self):
        self.set_mode('TX')

    def idle(self):
        self.set_mode('IDLE')

    def repeat(self, repeat=0):
        self.set_mode('REPEAT', repeat)

    def get_status(self):
        if not self.ready():
            return

        logger.debug('Getting status')

        self._transport._send(
                command=self._transport.TOPIC_SEP.join((
                    self._transport.TOPIC_GET,
                    self._transport.TOPIC_STATUS)),
                payload='')

    def set_register(self, addr, value):
        if not self.ready():
            return

        logger.debug('Setting value of register 0x{:02X} = 0b{:08b}'.format(
            addr, value))

        register = rfquack_pb2.Register()
        register.address = addr
        register.value = value

        payload = register.SerializeToString()
        self._transport._send(
                command=self._transport.TOPIC_SEP.join((
                    self._transport.TOPIC_SET,
                    self._transport.TOPIC_REGISTER)),
                payload=payload)

    def get_register(self, addr):
        if not self.ready():
            return

        logger.debug('Getting value of register 0x{:02X}'.format(addr))

        register = rfquack_pb2.Register()
        register.address = addr

        payload = register.SerializeToString()
        self._transport._send(
                command=self._transport.TOPIC_SEP.join((
                    self._transport.TOPIC_GET,
                    self._transport.TOPIC_REGISTER)),
                payload=payload)

    def set_packet(self, data, repeat=1):
        if not self.ready():
            return

        packet = rfquack_pb2.Packet()
        packet.data = data

        try:
            packet.repeat = int(repeat)
        except Exception as e:
            logger.warning('Cannot set repeat of packet: {}'.format(e))

        payload = packet.SerializeToString()
        self._transport._send(
                command=self._transport.TOPIC_SEP.join((
                    self._transport.TOPIC_SET,
                    self._transport.TOPIC_PACKET)),
                payload=payload)

    send = set_packet

    def reset_packet_modifications(self):
        """
        Reset any packet modification.
        """
        if not self.ready():
          return

        logger.debug('Resetting packet modifications')

        self._transport._send(
                command=self._transport.TOPIC_SEP.join((
                    self._transport.TOPIC_SET,
                    self._transport.TOPIC_PACKET_MODIFICATION)),
                payload='')

    def get_packet_modifications(self):
        """
        Get list of packet modifications
        """
        if not self.ready():
            return

        self._transport._send(
                command=self._transport.TOPIC_SEP.join((
                    self._transport.TOPIC_GET,
                    self._transport.TOPIC_PACKET_MODIFICATION)),
                payload='')

    def add_packet_modification(self, **fields):
        klass = rfquack_pb2.PacketModification
        payload = self._make_payload(klass, **fields)
        self._transport._send(
                command=self._transport.TOPIC_SEP.join((
                    self._transport.TOPIC_SET,
                    self._transport.TOPIC_PACKET_MODIFICATION)),
                payload=payload)

    def reset_packet_filters(self):
        """
        Reset any packet filter.
        """
        if not self.ready():
            return

        logger.debug('Resetting packet filters')

        self._transport._send(
                command=self._transport.TOPIC_SEP.join((
                    self._transport.TOPIC_SET,
                    self._transport.TOPIC_PACKET_FILTER)),
                payload='')

    def get_packet_filters(self):
        """
        Get list of packet filters
        """
        if not self.ready():
            return

        self._transport._send(
                command=self._transport.TOPIC_SEP.join((
                    self._transport.TOPIC_GET,
                    self._transport.TOPIC_PACKET_FILTER)),
                payload='')

    def add_packet_filter(self, **fields):
        klass = rfquack_pb2.PacketFilter
        payload = self._make_payload(klass, **fields)
        self._transport._send(
                command=self._transport.TOPIC_SEP.join((
                    self._transport.TOPIC_SET,
                    self._transport.TOPIC_PACKET_FILTER)),
                payload=payload)


class RFQuackShell(object):
    """
    The RFQuack shell is a simple wrapper of IPython. It creates an interactive
    shell, through which the user can talk to the RFQuack dongle via a given
    transport.
    """
    def __init__(self, token, banner, transport):
        self._banner = banner
        self._transport = transport
        self._token = token

    def __call__(self):
        from IPython.terminal.prompts import Prompts, Token
        from IPython.terminal.interactiveshell import TerminalInteractiveShell

        token = self._token

        class RFQuackShellPrompts(Prompts):
            def in_prompt_tokens(self, cli=None):
                return [(Token, token), (Token.Prompt, '> ')]

        TerminalInteractiveShell.prompts_class = RFQuackShellPrompts
        shell = TerminalInteractiveShell()
        shell.autocall = 2
        shell.show_banner(self._banner)

        q = RFQuack(self._transport, shell)
        q.idle()
        shell.push(dict(q=q, rfquack_pb2=rfquack_pb2))

        shell.mainloop()


@click.group()
@click.option('--loglevel', '-l', type=click.Choice(LOG_LEVELS), default='DEBUG')
def cli(loglevel):
    coloredlogs.install(level=loglevel, logger=logger)


@cli.command()
@click.option('--client_id', '-i', default='RFQuackShell')
@click.option('--host', '-H', default='localhost')
@click.option('--port', '-P', type=int, default=1883)
@click.option('--username', '-u')
@click.option('--password', '-p')
def mqtt(client_id, host, port, username, password):
    '''
    RFQuack client with MQTT transport. Assumes one dongle per MQTT broker.
    '''
    kwargs = dict(
            client_id=client_id,
            host=host,
            port=port,
            username=username,
            password=password)

    transport = RFQuackMQTTTransport(**kwargs)

    s = RFQuackShell(
            'RFQuack({}, {}:{})'.format(client_id, host, port),
            io.open(BANNER_FILE).read(),
            transport)
    s()


@cli.command()
@click.option(
        '--baudrate', '-b',
        type=int,
        default=115200)
@click.option(
        '--bytesize', '-s',
        type=int,
        default=serial.serialutil.EIGHTBITS)
@click.option(
        '--parity', '-p',
        type=click.Choice(serial.serialutil.PARITY_NAMES),
        default=serial.serialutil.PARITY_NONE)
@click.option(
        '--stopbits', '-S',
        type=click.Choice(map(str, (
            serial.serialutil.STOPBITS_ONE,
            serial.serialutil.STOPBITS_ONE_POINT_FIVE,
            serial.serialutil.STOPBITS_TWO))),
        default=str(serial.serialutil.STOPBITS_ONE))
@click.option(
        '--timeout', '-t',
        type=int,
        default=None)
@click.option(
        '--port', '-P',
        required=True)
def tty(baudrate, bytesize, parity, stopbits, timeout, port):
    '''
    RFQuack client with serial transport.
    '''
    kwargs = dict(
            baudrate=baudrate,
            port=port,
            bytesize=bytesize,
            parity=parity,
            stopbits=int(stopbits),
            timeout=timeout)

    transport = RFQuackSerialTransport(**kwargs)
    s = RFQuackShell(
            'RFQuack({}, {},{},{},{})'.format(
                port, baudrate, bytesize, parity, stopbits),
            io.open(BANNER_FILE).read(),
            transport)

    s()


def main():
    cli()


if __name__ == '__main__':
    sys.exit(main())

# EOF
