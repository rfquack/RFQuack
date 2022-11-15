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

from rfquack.src import rfquack_pb2
from rfquack import topics
from google.protobuf.message import Message

logger = logging.getLogger("rfquack.core")


class ModuleInterface(object):
    def __init__(self, rfq, module_name):
        self.__dict__["_rfquack"] = rfq
        self.__dict__["_module_name"] = module_name
        self.__dict__["_help"] = dict()

    def _set_autocompletion(self, cmd_name, cmd_type, argument_type, description):
        self._help[cmd_name] = dict(
            cmd_type=cmd_type, argument_type=argument_type, description=description
        )
        rfq = self._rfquack

        if cmd_type == rfquack_pb2.CmdInfo.CmdTypeEnum.METHOD:
            # Add the method.
            if argument_type == "rfquack_VoidValue":
                # Create a void method
                self.__dict__[cmd_name] = lambda: rfq._set_module_value(
                    self._module_name, cmd_name, rfquack_pb2.VoidValue()
                )
            else:
                protobuf_type = getattr(
                    rfquack_pb2, argument_type[8:]
                )  # Strip initial rfquack_

                # If specified type has a single field named "value", directly assign the passed argument to it.
                if (
                    len(protobuf_type.DESCRIPTOR.fields_by_name) == 1
                    and "value" in protobuf_type.DESCRIPTOR.fields_by_name
                ):

                    self.__dict__[cmd_name] = lambda x: rfq._set_module_value(
                        self._module_name,
                        cmd_name,
                        self._parse_attribute(x, protobuf_type),
                    )
                else:
                    self.__dict__[cmd_name] = lambda **x: rfq._set_module_value(
                        self._module_name,
                        cmd_name,
                        self._parse_attribute(x, protobuf_type),
                    )

            # Create a method accepting an argument:

            logger.debug("q.{}.{}() created.".format(self._module_name, cmd_name))
            return

    def __dir__(self):
        return list(self._help.keys()) + ["help"]

    def help(self):
        sys.stdout.write("\n")
        sys.stdout.write("Helper for '{}':\n\n".format(self._module_name))
        for cmd_name in self._help:
            cmd = self._help[cmd_name]

            # Print full command and its type (method/attribute)
            sys.stdout.write("> q.{}.{}".format(self._module_name, cmd_name))
            if cmd["cmd_type"] == rfquack_pb2.CmdInfo.CmdTypeEnum.METHOD:
                sys.stdout.write("()")
            sys.stdout.write("\n")

            # Print the description
            sys.stdout.write("Accepts: {}\n".format(cmd["argument_type"]))
            sys.stdout.write(cmd["description"])
            sys.stdout.write("\n\n")

    def _parse_attribute(self, value, type):
        rfq = self._rfquack

        # None value is mapped to rfquack_VoidValue
        if value is None:
            return rfquack_pb2.VoidValue()

        # If attribute is a Protubuf object we assume user knows what he's doing.
        if isinstance(value, Message):
            return value

        # First check if the attribute we want to set is a primitive type.
        if isinstance(value, (bool, int, float, bytes)):
            return rfq._make_payload(type, value=value)

        # If attribute is a dict try to parse it.
        if isinstance(value, dict):
            return rfq._make_payload(type, **value)

        raise Exception("Don't know how to parse the input")

    def __setattr__(self, attr_name, value):

        if attr_name not in self._help.keys():
            self.help()
            return "'{}' not found".format(attr_name)

        rfq = self._rfquack
        module_name = self._module_name

        try:
            protobuf_type = getattr(
                rfquack_pb2, self._help[attr_name]["argument_type"][8:]
            )  # Strip initial rfquack_
            rfq._set_module_value(
                module_name, attr_name, self._parse_attribute(value, protobuf_type)
            )
        except Exception as error:
            logger.error("{}".format(error))

    def __getattr__(self, attr_name):
        # Ignore when IPython tries to probe our methods.
        if attr_name in (
            "_ipython_canary_method_should_not_exist_",
            "_repr_mimebundle_",
        ):
            return None

        if attr_name not in self._help.keys():
            self.help()
            return "'{}' not found".format(attr_name)

        rfq = self._rfquack
        return rfq._get_module_value(self._module_name, attr_name)


class RFQuack(object):
    """
    The RFQuack object connects to a given transport and abstracts the
    invocation of commands to the RFQuack dongle by means of function calls.
    """

    def __init__(self, transport, prefix, shell, select_first_dongle):
        self._transport = transport
        self._shell = shell

        self.data = list()
        self._prefix = prefix
        self.lastReply = {}
        self._spectrum_analyzer = dict()
        self._module_names = list()

        self._dongles = dict()
        self._select_first_dongle = select_first_dongle

        self._init()

    def _init(self):
        self._transport.init(
            on_message_callback=self._discovery_recv, prefix=topics.TOPIC_PREFIX_ANY
        )
        sys.stdout.write("\n\n" "  Select a dongle typing: q.dongle(id)")
        self._set_module_value("ping", "ping", rfquack_pb2.VoidValue())

    def _discovery_recv(self, **kwargs):
        dongle_prefix = kwargs.get("prefix")
        id = len(self._dongles)
        self._dongles[id] = dongle_prefix
        sys.stdout.write(" - Dongle {}: {}\n".format(id, dongle_prefix))

        # If select_first_dongle then automatically select the first received dongle.
        if self._select_first_dongle:
            self.dongle(id)

    def dongle(self, id):
        dongle_prefix = self._dongles.get(id)

        if dongle_prefix is None:
            sys.stdout.write("Wrong dongle id")
            return

        sys.stdout.write(
            "\n > You have selected dongle {}: {}\n\n".format(id, dongle_prefix)
        )

        # Delete any previously autoloaded module.
        for module_name in self._module_names:
            delattr(self, module_name)

        # Listen for the new dongle prefix
        self._transport.set_prefix(dongle_prefix)

        # Change the incoming messages handler.
        self._transport.set_on_message_callback(self._recv)

        # Ask info on the 'TOPIC_INFO', the dongle will reply back with the loaded modules
        self._transport._send(command=topics.TOPIC_INFO, payload=b"")

    def _recv(self, **kwargs):
        verb = kwargs.get("verb")
        module_name = kwargs.get("module_name")
        cmds = kwargs.get("cmds")
        msg = kwargs.get("msg")

        out = ""

        # Incoming autocompletion data
        if verb == topics.TOPIC_INFO.decode():
            # Create a new attribute if missing.
            if not hasattr(self, module_name):
                self._module_names.append(module_name)
                logger.info("Creating attribute q.{}".format(module_name))
                setattr(self, module_name, ModuleInterface(self, module_name))

            getattr(self, module_name)._set_autocompletion(
                cmds[0], msg.cmdType, msg.argumentType, msg.description
            )
            return

        # Store received reply.
        self.lastReply = msg

        # Print to screen incoming data
        if hasattr(msg, "DESCRIPTOR"):
            for field in list(msg.DESCRIPTOR.fields_by_name):
                out += "{} = {}\n".format(field, getattr(msg, field))
        else:
            out = "Don't know ho to serialize to CLI"

        # Type-specific output
        # Parse register info:
        if isinstance(msg, rfquack_pb2.Register):
            fmt = "0x{addr:02X} = 0b{value:08b} (0x{value:02X}, {value})\n"
            out += fmt.format(**dict(addr=msg.address, value=msg.value))

        # Parse and store incoming packet:
        if isinstance(msg, rfquack_pb2.Packet):
            out += "hex data = {}\n".format(msg.data.hex())
            self.data.append(msg)

        if out:
            sys.stdout.write("\n")
            sys.stdout.write(out)
            sys.stdout.write("\n")

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
                    "Skipping {} as it does not belong to: {}".format(name, fieldNames)
                )
                continue
            try:
                enum_type = klass.DESCRIPTOR.fields_by_camelcase_name[name].enum_type
                if enum_type is None:
                    setattr(obj, name, value)
                    logger.info("{} = {}".format(name, value))
                else:
                    if value in enum_type.values_by_name:
                        setattr(obj, name, enum_type.values_by_name[value].number)
                        logger.info("{} = {}".format(name, value))
                    else:
                        logger.error(
                            "{} must be one of {}".format(
                                name, ", ".join(enum_type.values_by_name.keys())
                            )
                        )
            except TypeError as e:
                logger.error("Wrong type for {}: {}".format(name, e))
            except Exception as e:
                logger.error("Cannot set field {}: {}".format(name, e))

        return obj

    def _send_module_cmd(self, message, module_name, verb, *args):
        if not self.ready():
            return

        topic_parts = [verb.encode(), module_name.encode()]
        topic_parts += list(map(lambda x: x.encode(), args))

        payload = message.SerializeToString()

        self._transport._send(
            command=topics.TOPIC_SEP.join(topic_parts), payload=payload
        )

    def _set_module_value(self, module_name, cmd_name, message):
        # In module's protobuf the protobuf classes are prepended by 'rfquack_'. Let's add it.
        message_type = "rfquack_{}".format(message.DESCRIPTOR.name)

        self._send_module_cmd(
            message,
            module_name,
            topics.TOPIC_SET.decode(),
            message_type,
            cmd_name,
        )

    def _get_module_value(self, module_name, attrName):
        self._send_module_cmd(
            rfquack_pb2.VoidValue(), module_name, topics.TOPIC_GET.decode(), attrName
        )
