# A Mock IoT Node

This example mocks a RF IoT node that:

- generates a random string at boot
- transmits FSK-encoded data over a 434MHz carrier using RadioLib
- enters RX mode at 433MHz and waits for valid frames
- compares the random string against each received frame
- if the "secret comparison" (check the [`src/main.cpp`](src/main.cpp)) is correct, the builtin blue LED will blink 5 times: this simulates a simple challenge-response mechanism

## Hardware

- Board: Wemos D1 Mini Lite
- Radio: RF69HC tuned at 433MHz

## Connections

- CS pin: GPIO 15 (D8)
- DIO0 pin: GPIO 5 (D1)
- RESET pin: GPIO 4 (D2)

## Requirements

- Python >= 3.10
- Pip

It's highly recommended to use [Pyenv](https://github.com/pyenv/pyenv) as a good practice to manage your Python versions, and [venv](https://docs.python.org/3/tutorial/venv.html) (now part of Python 3) to manage per-project Python environments.

```bash
pip install -r requirements.pip
```

This will install PlatformIO and its dependencies.

## Build

To build the firmware:

```bash
pio run
```

## Flash

To flash your board:

```bash
pio run -t upload
```

## Check the Console

```bash
$ pio device monitor -p /dev/<your serial port>
---------------------------------- TX @ 434MHz
TX success!
        Data: OH OH OH CKDXOSF2
---------------------------------- RX @ 433MHz
RX timeout!
---------------------------------- TX @ 434MHz
TX success!
        Data: OH OH OH CKDXOSF2
---------------------------------- RX @ 433MHz
RX timeout!
---------------------------------- TX @ 434MHz
TX success!
        Data: OH OH OH CKDXOSF2
---------------------------------- RX @ 433MHz
RX timeout!
---------------------------------- TX @ 434MHz
TX success!
        Data: OH OH OH CKDXOSF2
---------------------------------- RX @ 433MHz
RX timeout!
...
```

It'll go on like this until responses are received:

```bash
...
---------------------------------- RX @ 433MHz
success!
RX data:        OH OH OH CKDXOSF2
         RSSI: -57.00 dBm
---------------------------------- TX @ 434MHz
TX success!
        Data: OH OH OH CKDXOSF2
---------------------------------- RX @ 433MHz
RX timeout!
---------------------------------- TX @ 434MHz
TX success!
        Data: OH OH OH CKDXOSF2
---------------------------------- RX @ 433MHz
success!
RX data:        OH OH OH CKDXOSF2
         RSSI: -57.00 dBm
---------------------------------- TX @ 434MHz
TX success!
        Data: OH OH OH CKDXOSF2
---------------------------------- RX @ 433MHz
RX timeout!
---------------------------------- TX @ 434MHz
...
```

Here we see that the random string is `OH OH OH OIS5ZH45` but we don't see the LED blinking, so it's not been modified as expected by the secret comparator.

```bash
...
---------------------------------- TX @ 434MHz
TX success!
        Data: OH OH OH CKDXOSF2
---------------------------------- RX @ 433MHz
success!
RX data:        OH OH OH C␁DXOSF2
🔵 BLINK 🔵 BLINK 🔵 BLINK 🔵 BLINK 🔵 BLINK 
         RSSI: -57.00 dBm
---------------------------------- TX @ 434MHz
TX success!
        Data: OH OH OH CKDXOSF2
---------------------------------- RX @ 433MHz
...
```
