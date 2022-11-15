---
title: Quick Start
---

This guide is very...essential, we understand that. We'll improve it!

## With Hardware Ready

If you already have prepared your hardware, follow these steps. Otherwise, keep reading below!

```bash
git clone --recursive https://github.com/rfquack/RFQuack
cd RFQuack
pip install -r requirements.pip
vim build.env  # set your parameters and :wq
make clean build flash
```

## Prepare Your Hardware

1. Choose the radio chip and board that you want to use among the supported ones:
    - CC1101
    - RF69
    - nRF24
    - [read more](../hardware/radios.md)

2. Assemble the board and the radio chip together: if you choose the Adafruit Feather system, all you have to do is stack the boards together, and do some minor soldering. [Read more](../hardware/boards.md).
3. Connect the assembled board to the USB port.
