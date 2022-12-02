# A Mock IoT Node

This example mocks a RF IoT node that transmits FSK-encoded data over a 434MHz carrier using RadioLib's [default radio settings](https://github.com/jgromes/RadioLib/wiki/Default-configuration#overview).

## Hardware

- Board: Wemos D1 Mini Lite
- Radio: RF69HC ~433MHz

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
pio device monitor
```
