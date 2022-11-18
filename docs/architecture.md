!!! warning
    This section is fairly incomplete.

![RFQuack Architecture](imgs/RFQuack%20Architecture.png)

RFQuack has a modular software and hardware architecture comprising:

- a radio chip (usually within a module)
- a micro-controller unit (MCU)
- an optional network adapter (cellular or WiFi)

The communication layers are organized as follows:

- The Python client encodes the message for RFQuack with Protobuf (via [nanopb](https://github.com/nanopb/nanopb)): this ensures data-type consistency across firmware (written in C) and client (written in Python), light data validation, and consistent development experience.
- The serialized messages are transported over MQTT (which allows multi-node and multi-client scenarios) or serial (when you need minimal latency).
- The connectivity layer is just a thin abstraction over various cellular modems and the Arduino/ESP WiFi (or simply serial).
- The message is decoded and handled by a software [module](#modules)
