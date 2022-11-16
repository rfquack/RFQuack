It may happen that you don't know the frequency and/or the bitrate used by a transmitted. RFQuack comes with a module called `guessing` which automatically tries to, well, guess them!
The module comes already configured for scanning from `432MHz` up to `437MHz`, you can easily tweak its parameters using the CLI and use it on any carrier frequency supported by the radio module.

**Example:** start the module and it'll automagically determine the `carrierFreq` and/or `bitRate` of a transmission.

```python
RFQuack(/dev/ttyRFQ)> q.guessing.start()

result = 0
message = Started.

[... press any button on the keyfob ...]

data =  b'\x06\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa[...]'
rxRadio = 0
millis = 130090
bitRate = 3.3333332538604736
carrierFreq = 434.4758605957031
hex data = 06aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa666a95a96aaaa5[...]
```

**Note:** Currently it only supports OOK modulation, but we believe it can be extended to 2-FSK with some offset tuning (which will make a 2-FSK look like an OOK).
