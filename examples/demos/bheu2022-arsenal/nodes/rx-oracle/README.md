# A Mock RX IoT Node

This example mocks a RF IoT node that receives FSK-encoded data over a 434MHz carrier using RadioLib's [default radio settings](https://github.com/jgromes/RadioLib/wiki/Default-configuration#overview).

## Hardware

- Board: Feather ESP32
- Radio: RF69HW 434MHz (Radio A)

## Connections

- CS pin: 13
- DIO0 pin: 22
- RESET pin: 32

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
pio device monitor
```
