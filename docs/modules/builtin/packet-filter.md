One of the main reasons why we created RFQuack is that we wanted to automate certain tasks in a flexible and fast way. For instance, we were building a PoC for a vulnerability in a radio protocol that, with a change in two bytes of the payload, the vulnerable receiver would execute another command. So, all we had to do was: stay in RX mode, wait for a packet matching a pattern, alter it, and re-transmit it.

Most of this could be done with an SDR or with a RF-dongle and RFCat, but in both cases you'd have to "pay" the round-trip time from the radio, to the client, and back. For certain protocols, this timing is not acceptable. RFQuack's firmware implements this functionality natively, and exposes a simple API to configure packet filtering and manipulation.

**Important:** filtering and patterns are applied past any filtering performed by the radio (e.g., based on sync words, address, CRC, RSSI, LQI). If you want to consider any packet, including noise, you'll have to disable these low-level filters enabling *promiscuous mode*)

- `q.packet_filter.add(pattern="", negateRule=bool)` takes two parameters: a regular-expression pattern complying with the [tiny-regex-c](https://github.com/kokke/tiny-regex-c) library (most common patterns are supported); adding a pattern means that RFQuack will discard any payload not matching that regex (or matching it, using `negateRule`); you can add multiple filters, they'll be applied one next the other (AND logic).
- `q.packet_filter.reset()` will delete any stored filtering rule.
- `q.packet_filter.dump()` will dump to CLI any stored rule.
- `q.packet_filter.enabled` boolean that controls whatever the module is enabled, **do not forget to set it!**

**NOTE** Packet's payload will be treated as a hex string.

Example:

```python
RFQuack(/dev/ttyDUMMY, 115200,8,N,1)> \
  q.packet_filter.add(
      # Accept only packets starting this way.
      pattern="^aaaaaaaaaa999aa56a",
      negateRule=False
      )
result = 0
message = Rule added, there is 1 filtering rule.

RFQuack(/dev/ttyDUMMY, 115200,8,N,1)> \
  # Do not forget to enable the module!
  q.packet_filter.enabled = True 
result = 0
message =
```