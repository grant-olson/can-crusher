#UI Software

The current UI software runs on a Raspberry Pi Zero with an attached
TFT LCD Touchscreen. There are a variety of vendors that sell the same
style 320x240 with the same pinout on Amazon, eBay, and
AliExpress. Although it can be wired up manually there is also a Pi
Hat in the kicad section of the project to make an adapter PCBA.

It communicates with the main control software via a serial connection
and receives its power from the base unit.

There are three programs that are important:

`slide-maker` pre-builds optimized versions of graphics to make it
easier to run efficiently on the moderately slow Raspberry Pi Zero.

`can-crusher-pi` runs the code for the specified head unit
configuration above.

`can-crusher-cli` provides a simple way to run the code directly from
your dev computer with a USB-to-UART dongle, which is much easier than
trying to remotely program on the Pi Zero.

The `can_crusher` directory contains the python modules that do all
the heavy work.