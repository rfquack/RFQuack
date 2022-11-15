# Welcome to RFQuack

RFQuack is the only versatile RF-analysis tool that quacks! 🦆

## Here, Flash Your Dongle!

```bash
git clone --recursive https://github.com/rfquack/RFQuack
cd RFQuack
pip install -r requirements.pip
vim build.env  # set your parameters and :wq
make clean build flash
```

## OK, but What Does it Even Mean?

It's a library firmware that allows you to sniff, manipulate, and transmit data over the air. And if you're not happy how the default firmware functionalities or you want to **change the hardware**, we made it **easy to extend**. Consider it as the hardware-agnostic and developer-friendly version of the great [YardStick One](https://greatscottgadgets.com/yardstickone/), which is based on the CC1101 radio chip. Differently from the other RF dongles, RFQuack is designed to be agnostic with respect to the radio chip. So if you want to use, say, the RF69, you can do it. If you need to use the CC1101L or CC1120, you can do it. Similarly to RFCat, RFQuack has a [console-based, Python-scriptable client](https://github.com/rfquack/RFQuack-cli) that allows you to set parameters, receive, transmit, and so on.

## Another RF-analysis Dongle?

Not really. RFQuack is midway between software-defined radios (SDRs), which offer great
flexibility at the price of a fatter code base, and RF dongles, which offer
great speed and a plug-and-play experience at the price of less flexibility
p(you can't change the radio module).

RFQuack is unique in these ways:

- It's a **library** firmware, with many settings, sane defaults, and rich logging and debugging functionalities.
- Supports **multiple radio chips**: nRF24, CC1101, basically all the chips supported by [RadioLib](https://github.com/jgromes/RadioLib), and we're adding more.
- Does not require a **wired connection** to the host computer: the serial port is used only to display debugging messages, but the interaction between the client and the node is over TCP using WiFi (via Arduino WiFi) and GPRS (via [TinyGSM](https://github.com/vshymanskyy/TinyGSM) library) as physical layers.
- The [RFQuack client](https://github.com/rfquack/RFQuack-cli) allows both **high- and low-level operations**: change frequency, change modulation, etc., as well as to interact with the radio chip via registers.
- The firmware and its API support the concept of **packet-filtering** and **packet-modification rules**, which means that you can instruct the firmware to listen for a packet matching a given signature (in addition to the usual sync-word- and address-based filtering, which normally happen in the radio hardware), optionally modify it right away, and re-transmit it.

So, if you need to analyze a weird RF protocol with that special packet format or that very special modulation scheme, with mixed symbol encodings (yes, I'm looking at you, CC1120 in 4-FSK mode 🤬), with RFQuack you just swap the radio shield and you can just start working right away. And if we don't support that special radio chip, you can just craft your shield and add support to the software!

## Development Status and Maturity

RFQuack is quite experimental, expect glitches and imperfections. So far we're quite happy with it, and used it successfully to analyze some industrial radio protocols (read the [Trend Micro Research white paper](https://www.trendmicro.com/vinfo/us/security/news/vulnerabilities-and-exploits/attacks-against-industrial-machines-via-vulnerable-radio-remote-controllers-security-analysis-and-recommendations) or the [DIMVA 2019 paper](https://www.dimva2019.org) for details).