# RFQuack Command Line Interface Client

Command line interface client to [RFQuack](https://github.com/trendmicro/RFQuack) dongles.

- [RFQuack Command Line Interface Client](#rfquack-command-line-interface-client)
  - [Installation](#installation)
    - [Docker](#docker)
      - [Build Docker image](#build-docker-image)
    - [Manual Installation](#manual-installation)
  - [Basic Usage](#basic-usage)
  - [Example](#example)
  - [License](#license)
  - [Disclaimer](#disclaimer)

## Installation

RFQuack-cli can be used within a Docker container or installed from source.

### Docker

For example, to connect to a WiFi dongle:

```bash
$ docker run --rm -it rfquack/cli mqtt -H <mqttBroker> -P 1884
...
```

or any USB connected dongle:

```bash
$ docker run --device /dev/ttyUSB0 --user=root --rm -it \
  rfquack/cli tty -P /dev/ttyUSB0
...
```

#### Build Docker image
```bash
$ make docker-build
```

### Manual Installation

```bash
$ git clone https://github.com/rfquack/RFQuack-cli
cd RFQuack-cli
$ pipenv run pip install -e .
```

## Basic Usage

```bash
$ pipenv shell  # use pipenv shell or activate whatever virtual env system you're using
$ rfquack --help
Usage: rfq.py [OPTIONS] COMMAND [ARGS]...

Options:
  -l, --loglevel [CRITICAL|ERROR|WARNING|INFO|DEBUG|NOTSET]
  --help                          Show this message and exit.

Commands:
  mqtt  RFQuack client with MQTT transport.
  tty   RFQuack client with serial transport.

$ rfquack mqtt --help
Usage: rfq.py mqtt [OPTIONS]

  RFQuack client with MQTT transport. Assumes one dongle per MQTT broker.

Options:
  -i, --client_id TEXT
  -H, --host TEXT
  -P, --port INTEGER
  -u, --username TEXT
  -p, --password TEXT
  --help                Show this message and exit.

$ rfquack tty --help
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

## Example

```bash
$ docker run --device /dev/ttyUSB0 --user=root --rm -it rfquack/cli tty -P /dev/ttyUSB0
...


In [1]:  q.radioA.set_modem_config(modulation="OOK", carrierFreq=434.437, useCRC=False)
2020-02-16 23:45:59 thinkpad rfquack.transport[31490] DEBUG b'rfquack/in/set/radioA/rfquack_ModemConfig/set_modem_config' (21 bytes)
2020-02-16 23:45:59 thinkpad rfquack.transport[31490] DEBUG Message from module "b'radioA'"

result = 0
message = 3 changes applied and 0 failed.
```

```bash
$ rfquack mqtt -H localhost -P 1884
2019-04-10 18:04:31 local RFQuack[20877] INFO Transport initialized
2019-04-10 18:04:31 local RFQuack[20877] DEBUG Setting mode to IDLE
2019-04-10 18:04:31 local RFQuack[20877] DEBUG rfquack/in/set/status (2 bytes)
2019-04-10 18:04:31 local RFQuack[20877] INFO Transport pipe initialized (QoS = 2): mid = 2

...

In [1]: q.radioA.rx()
2020-02-16 23:47:06 thinkpad rfquack.transport[31490] DEBUG b'rfquack/in/set/radioA/rfquack_VoidValue/rx' (0 bytes)
2020-02-16 23:47:06 thinkpad rfquack.transport[31490] DEBUG Writing packet = b'>rfquack/in/set/radioA/rfquack_VoidValue/rx~\x00'

2020-02-16 23:47:06 thinkpad rfquack.transport[31490] DEBUG 2 bytes received on topic: "b'rfquack/out/set/radioA/rfquack_CmdReply/rx'" = "b'0800'"
2020-02-16 23:47:06 thinkpad rfquack.transport[31490] DEBUG Message from module "b'radioA'"

result = 0
message =
```

At this point you're good to go from here!

## License

Copyright (C) 2019 Trend Micro Incorporated.

This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program; if not, write to the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

## Disclaimer

RFQuack is a research tool intended to analyze radio-frequency (RF) signals via
software, with native hardware support. It is not intended for malicious or
offensive purposes.
