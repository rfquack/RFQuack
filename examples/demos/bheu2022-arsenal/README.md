# Black Hat Europe 2022 (Arsenal)

This folder contains notes and the code used to demonstrate RFQuack at Black Hat Europe 2022 (Arsenal).

## RFQuack Dongles

We'll use the following RFQuack dongles. Check the `build.env` file in each [dongles/](dongles/) subfolder, change the pin numbers according to your wiring, and copy it in the root of the RFQuack repository and you'll be ready to go.

### Dongle 1: ESP32, dual RF69

- Feather ESP32
- Radio A: RFM69HW @ ~433MHz
- Radio B: RFM69HW @ ~433MHz

### Dongle 2: ESP32, CC11-1

- ESP32 Dev Kit V1
- Radio A: CC1101 @ ~433MHz

### Building the RFQuack Firmware

```shell
git clone https://github.com/rfquack/rfquack
poetry install
cp examples/demos/bheu2022-arsenal/build.env .
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

- walkthrough the `build.env` file and explain options
- configure a `build.env` file for
  - RF69
  - ESP32
- build, flash
- show that CLI finds `radioA`
- configure a `build.env` file for
  - dual RF69
  - ESP32
- build, flash
- show that CLI finds `radioA` and `radioB`

## DEMO 3: Scripting Attacks

**Goal:** show how to script interactive attacks through the built-in packet filtering and modification modules.

- signal generator node
  - this can be anyting (e.g., real device, SDR)
  - for this demo we'll use a [separate dongle](sig-gen/) based on RadioLib, running a TX-wait-RX loop that transmits a simple 2-FSK beacon
- reveal the code of the signal generator to explain that we have an interactive RF protocol to attack (simplified on purpose)
- configure a packet filter that filters only those packets
- configure a packet modifier that modifies packets as requested by the "secret" loop running inside the signal generator.
