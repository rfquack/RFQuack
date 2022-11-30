# Black Hat Europe 2022 (Arsenal)

This folder contains notes and the code used to demonstrate RFQuack at Black Hat Europe 2022 (Arsenal).

## DEMO 1: Finding and Decoding a Signal

**Goal:** show what RFQuack can achieve, without going into the details of what's under the hood, yet.

- signal generator node
  - this can be anyting (e.g., real device, SDR)
  - for this demo we'll use a [separate dongle](sig-gen/) based on RadioLib, running a TX-wait-RX loop that transmits a simple 2-FSK beacon
- on the RFQuack dongle
  - run frequency scanner module
  - run guessing module
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
