![RFQuack Logo](docs/imgs/logo.png)

RFQuack is the versatile RF-analysis tool that quacks! It's a library firmware
that allows you to sniff, manipulate, and transmit data over the air. And if
you're not happy how the default firmware functionalities, we made it easy to
extend. Consider it as the hardware-modular and developer-friendly version of
the great [YardStick One](https://greatscottgadgets.com/yardstickone/), which
is based on the CC1111 radio chip. Differently from that and other RF dongles,
RFQuack is designed to be agnostic with respect to the radio chip. So if you
want to use, say, the RF69, you can do it. If you need to use the CC110L or
CC1120, you can do it. Similarly to RFCat, RFQuack has console based, Python
scriptable, client that allows you to set parameters, receive, transmit, and so
on.

We assume you know what you're doing 🤓

![RFQuack Demo](docs/imgs/rfquack-serial-wifi.gif)

# Table of Contents
* [Another RF-analysis Dongle?](#another-rf-analysis-dongle)
* [Quick Start Usage](#quick-start-usage)
  * [Prepare Your Hardware](#prepare-your-hardware)
  * [Build and Test](#build-and-test)
* [Interact with the RFQuack Hardware](#interact-with-the-rfquack-hardware)
* [Architecture](#architecture)
* [Main Functionalities](#main-functionalities)
  * [Modem Configuration](#modem-configuration)
  * [Transmit and Receive](#transmit-and-receive)
  * [Register Access](#register-access)
  * [Packet Filtering and Manipulation](#packet-filtering-and-manipulation)
  * [Frequency Synthesizer Calibration](#frequency-synthesizer-calibration)
* [License (GPLv2)](#license)

# Another RF-analysis Dongle?
Not really. RFQuack is midway between software-defined radios (SDRs), which offer great
flexibility at the price of a fatter code base, and RF dongles, which offer
great speed and a plug-and-play experience at the price of less flexibility
(you can't change the radio module).

RFQuack is unique in these ways:

* It's a **library** firmware, with many settings, sane defaults, and rich logging and debugging functionalities.
* Supports **multiple radio chips**: RF69, RF95, CC1120, basically all the chips supported by [RadioHAL](https://github.com/trendmicro/RadioHAL) (our GPLv2 fork of [RadioHead](https://www.airspayce.com/mikem/arduino/RadioHead/)), and we're adding more.
* Does not require a **wired connection** to the host computer: the serial port is used only to display debugging messages, but the interaction between the client and the node is over TCP using WiFi (via Arduino WiFi) and GPRS (via [TinyGSM](https://github.com/vshymanskyy/TinyGSM) library) as physical layers.
* The IPython client allows both **high- and low-level operations**: change frequency, change modulation, etc., as well as to interact with the radio chip via registers.
* The firmware and its API support the concept of **packet-filtering** and **packet-modification rules**, which means that you can instruct the firmware to listen for a packet matching a given signature (in addition to the usual sync-word- and address-based filtering, which normally happen in the radio hardware), optionally modify it right away, and re-transmit it.

So, if you need to analyze a weird RF protocol with that special packet format
or that very special modulation scheme, with mixed symbol encodings (yes, I'm
looking at you, CC1120 in 4-FSK mode 🤬), with RFQuack you just swap the radio
shield and you can just start working right away. And if we don't support that
special radio chip, you can just craft your shield and add support to the software!

# Quick Start Usage
RFQuack is quite experimental, expect glitches and imperfections. So far we're quite happy with it, and used it successfully to analyze some industrial radio protocols (read the [Trend Micro Research white paper](https://www.trendmicro.com/vinfo/us/security/news/vulnerabilities-and-exploits/attacks-against-industrial-machines-via-vulnerable-radio-remote-controllers-security-analysis-and-recommendations) or the [DIMVA 2019 paper](https://www.dimva2019.org) for details).

## Prepare Your Hardware
* choose radio chip and the board you want to use among the supported ones: we tested it with the CC1120 and RFM69HCW on ESP8266-based boards (namely the Adafruit Feather HUZZAH and the WEMOS D1 Lite);
* assemble the board and the radio chip together: if you choose the Adafruit Feather system, all you have to do is stack the Feather HUZZAH and the Radio FeatherWing together, and do some minor soldering;
* connect the board to the USB port.

| **Main board** | **Radio daughter board** | **Network connectivity** | **Cellular connectivity** |
|----------------|-------------------------------------|----------------------|-----------------------|
| Feather HUZZAH | Radio FeatherWing RFM69HW @ 433 MHz | ESPWiFi | Not tested |
| WEMOS D1 Lite | RFM69HCW @ 433 MHz | ESPWiFi | Not tested |
| WEMOS D1 Lite | CC1120 | ESPWiFi | Not tested |
| Feather FONA | Radio FeatherWing RFM69HW @ 433 MHz | None | Tested with early versions of RFQuack |

You could play around with other combinations, of course. And if you feel generous, you can fork this repository, add support for untested hardware, and send us a pull request (including schematics for new daughter-boards)! 👏

![RFQuack Boards](docs/imgs/hardware.jpg)

## Prepare Your Software
RFQuack comes in the form of a firmware *library*, which means that you need to write your own "main" to define a minimum set of parameters. Don't worry, there's not much to write in there, and we provide a [few working examples](https://github.com/trendmicro/RFQuack/blob/master/examples/).

* checkout this repository: `git clone https://github.com/trendmicro/RFQuack`
* enter the main directory: `cd RFQuack`
* install Python dependencies: `pip install -r src/client/requirements.pip` (note that this will automatically install [PlatformIO Core (CLI)](https://docs.platformio.org/en/latest/core.html), so you might want to remove such dependency if you have it installed already)
* install the dependencies listed in `library.json` via `pio install -g <library name>` (if you want to install them globally)
* to talk to your RFQuack dongle, you have two options:
  * **MQTT Transport (and hardware serial console):** install or have access to an MQTT broker (Mosquitto is just perfect for this):
    * PROs:
      * you don't need cables (hint: your RFQuack hardware can be battery powered)
      * if you want to connect the RFQuack hardware to your computer, you get a free (hardware) serial console for monitoring on the USB port
    * CONs:
      * you need network connectivity (WiFi or cellular)
      * there's latency
  * **Hardware Serial Transport (and software serial console):** connect the dongle via USB
    * PROs:
      * there's little latency
      * you don't need to rely on network stability
    * CONs:
      * if you want full monitoring and debugging capabilities, you'll need to hookup a UART cable to the RFQuack hardware (by default, a software serial device is used, and will write on pins 16, 12 (RX, TX); this can be changed by defining `RFQUACK_LOG_SS_RX_PIN` and `RFQUACK_LOG_SS_TX_PIN` before `#include <rfquack.h>`)
      * your range is limited by the length of your USB cable (you don't say! 😮)
* configure the firmware: best if you use one of the proposed examples

## Build and Test
* if you're using the MQTT transport, fire up the MQTT broker (hint: use `mosquitto -v` so you'll see debug messages)
* choose one of the examples or build your own
* `make && sleep 1 && make upload && sleep 1`
* if you want to see the debug messages, connect a serial terminal to the appropriate port (hardware serial if you're using MQTT transport, software serial if you're using the hardware serial as a transport)
* you should now see something like this (if it's not 100% the same, and if you get garbled output at the beginning, it's OK)

```
$ make monitor  # or pio device monitor --port <YOUR SERIAL MONITOR PORT> --baud 115200

{$␀lܟ|␀�$�|␂␌␄␌�␄l�␄c<��b��'o�␀d��l`␃�␛␓no␌d`␃␇␃o;���o␌␄#␌�␇l␇s��g␄␌c␄�␏d�␛l

[RFQ]        152 T: Setting sync words length to 4
[RFQ]        153 T: Packet filtering data initialized
[RFQ]        154 T: Packet modification data initialized
[RFQ]        156 T: RFQuack data structure initialized: WEMOSD1_CC1120
[RFQ]        464 T: Connecting WEMOSD1_CC1120_6c54 to MQTT broker 192.168.42.225:1883
[RFQ]       2117 T: MQTT connected
[RFQ]       2130 T: Subscribed to topic: rfquack/in/#
[RFQ]       2231 T: 📡 Setting up radio (CS: 15, RST: 5, IRQ: 4)
[RHAL] SRES
[RHAL] SCAL
[RHAL] SIDLE
[RHAL] START MARCSTATE.MARC_STATE ==============================
[RHAL] Waiting for MARCSTATE.MARC_STATE == 0b1
[RHAL] END MARCSTATE.MARC_STATE ==============================
[RHAL] IRQ bus clear
[RHAL] _variablePayloadLen = 1
[RFQ]       3141 T: 📶 Radio initialized (debugging: true)
[RFQ]       3142 T: CC1120 type 0x4823 ready to party 🎉
[RFQ]       3144 T: Modem config set to 5
[RFQ]       3147 T: Max payload length:  128 bytes
[RFQ]       3151 T: 📶 Radio is fully set up (RFQuack mode: 4, radio mode: 2)
[RFQ]       3258 T: Transport is sending 26 bytes on topic rfquack/out/status
```

# Interact with the RFQuack Hardware
Now you can use RFQuack via the IPython shell. We highly recommend tmux to keep an eye on the output log.

![RFQuack Console](docs/imgs/console1.png)
![RFQuack Console](docs/imgs/console2.png)
```bash
$ python src/client/rfq.py --help                      
Usage: rfq.py [OPTIONS] COMMAND [ARGS]...                                
                                                                         
Options:                                                                 
  -l, --loglevel [CRITICAL|ERROR|WARNING|INFO|DEBUG|NOTSET]              
  --help                          Show this message and exit.            
                                                                         
Commands:                                                                
  mqtt  RFQuack client with MQTT transport.                              
  tty   RFQuack client with serial transport.                            

$ python src/client/rfq.py mqtt --help                 
Usage: rfq.py mqtt [OPTIONS]                                             
                                                                         
  RFQuack client with MQTT transport. Assumes one dongle per MQTT broker.
                                                                         
Options:                                                                 
  -i, --client_id TEXT                                                   
  -H, --host TEXT                                                        
  -P, --port INTEGER                                                     
  -u, --username TEXT                                                    
  -p, --password TEXT                                                    
  --help                Show this message and exit.                      

$ python src/client/rfq.py tty --help                  
Usage: rfq.py tty [OPTIONS]                                              
                                                                         
  RFQuack client with serial transport.                                  
                                                                         
Options:                                                                 
  -b, --baudrate INTEGER                                                 
  -s, --bytesize INTEGER                                                 
  -p, --parity [M|S|E|O|N]                                               
  -S, --stopbits [1|1.5|2]                                               
  -t, --timeout INTEGER                                                  
  -P, --port TEXT           [required]                                   
  --help                    Show this message and exit.                  
```

More concretely:

```python
$ python src/client/rfq.py mqtt -H localhost -P 1884
2019-04-10 18:04:31 local RFQuack[20877] INFO Transport initialized
2019-04-10 18:04:31 local RFQuack[20877] DEBUG Setting mode to IDLE
2019-04-10 18:04:31 local RFQuack[20877] DEBUG rfquack/in/set/status (2 bytes)
2019-04-10 18:04:31 local RFQuack[20877] INFO Transport pipe initialized (QoS = 2): mid = 2

...

In [1]: q.rx()
2019-04-10 18:04:45 local RFQuack[20877] DEBUG Setting mode to RX
2019-04-10 18:04:45 local RFQuack[20877] DEBUG rfquack/in/set/status (2 bytes)

In [2]: q.set_modem_config(modemConfigChoiceIndex=0, txPower=14, syncWords='', carrierFreq=433)
2019-04-10 18:04:58 local RFQuack[20877] INFO txPower = 14
2019-04-10 18:04:58 local RFQuack[20877] INFO modemConfigChoiceIndex = 0
2019-04-10 18:04:58 local RFQuack[20877] INFO syncWords =
2019-04-10 18:04:58 local RFQuack[20877] INFO carrierFreq = 433
2019-04-10 18:04:58 local RFQuack[20877] DEBUG rfquack/in/set/modem_config (11 bytes)

...

In [73]: 2019-04-10 18:24:16 local RFQuack[20877] DEBUG Message on topic rfquack/out/status
2019-04-10 18:24:16 local RFQuack[20877] DEBUG rfquack/out/status -> <class 'rfquack_pb2.Status'>: stats {
  rx_packets: 0
  tx_packets: 0
  rx_failures: 0
  tx_failures: 0
  tx_queue: 0
  rx_queue: 0
}
mode: IDLE
modemConfig {
  carrierFreq: 433.0
  txPower: 14
  isHighPowerModule: true
  preambleLen: 4
  syncWords: "CB"
}
```

The last message (i.e., on the `rfquack/out/status` topic) is automatically sent by the RFQuack dongle at first boot, and shows that the dongle is up and running, with some basic info about its status.

At this point you're good to go from here!

# Architecture

![RFQuack Architecture](docs/imgs/RFQuack%20Architecture.png)

RFQuack has a modular software and hardware architecture comprising:

* a radio chip (usually within a module)
* a micro-controller unit (MCU)
* an optional network adapter (cellular or WiFi)

The communication layers are organized as follows:

* The Python client encodes the message for RFQuack with Protobuf (via [nanopb](https://github.com/nanopb/nanopb)): this ensures data-type consistency across firmware (written in C) and client (written in Python), light data validation, and consistent development experience.
* The serialized messages are transported over MQTT (which allows multi-node and multi-client scenarios) or serial (when you need minimal latency).
* The connectivity layer is just a thin abstraction over various cellular modems and the Arduino/ESP WiFi (or simply serial).

# Main Functionalities
RFQuack is meant to be as generic as possible. What's not directly abstracted with an function call can be accomplished by setting the registers via the `set_register` function. In the following, we explore the main functionalities through some examples.

When you fire up the Python shell, you can interact with the API through the `q` object. If unsure which parameters a function can take please check the `src/rfquack.proto` protocol definition. Since we're using reflection, IPython can't offer completion here (if you know a way to have completion on dynamic attributes, please let us know!).

## Modem Configuration
RFQuack's radio sub-system is based on [RadioHAL](https://github.com/trendmicro/RadioHAL/) (our GPLv2 fork of RadioHead), so for most aspects you can refer to the RadioHead documentation. The key difference between RadioHead and RadioHAL is that RadioHAL does not make any assumption on the packet format: it just gives you straight access to the payload. This is true for RFM69 and CC1120 (newly added!), while we're still in the process of removing payload parsing routines from the drivers of the other radio chips supported by RadioHead.

Not all radio modules support modem configuration. Sub-gigahertz modems usually do. The `q.set_modem_config()` function takes the following parameters:

* `modemConfigChoiceIndex`: this refers to the RadioHAL (fork of RadioHead) pre-defined modem configuration (a.k.a., `ModemConfigChoice`), which are common settings for modulation, bitrate, frequency deviation, bandwidth, and so on. Depending on the radio chip you're using, check the RadioHead documentation (e.g., [for the RFM69](https://www.airspayce.com/mikem/arduino/RadioHead/classRH__RF69.html#a8b7db5c6e4eb542f46fec351b2084bbe)). We'll document RadioHAL at some point in the future.
* `txPower` and `isHighPowerModule`: these parameters control the transmission power; set them wisely and make sure to follow the laws that apply to your country; also, note that some radio modules (e.g., RFM69HCW) need to be set in "high-power mode" (that's what the 'H' stands for), so if you set `txPower` below a certain minimum value, say 14, you will get zero transmission power. In particular, these modules will need `txPower >= 14` and, `isHighPowerModule = true`.
* `syncWords`: sync-word matching is a basic functionality of most packet-radio modules, which allow to efficiently filter packets that match the sync words and just ignore the rest, in order to keep the radio chip and the MCU busy only when an expected packet is received; depending on the radio module, the sync words can be set to zero (promiscuous mode) or up to a certain number of octects (e.g., 4); in promiscuous mode, the radio and MCU will be *very* busy, because they will pick up *everything*, including noise.
* `carrierFreq`: this is the carrier frequency, easy; make sure you comply to the radio module you chosen.

## Transmit and Receive
The `q.tx()` and `q.rx()` functions are self-explanatory: they set the module in transmit and receive mode, respectively. To actually transmit data, you can use `q.set_packet(data, repeatitions)`, where data must be a list of raw octect values (e.g., `'\x43\x42'`) as well as a list of ASCII symbols (e.g., `'ABC'`); there's a limit in the length, which is imposed by the radio module, so make sure you check the documentation.

By default, a packet is transmitted only once. If you want to repeat it, just set `repetitions` to whatever you want, and RFQuack will repeat the transmission as fast as possible (bound by the MCU clock, of course).

## Register Access
While RadioHead (and thus its fork RadioHAL) has gone very far in abstracting the interaction with the radio, some radio chips are really "unique," so to speak. In these cases, the only option is to grab a large cup of your favorite beverage, read through the datasheet, read again, again, and again.

Once you understand enough of how the radio works at the low level, you want to get-set registers in order to use it. In principle, you can do pretty much everything via registers.

There's not much else to say here. Just use `q.get_register(addr)` and `q.set_register(addr, value)`, and recall that Python lets you do nice things like `q.set_register(0x37, 0x01001100)` so you don't have to do any conversions.

```python
In [34]: q.set_register(0x38, 0b10000000)
2019-04-10 18:11:49 local RFQuack[20877] DEBUG Setting value of register 0x38 = 0b10000000
```

Note that every call to `q.set_modem_config()` will **reset the modem including several registers** to their default values (according to the datasheet). Also, many radio chips need to be in an "idle" state while setting certain registers. Please check the datasheet and use `q.idle()` before setting registers to be on the safe side. Last, be wise and double check that the values you set are actually there, using `get_register` after each `set_register`.

We noticed some timing issues with some radio chips. So, allow a small delay if you're setting many registers in a row (e.g., `for addr, value in regs: q.set_register(addr, value); time.sleep(0.2)`).

## Packet Filtering and Manipulation
One of the main reasons why we created RFQuack is that we wanted to automate certain tasks in a flexible and fast way. For instance, we were building a PoC for a vulnerability in a radio protocol that, with a change in two bytes of the payload, the vulnerable receiver would execute another command. So, all we had to do was: stay in RX mode, wait for a packet matching a pattern, alter it, and re-transmit it.

Most of this could be done with an SDR or with a RF-dongle and RFCat, but in both cases you'd have to "pay" the round-trip time from the radio, to the client, and back. For certain protocols, this timing is not acceptable. RFQuack's firmware implements this functionality natively, and exposes a simple API to configure packet filtering and manipulation.

**Important:** filtering and patterns are applied past any filtering performed by the radio (e.g., based on sync words, address, CRC, RSSI, LQI). If you want to consider any packet, including noise, you'll have to disable these low-level filters via `set_register()`, knowing the specs of the radio. The following diagram gives a simplified representation of the packet filtering pipeline implemented by most of embedded radio modules (e.g., RF69, CC110L, CC1120).

![RFQuack Radio Architecture](docs/imgs/RFQuack%20Radio%20Architecture.png)

RFQuack's packet filtering and manipulation pipeline is still running as native code on the MCU, so it's pretty efficient.

![RFQuack Filtering Architecture](docs/imgs/RFQuack%20Filtering%20Architecture.png)

* `q.add_packet_filter(pattern)` takes only one parameter, a regular-expression pattern complying with the [tiny-regex-c](https://github.com/kokke/tiny-regex-c) library (most common patterns are supported); adding a pattern means that RFQuack will discard any payload not matching the regular-expression pattern; you can add multiple filters (in AND logic); or you can reset them with `q.reset_packet_filter()`; the number of filters is bound by `RFQUACK_MAX_PACKET_FILTERS` (defaults to 64)

* `q.add_packet_modification()` takes several parameters
  * `position` (number, optional) indicates the position in the payload that will be modified (e.g., 3rd byte);
  * `content` (byte, optional) indicates the content that will be modified (e.g., all octects which value is `'A'`);
  * `pattern` (optional) same as for the filter: only packets matching the pattern will be modified; if no pattern is specified, all packets will be modified.
  * `operation` (enum) is the operation on the value (AND, OR, XOR, NOT, SLEFT, SRIGHT), together with an operand (except for NOT);
  * `operand` (byte) is the "right" value for the operation.

**Example:** Let's say that you want to invert byte 3 of all packets that end with `'XYZ'` and XOR with `0x44` all bytes which value is `'A'` (and in position 5) of all packets that start with `'AAA'`. And you want to ignore any packet that do not contain at least 3 digits in their payload. You're going to need two modifications and one filter:

```python
In [72]: q.add_packet_filter(
    pattern="[0-9]{3,}"  # ignore packets not containing at least 3 digits
)

# ...

In [73]: q.add_packet_modification(
    pattern="XYZ$"  # for all packets that end in "XYZ"
    position=3,     # at position 3
    operation=4     # apply a NOT of whatever value is there
                    # (no operand needed)
)

# ...

In [74]: q.add_packet_modification(
    pattern="^AAA"  # for all packets that start with "AAA"
    content=0x42,   # for all octects which value equals A
    position=5,     # and at position 5
    operation=3     # XOR the value with the operand
    operand=0x44
)

In [75]: q.repeat(10) # enable packet manipulation and re-transmission (10 times)
```

The last call puts the radio in RX mode, but whenever a matching packet is received, it'll quickly switch to TX mode to re-transmit it.

Looking at the full picture, here's the full journey of a packet within RFQuack.

![RFQuack Full Architecture](docs/imgs/RFQuack%20Full%20Architecture.png)

## Frequency Synthesizer Calibration
Recall that radio chips may have internal calibration routines (manual or
automatic) for the frequency synthesizer, which outcome may vary slightly.
Temperature is another factor that may slightly influence the actual carrier
frequency. In lack of a stable and reliable reference point, we suggest to set
the registers so as to get as close as possible to your target frequency (e.g.,
aided by a spectrogram), and then nudge around until matched.

# License
Copyright (C) 2019 Trend Micro Incorporated.

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

# Disclaimer
RFQuack is a research tool intended to analyze radio-frequency (RF) signals via
software, with native hardware support. It is not intended for malicious or
offensive purposes.
