Welcome to the repository for the most sophisticated robotic aluminum
can crusher that the world has seen!

This is the result of a hobby project that combines CAD design,
Hardware, PCB design, firmware, and software to build an over-the-top
computer-controlled can crusher. I wanted to use a wide variety of skills
on this project instead of going deep into a specific skill.

## In depth 30 minute video on the project

[The Most Technologically Advanced Aluminum Can Crusher In The World](https://youtu.be/nmYN3EZNI_g)

## The Frame

![Crusher at home](./can-crusher-in-kitchen.jpg)

The frame starts with 3 standard 2040 slotted aluminum extrusions. From there
the major structural elements, the:

* top
* bottom
* crusher plate

Are made from laser cut aluminum.

The rest of the parts are 3D printed.

All design elements are in the included FreeCAD workspace. If you just
want to cut or print the parts refer to the [stl directory](./stl) for
3D print files or the [step directory](./step) for files suitable for laser
cutting.

You will also need two stepper motors with rods and two smooth rods to
assemble the frame in its entirety.

## Stepper Control Board

![Stepper Controller](./stepper-control-pcba.jpg)

To drive the stepper motors I've made a control board powered by a Raspberry
Pi Pico that uses TMC2209 chips to drive the steppers. This allows me to
auto-detect limits of movement.

The design is in KiCad. Gerber files are also included in the repo for
getting PCBs made.

At a high level the control board:

* Initializes the system
* Accepts a small command language over the UART interface.
* Controls the motors appropriately.

I chose the Pi Pico so that I could use the PIO subprocessors to get
extremely reliable control of the stepper signal waves. I'm able to
run those completely deterministicly based on the clock speed where
even IRQs won't affect triggering.

To build the software:

* Set up your computer to build for Pico projects.
* `cd firmware/pico`
* `mkdir build`
* `cd build`
* `cmake ..`
* `make`
* `picotool load can-crusher.uf2 && picotool reboot`

If you have stepper motors hooked up they should perform a minor
up/down jogging of the motors upon reboot. Use this to see if the
code is actually running without the admin consoles set up.

## High level control and UI

![Head Unit](./touchscreen-display.jpg)


For the high level control and UI I've written a program for a Raspberry
Pi that shows a minimal display on a TFT-LCD with touchscreen to allow users
to interact.

To make a compact display I've created a Hat for a Pi Zero board that allows
you to mount the parts together directly that is also released as a KiCad file.

Due to continuing Raspberry Pi shortages I'd like to build some
alternate versions of the control boards for other SBCs. The software
itself iswritten in python and should be portable to pretty much any
linux system. There are only 3 hardware requirements so I expect other
boards will be easy to integrate:

* Ability to have a hardware level SPI control with /dev/spidevX.X exposed.

* A kernel that supports ADS7846  touch screens and provides integration with
    `evdev` to read touch screen events.

* A hardware serial/uart port capable of running at 115200 baud.


![Cad render](./can-crusher-cad.png)

