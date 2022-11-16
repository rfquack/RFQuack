If you already have Python installed and usable as user, you can [skip to the bottom](#rfquack-dependencies).

We only require very few dependencies, namely [Protobuf](https://github.com/protocolbuffers/protobuf#protocol-compiler-installation) and [PlatformIO](https://docs.platformio.org/en/latest/). Most of the burden is on PlatformIO, which fortunately, can be installed as easy as a Python package.

If you don't want to deal with any of this, you can opt for a [Dockerized build system](../build).

## Protobuf

Installing the Protobuf compiler is pretty easy, but every system has its own package managers and compilers.

=== "macOS"

    ``` shell
    brew install protobuf protobuf-c
    ```

=== "Ubuntu/Debian"

    ``` shell
    apt install protobuf-compiler
    ```


## Python and Pip

RFQuack needs Python 3.10.* installed and usable as `$USER`. It's up to you to choose how you install and manage Python and Python packages in your system.

### Check Python Installation

To verify that your Python installation is usable and you can install packages as user:

``` shell
$ env python
Python 3.10.5 (main, Sep 20 2022, 10:54:01) [Clang 14.0.0 (clang-1400.0.29.102)] on darwin
Type "help", "copyright", "credits" or "license" for more information.
>>> print("hello world")
>>> hello world
>>> exit()
```

Let's try to install a package:

``` shell
pip install -U platformio
```

If this completes well, you're good to go.

### Have Pyenv and Poetry in Your Life

We highly recommend using [Pyenv](https://github.com/pyenv/pyenv) to manage Python on your system, and [Poetry](https://python-poetry.org/) to manage Python project dependencies.

#### Installing Pyenv

``` shell
curl https://pyenv.run | bash
```

The follow the [Pyenv post-installation instructions](https://github.com/pyenv/pyenv#set-up-your-shell-environment-for-pyenv) and install & select the latest Python 3.10.* version (3.10.5 at the time of writing).

```shell
pyenv global 3.10.5
```


#### Installing Poetry

Please follow the [official documentation](https://python-poetry.org/docs/#installation).

``` bash
curl -sSL https://install.python-poetry.org | python -
```

From now on, whenever you'll select the just-installed Python 3.10.* version, it'll come with Poetry.

## RFQuack Dependencies

Now you can install the actual dependencies needed by RFQuack:


=== "With Poetry (recommended)"

    ``` shell
    cd RFQuack
    poetry install
    ```

    **Bonus:** This will install the shorthand `rfq` to start the [RFQuack CLI Client](../clients/cli.md)

    ``` shell
    $ poetry shell
    Spawning shell within .venv

    $ rfq
    Usage: rfq [OPTIONS] COMMAND [ARGS]...

    Options:
      -l, --loglevel [CRITICAL|ERROR|WARNING|INFO|DEBUG|NOTSET]
      -h, --help                      Show this message and exit.

    Commands:
      mqtt  RFQuack client with MQTT transport.
      tty   RFQuack client with serial transport.
    ```

=== "Without Poetry"

    ```shell
    cd RFQuack
    pip install -r requirements.pip
    ```

