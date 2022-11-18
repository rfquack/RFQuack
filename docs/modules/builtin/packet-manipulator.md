RFQuacks comes with a powerful packet modification module:

- `q.packet_modification.add()` takes several parameters:
  - `position` (number, optional) indicates the position in the payload that will be modified (e.g., 3rd byte);
  - `content` (byte, optional) indicates the content that will be modified (e.g., all octects which value is `'A'`);
  - `pattern` (optional) same as for the filter: only packets matching the pattern will be modified; if no pattern is specified, all packets will be modified.
  - `operation` (enum) is the action to be performed, available operations are:
    - (AND, OR, XOR, NOT, SLEFT, SRIGHT) + `operand` field.
    - (PREPEND, APPEND, INSERT) + `payload` field.
    - NOT.
  - `operand` (byte) is the "right" value for the operations that need it *(AND, OR, XOR, NOT, SLEFT, SRIGHT)*.
  - `payload` (byte) is the "payload" value for the operations that need it *(PREPEND, APPEND, INSERT)*.
  - `pattern` (string) a regular-expression pattern complying with the [tiny-regex-c](https://github.com/kokke/tiny-regex-c), to restrict modifications to matching packets only.
- `q.packet_modification.reset()` will delete any stored rule.
- `q.packet_modification.dump()` will dump to CLI any stored rule.
- `q.packet_modification.auto_shift` (boolean), if enabled the module will automatically left shifts packets matching `^5555` to get `^aaaa` packets.
- `q.packet_modification.enabled` (boolean), controls whatever the module is enabled, **do not forget to set it!**

**Example:** Let's say that you want to invert byte 3 of all packets that end with `'XYZ'` and XOR with `0x44` all bytes which value is `'A'` (and in position 5) of all packets that start with `'AAA'`. And you want to ignore any packet that do not contain at least 3 digits in their payload. You're going to need two modifications and one filter:

```python
In [72]: q.packet_filter.add(
    pattern="[0-9]{3,}"  # ignore packets not containing at least 3 digits
)

# ...

In [73]: q.packet_modification.add(
    pattern="XYZ$"  # for all packets that end in "XYZ"
    position=3,     # at position 3
    operation=4     # apply a NOT of whatever value is there
                    # (no operand needed)
)

# ...

In [74]: q.packet_modification.add(
    pattern="^AAA"  # for all packets that start with "AAA"
    content=0x42,   # for all octects which value equals A
    position=5,     # and at position 5
    operation=3     # XOR the value with the operand
    operand=0x44
)

In [76]: q.packet_filter.enabled = True # enable packet filtering
In [77]: q.packet_modification.enabled = True # enable packet manipulation
```

**Example:** Let's say you are capturing packets by mean of a specific syncword filter; the radio will *consume* the preamble and the specified syncword to recognize the packet and, consequently, sends you the remaining payload.
You are not happy with this and want to *prepend* the consumed part. Well, nothing easier:

```python
In [78]: q.packet_modification.add(
    operation="PREPEND",                      # Select prepend action
    payload=bytes.fromhex("aaaaaaaaaae5e5")   # Prepend the consumed preamble and the syncword (\xE5\xE5)
   )
In [79]: q.packet_modification.enabled = True # enable packet manipulation
```