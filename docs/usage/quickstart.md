
RFQuack is quite experimental, expect glitches and imperfections. So far we're quite happy with it, and used it successfully to analyze some industrial radio protocols (read the [Trend Micro Research white paper](https://www.trendmicro.com/vinfo/us/security/news/vulnerabilities-and-exploits/attacks-against-industrial-machines-via-vulnerable-radio-remote-controllers-security-analysis-and-recommendations) or the [DIMVA 2019 paper](https://www.dimva2019.org) for details).

### Prepare Your Hardware

- Choose the radio chip and board that you want to use among the supported ones: we tested the CC1101, nRF24 and ESP32-based boards (namely the Adafruit Feather HUZZAH32).
- Assemble the board and the radio chip together: if you choose the Adafruit Feather system, all you have to do is stack the boards together, and do some minor soldering.
- Connect the board to the USB port.

| **Main board** | **Radio daughter board** | **Network connectivity** | **Cellular connectivity** |
|----------------|-------------------------------------|----------------------|-----------------------|
| Feather HUZZAH32 | Feather nRF24 | ESPWiFi | Not tested |
| Feather HUZZAH32 | Feather CC1101 | ESPWiFi | Not tested |
| Feather FONA | Radio FeatherWing RFM69HW 433 MHz | None | Tested with early versions of RFQuack |

You could play around with other combinations, of course. And if you feel generous, you can fork this repository, add support for untested hardware, and send us a pull request (including schematics for new daughter-boards)! 👏
