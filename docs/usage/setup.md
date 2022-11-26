## Prepare Your Hardware

1. Choose the radio chip and board that you want to use among the supported ones:
    - CC1101
    - RF69
    - nRF24
    - [read more](../hardware/radios.md)

2. Assemble the board and the radio chip together: if you choose the Adafruit Feather system, all you have to do is stack the boards together, and do some minor soldering. [Read more](../hardware/boards.md).
3. Connect the assembled board to the USB port.

## Firmware Configuration

RFQuack's build system is parametric, so that you can customize some functionality without touching the code. This means that the build system "compiles" a template `main.cpp` based on some variables, before running the actual compilation step (from C++ to binary executable).

The build process is baed on PlatformIO and will read variables from two files:

- [platformio.ini](https://github.com/rfquack/RFQuack/blob/master/platformio.ini): board and platform definition.
    - **Default:** `featheresp32` (ESP32)
- [build.env](https://github.com/rfquack/RFQuack/blob/master/build.env): type of radio(s) and connections.
    - **Default:** 3 radios (CC1101, nRF24, RF69) connected "randomly" (likely you'll need to change this).

### General Configuration

| Variable           | Description                                                                 | Required           |
| ------------------ | --------------------------------------------------------------------------- | ------------------ |
| `RFQUACK_UNIQ_ID`  | Unique identifier for this node (defaults to `RFQUACK`)                     | No                 |
| `SERIAL_BAUD_RATE` | Defaults to `115200`                                                        | No                 |
| `USE_MQTT`         | Disables Serial transport and enables the MQTT one                          | No                 |
| `WIFI_SSID`        | WiFi SSID                                                                   | Yes, if `USE_MQTT` |
| `WIFI_PASS`        | WiFi Password                                                               | Yes, if `USE_MQTT` |
| `MQTT_HOST`        | MQTT Broker host                                                            | Yes, if `USE_MQTT` |
| `MQTT_PORT`        | MQTT Broker port (defaults to `1883`)                                       | No                 |
| `MQTT_USER`        | MQTT Broker username                                                        | No                 |
| `MQTT_PASS`        | MQTT Broker password                                                        | No                 |
| `MQTT_SSL`         | Enables MQTT over SSL (put your certificates into `rfquack_certificates.h`) | No                 |

### Radio Configuration

RFQuack supports up to 5 radios, up to what your board supports (i.e., enough interrupt and chip select pins). You must configure, at least, `RadioA`:

| Variable       | Description                                                                                   | Required                           |
| -------------- | --------------------------------------------------------------------------------------------- | ---------------------------------- |
| `RADIOA`       | Chosen modem for `RadioA`: (options: `rF69`, `CC1101`, `nRF24` *case sensitive*)              | Yes                                |
| `RADIOA_CS`    | Chip select pin for `RadioA`                                                                  | Yes                                |
| `RADIOA_IRQ`   | Interrupt pin for `RadioA`. It's labeled `IRQ` on `nRF24` modules, or `GDO0` on `CC1101` ones | No                                 |
| `RADIOA_RST`   | Reset pin for `RadioA` (called chip enable pin in `nRF24` )                                   | `nRF24` only (optional for others) |
| `RADIO<X>`     | Chosen module for `RadioA`: (options: `RF69`, `CC1101`, `nRF24`)                              | No                                 |
| `RADIO<X>_CS`  | Chip Select pin for RadioX                                                                    | No                                 |
| `RADIO<X>_IRQ` | Interrupt pin for `RadioX` (e.g., labeled `IRQ` on `nRF24` modules, `GDO0` on `CC1101`)       | No                                 |
| `RADIO<X>_RST` | Reset pin for `RadioX` (needed only for `nRF24` radios)                                       | No                                 |

Valid values of `<X>` are `B`, `C`, `D`, `E`.

## Modules Configuration

RFQuack comes with some built-in modules, which are **not** enabled by default to keep the firmware lightweight. If you want to enable them, define the following variables into `build.env`:

- `GUESSING_MODULE`
- `FREQ_SCANNER_MODULE`
- `MOUSE_JACK_MODULE`
- `PACKET_FILTER_MODULE`
- `PACKET_MOD_MODULE`
- `PACKET_REPEAT_MODULE`
- `ROLL_JAM_MODULE`

For more information, check out the [Modules](../modules/overview.md) section.

## Example Configurations

### ESP32 + CC1101

This is the declaration of an RFQuack dongle based on an `featheresp32` board and 1 CC1101 radio.

#### General Configuration

This is the declaration of a `featheresp32`-based board in PlatformIO:

``` INI title="platformio.ini"
; ...you can ignore what's above this...

[env:featheresp32]
extends = env:espressif32
board = featheresp32

```

Now let's combine it with a CC1101 radio.

#### Radio Configuration

``` INI title="build.env"
RADIOA=CC1101
RADIOA_CS=2
RADIOA_IRQ=5
```

- Chip select (CS) to pin 2, which means that your radio's CS is connect to pin 2 on the `featheresp32` [^featheresp32_pinout].
- Interrupt (IRQ) to pin 5, which means that your radio's default [^default_gdo] digital I/O pin is connected to pin 2 on the `featheresp32`.

[^featheresp32_pinout]: See the [Adafruit Feather ESP32 pinout definition](https://learn.adafruit.com/adafruit-huzzah32-esp32-feather/pinouts).
[^default_gdo]: See **Figure 9** and **Table 19** of [TI CC1101 datasheet](../datasheets/CC1101/CC1101.pdf) for more details.

### More Examples

We'll add more examples in this section. Meanwhile, please check the [examples/](https://github.com/rfquack/RFQuack/tree/master/examples) folder for inspiration.
