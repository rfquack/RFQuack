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
import base64
import binascii

import serial
import serial.threaded
import serial.serialutil

import paho.mqtt.client as paho_mqtt

from rfquack import topics
from rfquack.src import rfquack_pb2

logger = logging.getLogger("rfquack.transport")


def hexelify(blob):
    return " ".join(["0x{:02X}".format(o) for o in blob])


class RFQuackTransport(object):
    """
    Every RFQuack transport is based on messages, which are composed by a topic
    and an optional (binary) payload. The payload is a serialized Protobuf
    message.

    The concept of topics is borrowed by MQTT. Indeed, the most obvious
    implementation of the RFQuack transport is on top of MQTT. Each topic is
    like a path, formed by three parts, split by a separator, usually a slash.

        <prefix>/<way>/<set|get|info>/<module_name>/<message_type>/<args>

    The <prefix> (e.g., rfquack) is used to distinguish correct messages.

    The direction, or <way> (e.g., in, out), indicates that indicates whether
    the message is inbound or outbound. Inbound messages are going from this
    client to the RFQuack dongle. Outbound messages come from the RFQuack
    dongle and are directed to this client.

    The <get|set> part indicates whether a message is intended to set or get a
    value. The semantic of this part is implemented in the RFQuack firmware.
    For example, if we need to set a register to a value, we will use a topic
    such as 'rfquack/in/set/<module_name>/...'.

    The module name indicates which module should handle the command.
    For example: 'rfquack/in/set/radio_module/....'

    The message type indicates the protobuf type of the serialized message
    For example: 'rfquack/in/set/radio_module/rfquack_Void'

    The args are optional and must be the last part, they add arguments
    to the message, they are used as arguments for the module:
    Following the above example: 'rfquack/in/set/radio_module/rfquack_Void/reset'

    Once a message is received, it is dispatched to the correct module,
    together with its payload, which must be deserialized according to the
    right Protobuf message class.
    """

    def __init__(self, *args, **kwargs):
        self._ready: bool = False
        raise NotImplementedError("You must override the constructor")

    def ready(self):
        return self._ready

    def _message_parser(self, topic, payload):
        try:
            prefix, way, verb, module_name, message_type, *cmds = topic.split(
                topics.TOPIC_SEP
            )
        except Exception:
            logger.warning(
                "Cannot parse topic: "
                "must be <prefix>/<way>/<verb>/<module_name>/<message_type>(/<args>)+"
            )
            return

        logger.debug('Message from module "{}"'.format(module_name))

        if prefix != self._prefix and self._prefix != topics.TOPIC_PREFIX_ANY:
            logger.warning(
                "Invalid prefix: {} should be {}".format(prefix, self._prefix)
            )
            return

        if way != topics.TOPIC_OUT:
            return

        # Retrive protobuf class name by stripping 'rfquack_' prefix from message_type
        klass = rfquack_pb2.__dict__.get(message_type[8:].decode(), None)

        if klass is None:
            logger.warning(
                f'Ignoring "{message_type[8:]}": doesn\'t match any known protobuf class'
            )
            return None

        pb_msg = klass()
        try:
            pb_msg.ParseFromString(payload)
        except Exception as e:
            logger.error("Cannot deserialize data: {}".format(e))
            return

        if self._on_message_callback:
            self._on_message_callback(
                prefix=prefix.decode(),
                verb=verb.decode(),
                module_name=module_name.decode(),
                cmds=list(map(lambda x: x.decode(), cmds)),
                msg=pb_msg,
            )

    def set_on_message_callback(self, on_message_callback):
        self._on_message_callback = on_message_callback

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

    SERIAL_PREFIX_IN = b"<"  # packet for us
    SERIAL_PREFIX_OUT = b">"  # packet for the dongle
    SERIAL_SUFFIX = b"\0"
    SERIAL_SEPARATOR = b"~"
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
        logger.info("Port opened")

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
            print("DATA CHUNK RECEIVED = '{}'".format(data))

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
                    len(payload), topic, binascii.hexlify(payload)
                )
            )

            if self.callback:
                self.callback(topic, payload)
                # try:
                #    self.callback(topic, payload)
                # except Exception as e:
                #    logger.error('Cannot parse message: {}'.format(e))
        else:
            logger.error("Unexpected data format: {}".format(packet))

    def write_packet(self, topic, payload):
        # {prefix}{topic}{sep}{payload}{suffix}'
        data = b"".join(
            (
                self.SERIAL_PREFIX_OUT,
                topic,
                self.SERIAL_SEPARATOR,
                base64.b64encode(payload),
                self.SERIAL_SUFFIX,
            )
        )

        if self._verbose:
            logger.debug("Writing packet = {}".format(data))
        return self.transport.write(data)

    def handle_out_of_packet_data(self, byte):
        """Accumulate bytes until a terminator is found, look for the
        begin-of-log-line tokens, and consider it a packet"""
        self.line.extend(byte)

        for tok in self.SERIAL_LOG_TOKENS:
            if tok in self.line and self.line.endswith(self.SERIAL_LOG_NEWLINE):
                s = self.line.index(tok)
                self.handle_log(bytes(self.line[s:]))
                self.line = bytearray()

    def handle_log(self, line):
        # TODO find the right way to print in IPython
        if self._verbose:
            print(
                "\033[94m" + line,
            )


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

        if "port" not in kwargs:
            raise ValueError("Please specify the port")

        self.args = args
        self.kwargs = kwargs
        self.ser = None
        self._on_message_callback = None
        self._prefix = topics.TOPIC_PREFIX_ANY

    def init(self, *args, **kwargs):
        self._on_message_callback = kwargs.get("on_message_callback")
        self._prefix = kwargs.get("prefix")
        self.ser = serial.Serial(**self.kwargs)

        class _RFQuackSerialProtocol(RFQuackSerialProtocol):
            callback = self._on_message

        self._reader = serial.threaded.ReaderThread(self.ser, _RFQuackSerialProtocol)

        self._reader.start()
        self._transport, self._protocol = self._reader.connect()
        self._ready = True

    def set_prefix(self, prefix):
        self._prefix = prefix.encode()

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
        topic = topics.TOPIC_SEP.join((self._prefix, topics.TOPIC_IN, command))
        logger.debug("{} ({} bytes)".format(topic, len(payload)))
        # logger.debug('payload = {}'.format(hexelify(bytearray(payload))))

        self._protocol.write_packet(topic, payload)


class RFQuackMQTTTransport(RFQuackTransport):
    """
    MQTT transport implements the RFQuack protocol by mapping topics and
    payloads onto valid MQTT messages.
    """

    QOS = 2
    RETAIN = False

    def __init__(
        self,
        client_id,
        username=None,
        password=None,
        host="localhost",
        port=1883,
        certificate_authority=None,
        certificate=None,
        private_key=None,
    ):

        self._userdata = {}
        self._username = username
        self._password = password
        self._mqtt = dict(host=host, port=port)
        self._client = paho_mqtt.Client(client_id=client_id, userdata=self._userdata)

        if certificate_authority:
            self.QOS = 1
            self._client.tls_set(
                ca_certs=certificate_authority,
                certfile=certificate,
                keyfile=private_key,
            )

        self._ready = False
        self._on_packet = None
        self._prefix = topics.TOPIC_PREFIX_ANY

    def init(self, *args, **kwargs):
        self._client.on_message = self._on_message
        self._client.on_connect = self._on_connect
        self._client.on_subscribe = self._on_subscribe

        if kwargs.get("on_message_callback"):
            self._on_message_callback = kwargs.get("on_message_callback")

        if self._username:
            self._client.username_pw_set(self._username, self._password)

        self._client.connect(self._mqtt.get("host"), self._mqtt.get("port"))

        logger.info("Transport initialized")

        self._client.loop_start()

        self._ready = True

    def get_subscribe_url(self, topic_prefix):
        # "any" in MQTT world is matched to a '+'
        if topic_prefix is topics.TOPIC_PREFIX_ANY:
            return topics.TOPIC_SEP.join((b"+", topics.TOPIC_OUT, b"#")).decode()

        return topics.TOPIC_SEP.join((topic_prefix, topics.TOPIC_OUT, b"#")).decode()

    def set_prefix(self, prefix):
        # Unsubscribe from old topic
        self._client.unsubscribe(self.get_subscribe_url(self._prefix))

        self._prefix = prefix.encode()

        # Subscribe to new one
        url = self.get_subscribe_url(self._prefix)
        self._client.subscribe(url, qos=self.QOS)

    def end(self):
        self._ready = False
        self._client.loop_stop()

    def _on_connect(self, client, userdata, flags, rc):
        url = self.get_subscribe_url(self._prefix)
        self._client.subscribe(url, qos=self.QOS)
        logger.info("Connected to broker. Feed = {}".format(url))

    def _on_subscribe(self, client, userdata, mid, granted_qos):
        logger.info(
            "Transport pipe initialized (QoS = {}): mid = {}".format(
                granted_qos[0], mid
            )
        )

    def _on_message(self, client, userdata, msg):
        self._message_parser(msg.topic.encode("utf-8"), msg.payload)

    def _send(self, command, payload):
        topic = topics.TOPIC_SEP.join((self._prefix, topics.TOPIC_IN, command))
        logger.debug("{} ({} bytes)".format(topic, len(payload)))

        self._client.publish(
            topic.decode("utf-8"), payload=payload, qos=self.QOS, retain=self.RETAIN
        )
