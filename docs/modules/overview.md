RFQuack's functionalities are built as pluggable modules, developed on top of a generic API.

When you fire up the Python shell, you can interact with the connected dongle through the `q` object; try auto-completion *(tab is your friend)* and discover each loaded module.

Each module has a built-in, super handy, helper function:

```python
RFQuack(/dev/ttyUSB0, 115200,8,N,1)> q.frequency_scanner.help()  

Helper for 'frequency_scanner' module:
> q.frequency_scanner.freq_step
Accepts: rfquack_FloatValue
Frequency step in Mhz (default: 1)

> q.frequency_scanner.start()
Accepts: rfquack_VoidValue
Starts frequency scan

...
```

For sure, you already understood how it works: `q.frequency_scanner.freq_step` is a `float` property; you are free to **get** it.

```python
RFQuack(/dev/ttyUSB0, 115200,8,N,1)> q.frequency_scanner.freq_step
value = 1.0
```

or **set** it:

```python
RFQuack(/dev/ttyUSB0, 115200,8,N,1)> q.frequency_scanner.freq_step = 5.0
result = 0
message =
```

While `q.frequency_scanner.start()` is a `function(void)` :

```python
RFQuack(/dev/ttyUSB0, 115200,8,N,1)> q.frequency_scanner.start()
result = 0
message = Nothing detected
```

That's all!

If unsure which parameters a function/property can take please check the `src/rfquack.proto` protocol definition. Since we're using reflection, IPython can't offer completion here (if you know a way to have completion on dynamic attributes, please let us know!).

In the following, we explore the main functionalities of each - built in - module through some examples.

## List Available Modules

There are few other built-in modules, and you can check their documentation by typing `q.moduleName.help()` in the CLI.
