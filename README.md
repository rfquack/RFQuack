# RFQuack ![Build Status](https://github.com/rfquack/RFQuack/actions/workflows/build.yml/badge.svg)

The only RF-analysis tool that quacks! 🦆

RFQuack is a versatile RF-analysis tool that allows you to sniff, analyze, and transmit data over the air.

Similarly to RFCat RFQuack has a Python-based scriptable shell that allows you to set parameters, receive, transmit, and so on.

## Documentation

We're building an official [documentation](https://rfquack.org) currently very work in progress.

## Supported Radios

We porting from (and contribute back to) [RadioLib](https://github.com/jgromes/RadioLib). So far, we support:

* __CC1101__ OOK, 2-FSK, 4-FSK, MSK radio module
* __nRF24L01__ 2.4 GHz module
* __RF69__ FSK, OOK radio module

## Supported Arduino Platforms

In principle, RFQuack can run on any board and platform supported by [PlatformIO](https://docs.platformio.org). So far, we tested the following boards:

* **Espressif**
  * [**ESP32**](https://github.com/espressif/rduino-esp32) - ESP32-based boards

* **PJRC**
  * [**Teensy**](https://github.com/PaulStoffregen/cores) - Teensy 2.x, 3.x and 4.x boards

## Demos

[![RFQuack DEMO - Packet capture and filtering via interactive CLI](https://img.youtube.com/vi/_59SRwfS6PU/0.jpg)](https://www.youtube.com/watch?v=_59SRwfS6PU)

[![RFQuack DEMO - Analyzing 2.4GHz RF protocols](https://img.youtube.com/vi/XNLTjWi7cPo/0.jpg)](https://www.youtube.com/watch?v=XNLTjWi7cPo)
