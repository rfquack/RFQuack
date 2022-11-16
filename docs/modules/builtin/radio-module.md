Each connected radio will pop up as a module, progressively named after `radioA`, `radioB`, `radioC`, `radioD`, `radioE`.

RFQuack's radio sub-system is based on [RadioLib](https://github.com/jgromes/RadioLib), so for most aspects you can refer to the RadioLib documentation. (yep, even for error codes).

## Modem Configuration

Not all radio modules support modem configuration. Sub-gigahertz modems usually do. The `q.radioA.set_modem_config()` function takes as argument a `rfquack_ModemConfig`, which is built of the following optional parameters:

- `carrierFreq`: this is the carrier frequency, easy; make sure you comply to the radio module you chosen.
- `txPower`: control the transmission power; set them wisely and make sure to follow the laws that apply to your country.
- `preambleLen`: control the length of the radio's preamble.
- `syncWords`: sync-word matching is a basic functionality of most packet-radio modules, which allow to efficiently filter packets that match the sync words and just ignore the rest, in order to keep the radio chip and the MCU busy only when an expected packet is received; depending on the radio module, the sync words can be set to zero (promiscuous mode) or up to a certain number of octects (e.g., 4); in promiscuous mode, the radio and MCU will be *very* busy, because they will pick up *everything*, including noise.
- `isPromiscuous`: handy way to automatically set neat parameters and enter a fully *promiscuous* mode: sets syncword, disable crc filtering, disables automatic acknowledges, ...
- `modulation`: this is the carrier modulation (ASK, OOK, FSK, GSK ...); make sure you comply to the radio module you chosen.
- `useCRC`: whatever to enable or disable CRC filtering.
- `bitRate`: this is the symbol bitrate (in kbps); make sure you comply to the radio module you chosen.
- `rxBandwidth`: Sets receiver bandwidth (in kHz); make sure you comply to the radio module you chosen.

Usage example (on a `CC1101` radio):

```python
RFQuack(/dev/ttyUSB0, 115200,8,N,1)> \
  q.radioA.set_modem_config(modulation="OOK",
                            carrierFreq=434.437,
                            bitRate=3.41296,
                            useCRC=False,
                            syncWords=b"\x99\x9A",
                            rxBandwidth=58)
result = 0
message = 6 changes applied and 0 failed.
```

It's not over 😛

Usually, radios receive and transmit *packets*. You can set the radio to expect a *fixed length* packet or, if it's supported, you can ask the radio to look for the packet length in the payload itself. All of this can be done using the `set_packet_len` function.

Usage example (on a `CC1101` radio):

```python
RFQuack(/dev/ttyUSB0, 115200,8,N,1)> \
  q.radioA.set_packet_len(
    isFixedPacketLen=True,
    packetLen=102) # Sets len to 102 bytes.
result = 0
message =
```

## Transmit and Receive

The `tx()`, `rx()`, `idle()` functions are self-explanatory: they set the module in transmit, receive and idle mode, respectively. To actually transmit data, you can use `send(data=b"\xAA\xBB")`, where data must be a list of raw octect values; there's a limit in the length, which is imposed by the radio module, so make sure you check the documentation.

```python
RFQuack(/dev/ttyUSB0, 115200,8,N,1)> \
  q.radioA.tx() # Enters TX mode.

result = 0
message =

RFQuack(/dev/ttyUSB0, 115200,8,N,1)> \
  q.radioA.send(data=bytes.fromhex("555555d42d"))

result = 0
message =
```

By default, a packet is transmitted only once. If you want to repeat it, just set `repetitions` to whatever you want, and RFQuack will repeat the transmission as fast as possible (bound by the MCU clock, of course).

## Register Access

While RadioLib has gone very far in abstracting the interaction with the radio,

Some radio chips are really "unique," so to speak. In these cases, the only option is to grab a large cup of your favorite beverage, read through the datasheet, read again, again, and again.

Once you understand enough of how the radio works at the low level, you want to get-set registers in order to use it. In principle, you can do pretty much everything via registers.

RFQuack is meant to be as generic as possible. What's not directly abstracted within a module can be accomplished by setting the registers via the `set_register` and `get_register` function.

Usage example: retrieve the content of register `0x02`

```python
RFQuack(/dev/ttyUSB0, 115200,8,N,1)> q.radioA.get_register(int("0x02",16))  
address = 2
value = 3
0x02 = 0b00000011 (0x03, 3)
```

Or alter it:

```python
RFQuack(/dev/ttyUSB0, 115200,8,N,1)> q.radioA.set_register(address=int("0x02",16), value=int("0xFF",16))  
result = 0
message =
```

Recall that Python lets you do nice things like `q.radioA.set_register(address=int("0x02", 16), value=0x01001100)` so you don't have to do any conversions.

Note that every call to `set_modem_config()` will **alter the modem state, including several registers** to their default values (according to the datasheet). Also, many radio chips need to be in an "idle" state while setting certain registers. Please check the datasheet and use `idle()` before setting registers to be on the safe side. Last, be wise and double check that the values you set are actually there, using `get_register` after each `set_register`.

We noticed some timing issues with some radio chips. So, allow a small delay if you're setting many registers in a row (e.g., `for addr, value in regs: q.radioA.set_register(address=addr, value=value); time.sleep(0.2)`).