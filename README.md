# RFQuack

<a name="readme-top"></a>

![Build Status](https://github.com/rfquack/RFQuack/actions/workflows/build.yml/badge.svg)

[![Contributors][contributors-shield]][contributors-url]
[![Forks][forks-shield]][forks-url]
[![Stargazers][stars-shield]][stars-url]
[![Issues][issues-shield]][issues-url]
[![GPL 2 License][license-shield]][license-url]


<!-- PROJECT LOGO -->
<br />
<div align="center">
  <a href="https://github.com/rfquack/RFQuack">
    <img src="docs/imgs/logo-round-nobg.png" alt="RFQuack Logo" />
  </a>

  <h3 align="center">RFQuack</h3>

  <p align="center">
    The only RF-analysis tool that quacks!
    <br />
    <br />
    <a href="https://rfquack.org"><strong>Documentation</strong></a>
    |
    <a href="https://arxiv.org/abs/2104.02551"><strong>Research Paper</strong></a>
    <br />
    <br />
    <a href="https://www.youtube.com/playlist?list=PL8hbvIylvVegA6ES-UUfd6sgK-MFNciz5">View Demo</a>
    ·
    <a href="https://github.com/rfquack/RFQuack/issues">Report Bug</a>
    ·
    <a href="https://github.com/rfquack/RFQuack/issues">Request Feature</a>
  </p>
</div>




<!-- ABOUT THE PROJECT -->
<p align="center">
<img src="https://img.youtube.com/vi/_59SRwfS6PU/0.jpg" />
</p>

## About RFQuack

RFQuack is a versatile RF-analysis tool that allows you to sniff, analyze, and transmit data over the air.

Similarly to RFCat RFQuack has a Python-based scriptable shell that allows you to set parameters, receive, transmit, and so on.

<p align="right">(<a href="#readme-top">back to top</a>)</p>

## Supported Radios

We porting from (and contribute back to) [RadioLib](https://github.com/jgromes/RadioLib). So far, we support:

* __CC1101__ OOK, 2-FSK, 4-FSK, MSK radio module
* __nRF24L01__ 2.4 GHz module
* __RF69__ FSK, OOK radio module

## Supported Arduino Platforms

In principle, RFQuack can run on any board and platform supported by [PlatformIO](https://docs.platformio.org). So far, we tested the following boards:

* [**ESP32**](https://github.com/espressif/rduino-esp32) - ESP32-based boards
* [**Teensy**](https://github.com/PaulStoffregen/cores) - Teensy 2.x, 3.x and 4.x boards

<!-- GETTING STARTED -->
## Getting Started

This is an example of how you may give instructions on setting up RFQuack.

### Prerequisites

You'll need the Protbuf Compiler, a sane Python 3.10, and PlatformIO (which itself requires some dependencies):

- Protobuf Compiler
- Python 3.10.x
- PlatformIO

For more details, please refer to the [Documentation](https://rfquack.org).

<p align="right">(<a href="#readme-top">back to top</a>)</p>

<!-- INSTALLATION -->
## Installation

An easy way to quick start is to have an ESP32 board and a CC1101 and/or RF69 (easier to find) radio module.

```bash
git clone --recursive https://github.com/rfquack/RFQuack
cd RFQuack
pip install -r requirements.pip
vim build.env  # set your parameters and :wq
make clean build flash
```

For more details, please refer to the [Documentation](https://rfquack.org).

<p align="right">(<a href="#readme-top">back to top</a>)</p>


<!-- USAGE -->
## Usage

An easy way to quick start is to connect the dongle via USB and use the CLI.

```shell
$ rfq tty -P /dev/ttyUSB0
2019-04-10 18:04:31 local RFQuack[20877] INFO Transport initialized
2019-04-10 18:04:31 local RFQuack[20877] INFO Transport initialized (QoS = 2): mid = 2

...

RFQuack(/dev/ttyUSB0, 115200,8,N,1)> q.radioA.set_modem_config(modulation="OOK", carrierFreq=434.437)

result = 0
message = 2 changes applied and 0 failed.

RFQuack(/dev/ttyUSB0, 115200,8,N,1)> q.radioA.rx()

result = 0
message =
...
```

For more details, please refer to the [Documentation](https://rfquack.org).


<!-- ROADMAP -->
## Roadmap

- [ ] Add all relevant [RadioLib](https://github.com/jgromes/RadioLib) modules
- [ ] Test with more than 2 radio modules
- [ ] Revisit Python CLI source code and use typed Python 3
- [ ] Integrate with URH and GNU Radio
- [ ] Make a web UI

See the [open issues](https://github.com/rfquack/RFQuack/issues) for a full list of proposed features (and known issues).

<p align="right">(<a href="#readme-top">back to top</a>)</p>



<!-- CONTRIBUTING -->
## Contributing

Contributions are what make the open source community such an amazing place to learn, inspire, and create. Any contributions you make are **greatly appreciated**.

If you have a suggestion that would make this better, please fork the repo and create a pull request. You can also simply open an issue with the proper template.

Don't forget to give the project a star! Thanks again!

1. Fork the project
2. Create your feature branch (`git checkout -b feature/AmazingFeature`)
3. Commit your changes (`git commit -m 'Add some AmazingFeature'`)
4. Run simple integration tests (`make clean build`)
4. Push to the branch (`git push origin feature/AmazingFeature`)
5. Open a Pull Request

<p align="right">(<a href="#readme-top">back to top</a>)</p>



<!-- LICENSE -->
## License

Distributed under the GPL 2 License. See `LICENSE` for more information.

<p align="right">(<a href="#readme-top">back to top</a>)</p>

## Acknowledgments

RFQuack wouldn't exist without the inspiration, feedback, and help received from related tools and awesome humans:

* [RFCat](https://github.com/atlas0fd00m/rfcat)
* [RadioLib](https://github.com/jgromes/RadioLib)
* [RadioHead](https://www.airspayce.com/mikem/arduino/RadioHead/)
* [URH](https://github.com/jopohl/urh)

<p align="right">(<a href="#readme-top">back to top</a>)</p>

<!-- MARKDOWN LINKS & IMAGES -->
<!-- https://www.markdownguide.org/basic-syntax/#reference-style-links -->
[contributors-shield]: https://img.shields.io/github/contributors/rfquack/RFQuack.svg?style=for-the-badge
[contributors-url]: https://github.com/rfquack/RFQuack/graphs/contributors
[forks-shield]: https://img.shields.io/github/forks/rfquack/RFQuack.svg?style=for-the-badge
[forks-url]: https://github.com/rfquack/RFQuack/network/members
[stars-shield]: https://img.shields.io/github/stars/rfquack/RFQuack.svg?style=for-the-badge
[stars-url]: https://github.com/rfquack/RFQuack/stargazers
[issues-shield]: https://img.shields.io/github/issues/rfquack/RFQuack.svg?style=for-the-badge
[issues-url]: https://github.com/rfquack/RFQuack/issues
[license-shield]: https://img.shields.io/github/license/rfquack/RFQuack.svg?style=for-the-badge
[license-url]: https://github.com/rfquack/RFQuack/blob/master/LICENSE