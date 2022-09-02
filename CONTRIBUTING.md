# Contributing to RFQuack

- [Contributing to RFQuack](#contributing-to-rfquack)
  - [Code of Conduct](#code-of-conduct)
  - [Issues](#issues)
  - [Code style guidelines](#code-style-guidelines)

First of all, thank you very much for taking the time to contribute! All feedback and ideas are greatly appreciated.
To keep this library organized, please follow these guidelines.

**Credits:** Given the tight coupling between RFQuack and [RadioLib](https://gituhb.com/jgromes/RadioLib), these contributing guidelines as well as the issue templates are based on RadioLib's.

## Code of Conduct

Please read through our community [Code of Conduct](https://github.com/rfquack/RFQuack/blob/master/CODE_OF_CONDUCT.md).

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

1. **Tabs**  
Use 4 space characters for tabs.

1. **Single-line comments**  
Comments can be very useful and they can become the bane of readability. Every single-line comment should start at new line, have one space between comment delimiter `//` and the start of the comment itself. The comment should also start with a lower-case letter.

```c++
// this function does something
foo("bar");

// here it does something else
foo(12345);
```

1. **Split code into blocks**  
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

1. **No Arduino Strings**  
Arduino `String` class should never be used internally in the library. The only allowed occurrence of Arduino `String` is in public API methods, and only at the top-most layer.
