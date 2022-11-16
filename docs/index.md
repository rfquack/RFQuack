# Welcome to RFQuack

RFQuack is the only versatile RF-analysis tool that quacks! 🦆

## Quickstart

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

## Talks & Publications About RFQuack

If you use RFQuack and find it useful, we'd appreciate if you cite at least one of the following resources:

- **RFQuack - Cheap and easy RF analysis**, Andrea Guglielmini, [CanSecWest 2020](https://cansecwest.com/post/2020-03-09-22:00:00_2020_Speakers)
- **RFQuack: The RF-Analysis Tool That Quacks**, Federico Maggi, HITB Amory, Amsterdam, May 9, 2019 [[PDF](https://github.com/phretor/publications/raw/master/files/talks/maggi_rfquack_talk_2019.pdf)]
  - Radio and Hardware Security Testing for Human Beings, Federico Maggi, NoHat 2019, [[Video](https://www.youtube.com/watch?v=0m-Rjb5aWaM)]
  - Reverse engineering di protocolli radio proprietari, Federico Maggi, HackInBo® Winter Edition 2019, [[Video](https://www.youtube.com/watch?v=3r_9Za_Xboc)]

## Research Projects that used RFQuack

- **A Security Evaluation of Industrial Radio Remote Controllers**, Federico Maggi, Marco Balduzzi, Jonathan Andersson, Philippe Lin, Stephen Hilt, Akira Urano, and Rainer Vosseler. Proceedings of the 16th International Conference on Detection of Intrusions and Malware, and Vulnerability Assessment (DIMVA). Gothenburg, Sweden, June 19, 2019 [[PDF](https://github.com/phretor/publications/raw/master/files/papers/conference-papers/maggi_industrialradios_2019.pdf)]

- **A Security Analysis of Radio Remote Controllers for Industrial Applications**,
Jonathan Andersson, Marco Balduzzi, Stephen Hilt, Philippe Lin, Federico Maggi, Akira Urano, and Rainer Vosseler., Trend Micro, Inc. Trend Micro Research, January 15, 2019 [[PDF](https://documents.trendmicro.com/assets/white_papers/wp-a-security-analysis-of-radio-remote-controllers.pdf)]
  - Attacking Industrial Remote Controllers, Marco Balduzzi and Federico Maggi, HITB2019, Amsterdam [[Video](https://www.youtube.com/watch?v=pEP7EOQkm_0)]
  - How we reverse-engineered multiple industrial radio remote-control systems, Stephen Hilt, BSides Knoxville 2020, [[Video](https://www.youtube.com/watch?v=xBXktWwvEyI)]
  - Attacking industrial remote controllers for fun and profit, Dr. Marco Balduzzi, CONFidence 2019, [[Video](https://www.youtube.com/watch?v=T6sJCUxFohc)]
  - How we reverse-engineered multiple industrial radio remote-control systems, Stephen Hilt, CS3STHLM 2019, [[Video](https://www.youtube.com/watch?v=5l_cWD5ZR-M)]
- [EvilCrowRF](https://github.com/joelsernamoreno/EvilCrowRF-Beta)

## Development Status and Maturity

RFQuack is quite experimental, expect glitches and imperfections. So far we're quite happy with it, and used it successfully to analyze some industrial radio protocols (read the [Trend Micro Research white paper](https://www.trendmicro.com/vinfo/us/security/news/vulnerabilities-and-exploits/attacks-against-industrial-machines-via-vulnerable-radio-remote-controllers-security-analysis-and-recommendations) or the [DIMVA 2019 paper](https://www.dimva2019.org) for details).

## Disclaimer

RFQuack is a research tool intended to analyze and emit radio-frequency (RF) signals via software, with native hardware support. Although it is not intended for illegal, malicious or offensive purposes, it can be used to those ends. We take no responsibility whatsoever about the unforeseen consequences of unethical or illegal use of this software.
