# Black Hat Arsenal (Europe 2022)

![Black Hat Europe 2022 Arsenal](https://rawgit.com/toolswatch/badges/master/arsenal/europe/2022.svg)

This folder contains notes and the code used to demonstrate RFQuack at Black Hat Arsenal (Europe 2022).

- [Black Hat Arsenal (Europe 2022)](#black-hat-arsenal-europe-2022)
  - [Signal Generator](#signal-generator)
  - [RFQuack Dongles](#rfquack-dongles)
    - [Building the RFQuack Firmware](#building-the-rfquack-firmware)
  - [DEMO 1: Finding and Decoding a Signal](#demo-1-finding-and-decoding-a-signal)
  - [DEMO 2: Firmware Customization](#demo-2-firmware-customization)
    - [DEMO 2.1: RF69 433MHz (`demo2.1-build.env`)](#demo-21-rf69-433mhz-demo21-buildenv)
    - [DEMO 2.2: Dual RF69 433MHz (`demo2.2-build.env`)](#demo-22-dual-rf69-433mhz-demo22-buildenv)
    - [DEMO 2.3: Dual nRF24 2.4GHz and RF69 433MHz (`demo2.3-build.env`)](#demo-23-dual-nrf24-24ghz-and-rf69-433mhz-demo23-buildenv)
    - [DEMO 2.4: Enabling Modules (`demo2.4-build.env`)](#demo-24-enabling-modules-demo24-buildenv)
  - [DEMO 3: Scripting Attacks (`demo3-build.env`)](#demo-3-scripting-attacks-demo3-buildenv)

## Signal Generator

We'll use a signal generator to simulate an interactive IoT node. Please check th [nodes/sig-gen/](nodes/sig-gen/) subfolder for instructions.

## RFQuack Dongles

We'll use RFQuack dongles described in the `build.env` file in each [*-build.env](build.env/) file. If necessary, change the pin numbers according to your wiring, and copy it in the root of the RFQuack repository and you'll be ready to go. Be careful, because some pins are not good as IRQs or CSs.

If you're experiencing delays, consider removing the `LOG_ENABLED` setting.

### Building the RFQuack Firmware

```shell
git clone https://github.com/rfquack/rfquack
poetry install
cp examples/demos/bheu2022-arsenal/demoX-build.env .
make clean flash
make lsd      # take note of the serial port
make console  # check for any errors, just in case
```

```shell
poetry run rfq tty -P /dev/<your serial port>
```

You should see the radios, `q.radioA` and/or `q.radioB` in the RFQuack shell.

## DEMO 1: Finding and Decoding a Signal

**Goal:** show what RFQuack can achieve, without going into the details of what's under the hood, yet.

- signal generator node
  - this can be anyting (e.g., real device, SDR)
  - for this demo we'll use a [separate dongle](sig-gen/) based on RadioLib, running a TX loop that transmits a simple 2-FSK beacon
- on the RFQuack dongle
  - put RFQuack dongle in RX mode and sniff

## DEMO 2: Firmware Customization

**Goal:** show how to customize RFQuack firmware.

### DEMO 2.1: RF69 433MHz (`demo2.1-build.env`)

- walkthrough the `build.env` file and explain options
- configure a `build.env` file for
  - RF69
  - ESP32
- build, flash
- show that CLI finds `radioA`

```bash
rfq tty -P /dev/cu.usbserial-<your serial port>
```

```python
...

 Select a dongle typing: q.dongle(id)

 - Dongle 0: RFQUACK_UNIQ_ID = 'RFQUACK'

 > You have selected dongle 0: RFQUACK

RFQuack(/dev/cu.usbserial-016424A3)> q.radioA.help()

RFQuack(/dev/cu.usbserial-016424A3)> q.radioA.help()
Help for 'radioA':

> q.radioA.set_modem_config(rfquack_ModemConfig)
                carrierFreq
                txPower
                preambleLen
                syncWords
                isPromiscuous
                modulation
                useCRC
                bitRate
                rxBandwidth
                frequencyDeviation
        Apply configuration to modem.

> q.radioA.set_packet_len(rfquack_PacketLen)
                isFixedPacketLen
                packetLen
        Set packet length configuration (fixed/variable).

> q.radioA.set_register(rfquack_Register)
...
```

We now put the radio in RX mode and wait:

```python
RFQuack(/dev/cu.usbserial-016424A3)> q.radioA.set_modem_config(
                                              carrierFreq=434,
                                              txPower=10,
                                              modulation="FSK2",
                                              rxBandwidth=10.4,
                                              frequencyDeviation=5,
                                              bitRate=4.8)

result = 0
message = 6 changes applied and 0 failed.

RFQuack(/dev/cu.usbserial-016424A3)> q.radioA.rx()

data = b'OH OH OH CRWV55S3'
rxRadio = 0
millis = 1056862
repeat = 0
bitRate = 4.800000190734863
carrierFreq = 434.0
syncWords = b'\x12\xad'
modulation = FSK2
frequencyDeviation = 4.94384765625
RSSI = 0.0
model = RF69
hex data = 4f48204f48204f48204352575635355333

RFQuack(/dev/cu.usbserial-016424A3)> q.radioA.idle()
```

We received a packet from the signal generator.

### DEMO 2.2: Dual RF69 433MHz (`demo2.2-build.env`)

- configure a `build.env` file for
  - dual RF69
  - ESP32
- build, flash
- show that CLI finds `radioA` and `radioB`

```bash
rfq tty -P /dev/cu.usbserial-<your serial port>
```

```python
RFQuack(/dev/cu.usbserial-016424A3)> q.radioA
Out[5]: <rfquack.core.ModuleInterface at 0x104329990>

RFQuack(/dev/cu.usbserial-016424A3)> q.radioB
Out[6]: <rfquack.core.ModuleInterface at 0x10432a200>
```

### DEMO 2.3: Dual nRF24 2.4GHz and RF69 433MHz (`demo2.3-build.env`)

- configure a `build.env` file for
  - dual nRF24 and RF69
  - ESP32
- build, flash
- show that CLI finds `radioA` and `radioB`

```bash
rfq tty -P /dev/cu.usbserial-<your serial port>
```

```python
RFQuack(/dev/cu.usbserial-016424A3)> q.radioA
Out[5]: <rfquack.core.ModuleInterface at 0x104329990>

RFQuack(/dev/cu.usbserial-016424A3)> q.radioB
Out[6]: <rfquack.core.ModuleInterface at 0x10432a200>
```

### DEMO 2.4: Enabling Modules (`demo2.4-build.env`)

- configure a `build.env` file for
  - dual RF69
  - ESP32
  - Enable modules
- build, flash
- show that CLI finds `radioA` and `radioB`
- show that CLI finds modules

```bash
rfq tty -P /dev/cu.usbserial-<your serial port>
```

```python
RFQuack(/dev/cu.usbserial-016424A3)> q.packet_modification.help()
Help for 'packet_modification':

> q.packet_modification.enabled = ...
        Enable or disable this module.

> q.packet_modification.add(rfquack_PacketModification)
                position
                content
                operation
                operand
                pattern
                payload
        Adds a packet modification rule

> q.packet_modification.reset(rfquack_VoidValue)
        Removes all packet modification rules

> q.packet_modification.dump(rfquack_VoidValue)
        Dumps all packet modification rules

> q.packet_modification.auto_shift = ...
        Automatically left shifts ^5555 to get ^aaaa packets

Check src/rfquack.proto for type definitions
...
RFQuack(/dev/cu.usbserial-016424A3)> q.frequency_scanner
Out[8]: <rfquack.core.ModuleInterface at 0x1043288b0>

RFQuack(/dev/cu.usbserial-016424A3)> q.guessing
Out[9]: <rfquack.core.ModuleInterface at 0x104328580>

RFQuack(/dev/cu.usbserial-016424A3)> q.mouse_jack
Out[10]: <rfquack.core.ModuleInterface at 0x104328940>

RFQuack(/dev/cu.usbserial-016424A3)> q.packet_filter
Out[11]: <rfquack.core.ModuleInterface at 0x104329240>

RFQuack(/dev/cu.usbserial-016424A3)> q.packet_repeater
Out[12]: <rfquack.core.ModuleInterface at 0x104329720>

RFQuack(/dev/cu.usbserial-016424A3)> q.ping
Out[13]: <rfquack.core.ModuleInterface at 0x104329930>

RFQuack(/dev/cu.usbserial-016424A3)>
```

## DEMO 3: Scripting Attacks (`demo3-build.env`)

**Goal:** show how to script interactive attacks through the built-in packet filtering and modification modules.

- signal generator node
  - this can be anyting (e.g., real device, SDR)
  - for this demo we'll use a [separate dongle](sig-gen/) based on RadioLib, running a TX-wait-RX loop that transmits a simple 2-FSK beacon
- reveal the code of the signal generator to explain that we have an interactive RF protocol to attack (simplified on purpose)
- configure a packet filter that filters only those packets
- configure a packet modifier that modifies packets as requested by the "secret" comparator running inside the signal generator.

```bash
rfq tty -P /dev/cu.usbserial-<your serial port>
```

1. We'll use `radioB` to receive at 434MHz and `radioA` to retransmit (at 433MHz):

```python
FQuack(/dev/cu.usbserial-016424A3)> q.radioA.set_modem_config(
                                              carrierFreq=433,
                                              txPower=10,
                                              modulation="FSK2",
                                              rxBandwidth=10.4,
                                              frequencyDeviation=5,
                                              bitRate=4.8)

result = 0
message = 6 changes applied and 0 failed.

RFQuack(/dev/cu.usbserial-016424A3)> q.radioB.set_modem_config(
                                              carrierFreq=434,
                                              txPower=10,
                                              modulation="FSK2",
                                              rxBandwidth=15.6,
                                              frequencyDeviation=5,
                                              bitRate=4.8)

result = 0
message = 6 changes applied and 0 failed.

RFQuack(/dev/cu.usbserial-016424A3)> q.radioB.rx()

data = b'OH OH OH SJKS'
rxRadio = 1
millis = 1228048
repeat = 0
bitRate = 4.800000190734863
carrierFreq = 434.0
syncWords = b'\x12\xad'
modulation = FSK2
frequencyDeviation = 4.94384765625
RSSI = 0.0
model = RF69
hex data = 4f48204f48204f48204352575635355333
```

2. Now we know that the data being transmitted is `OH OH OH `; we also know, from our investigations, that the response must be the same as the data we received, with a "secret" modification:

```
tx_data[10] = rx_data[10] ^ 33
```

We can do this in the console as follows:

```python
RFQuack(/dev/cu.usbserial-016424A3)> q.data[-1].data[10]
Out[8]: 74

RFQuack(/dev/cu.usbserial-016424A3)> chr(q.data[-1].data[10])
Out[9]: 'J'

RFQuack(/dev/cu.usbserial-016424A3)> chr(q.data[-1].data[10] ^ 33)
Out[10]: 'k'

RFQuack(/dev/cu.usbserial-016424A3)> tx_data = bytearray(q.data[-1].data)

RFQuack(/dev/cu.usbserial-016424A3)> tx_data[10] = tx_data[10] ^ 33

RFQuack(/dev/cu.usbserial-016424A3)> tx_data = bytes(tx_data)

RFQuack(/dev/cu.usbserial-016424A3)> tx_data
Out[14]: b'OH OH OH SkKS'

RFQuack(/dev/cu.usbserial-016424A3)> q.radioA.tx()
result = 0
message =

RFQuack(/dev/cu.usbserial-016424A3)> q.radioA.send(data=tx_data, repeat=10)

result = 0
message =
```

And we notice that the signal generator blinks.

We now want to program the RFQuack dongle to do this operation automatically, without us having to do it on the client side.

3. We first configure the `packet_modification` module to modify every received packet according to the "secret" formula above

```python
RFQuack(/dev/cu.usbserial-016424A3)> q.packet_modification.add(position=10, operand=33, operation="XOR")

result = 0
message = Rule added, there are 1 modification rule(s).

RFQuack(/dev/cu.usbserial-016424A3)> q.packet_modification.dump()

position = 10
content = 0
operation = 3
operand = 33
pattern =
payload = b''
```

4. Now we configure the `packet_repeater` module to repeat (through `radioA`) every received packet, after the `packet_modification` module:

```python
RFQuack(/dev/cu.usbserial-016424A3)> q.packet_repeater.repeat = 5
RFQuack(/dev/cu.usbserial-016424A3)> q.packet_repeater.enabled = True
RFQuack(/dev/cu.usbserial-016424A3)> q.packet_modification.enabled = True
```

This means that every time the `packet_modification` module will receive a packet, it will modify it and then pass it to the `packet_repeater` module, which will repeat it (5 times) via `radioA`.

From this point on, whenever `radioB` will be in RX mode, the received packets will be processed as described above, with no intervention of the command line.
