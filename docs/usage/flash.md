Now you're ready to connect the board via USB and flash it!

## Check for Connected Boards

### No Devices

If there are no RFQuack boards connected, you'll see something like this (or maybe just empty).

```shell
$ make lsd
pio device list
/dev/cu.Bluetooth-Incoming-Port
-------------------------------
Hardware ID: n/a
Description: n/a
```

### Device Found!


If there is at least one RFQuack board connected, you'll see something like this. We recommend having **one board connected at a time**.

```shell
$ make lsd
pio device list
/dev/cu.Bluetooth-Incoming-Port
-------------------------------
Hardware ID: n/a
Description: n/a

/dev/cu.usbserial-016424A3
--------------------------
Hardware ID: USB VID:PID=10C4:EA60 SER=016424A3 LOCATION=20-2
Description: CP2104 USB to UART Bridge Controller - CP2104 USB to UART Bridge Controller
```

The actual port name may change. In this specific case we can see an UART controller connected at `/dev/cu.usbserial-016424A3`.

## Let's Flash!

Without further ado:

```shell
make flash
```

Now you can jump to the [Clients](/clients/cli) section.
