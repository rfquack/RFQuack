# Contributing to RFQuack

- [Contributing to RFQuack](#contributing-to-rfquack)
  - [Code of Conduct](#code-of-conduct)
  - [RadioLib and 3rd Party Libraries](#radiolib-and-3rd-party-libraries)
  - [Issues](#issues)
  - [Code style guidelines](#code-style-guidelines)
    - [Tabs](#tabs)
    - [Single-line comments](#single-line-comments)
    - [Split code into blocks](#split-code-into-blocks)
    - [No Arduino Strings](#no-arduino-strings)

---

First of all, thank you very much for taking the time to contribute! All feedback and ideas are greatly appreciated.
To keep this library organized, please follow these guidelines.

## Code of Conduct

Please read through our community [Code of Conduct](https://github.com/rfquack/RFQuack/blob/master/CODE_OF_CONDUCT.md).

## RadioLib and 3rd Party Libraries

If you find a bug or want to add a feature to a radio module (e.g., CC1101), you must take into account that *most*, but not all, the radio abstraction is implemented through [RadioLib](https://github.com/jgromes/RadioLib).
Those changes that cannot be accommodated into RadioLib's upstream, we have [RFQuack's fork of RadioLib](https://github.com/rfquack/radiolib).

Given this tight coupling between RFQuack and RadioLib, we try to make absolutely minimal changes into RFQuack's fork of RadioLib and always send PRs back to ensure we don't diverge too much from RadioLib's upstream.

In short:

1. **Use RadioLib for radio abstraction:** if we need to add a new radio module not currently supported by RadioLib, it should be implemented in RFQuack's RadioLib for and then send a pull request.
2. **Send pull requests to RadioLib:** if we can make an acceptable change in RadioLib, we should do it and send a pull request.
3. **Use radio wrappers:** if the pull request is not accepted, we incorporate the change in RFQuack's wrapper classes (e.g., the CC1101 wrapper `radio/RFQCC1101.h`).
4. **Use our fork (last resort):** if the change can only be made in RadioLib, we keep it in our fork.

The same principles should be applied for any other 3rd party library. It's easy to make a fork and modify, but it's not sustainable. So, whenever possible, let's try to send pull requests to minimize divergency.
## Issues

The following rules guide submission of new issues. These rules are in place mainly so that the issue author can get help as quickly as possible.

1. **Questions are welcome, spam is not.**  
Any issues without description will be considered spam and as such will be **CLOSED** and **LOCKED** immediately!
2. **This repository has issue templates.**  
To report bugs or suggest new features, use the provided issue templates. Use the default issue only if the templates do not fit your issue type.
3. **Be as clear as possible when creating issues.**  
Issues with generic titles (e.g. "not working", "lora", etc.) will be **CLOSED** until the title is fixed, since the title is supposed to categorize the issue. The same applies for issues with very little information and extensive grammatical or formatting errors that make it difficult to find out what is the actual issue.
4. **Issues deserve some attention too.**  
Issues that are left for 2 weeks without response by the original author when asked for further information will be closed due to inactivity. This is to keep track of important issues, the author is encouraged to reopen the issue at a later date.

## Code style guidelines

I like pretty code! Or at least, I like *consistent* code style. When creating pull requests, please follow these style guidelines, they're in place to keep high code readability.

1. **Bracket style**  
This library uses the following style of bracket indentation (1TBS, or "javascript" style):

```c++
if (foo) {
  bar();
} else {
  baz();
}
```

### Tabs

Use 4 space characters for tabs.

### Single-line comments

Comments can be very useful and they can become the bane of readability. Every single-line comment should start at new line, have one space between comment delimiter `//` and the start of the comment itself. The comment should also start with a lower-case letter.

```c++
// this function does something
foo("bar");

// here it does something else
foo(12345);
```

### Split code into blocks

It is very easy to write code that machine can read. It is much harder to write one that humans can read. That's why it's a great idea to split code into blocks - even if the block is just a single line!

```c++
// build a temporary buffer (first block)
uint8_t* data = new uint8_t[len + 1];
if(!data) {
  return(STATUS);
}

// read the received data (second block)
state = readData(data, len);

// add null terminator (third block)
data[len] = 0;
```

### No Arduino Strings

Arduino `String` class should never be used internally in the library. The only allowed occurrence of Arduino `String` is in public API methods, and only at the top-most layer.
