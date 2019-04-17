# Cube Garden Development Instructions
This code is based off https://github.com/bunnie/chibios-xz (bm17 branch).

## Getting Started

The development starting point is a Raspberry Pi 3B+. If you need to image one,
please grab the disk image from https://bunniefoo.com/bunnie/cubegarden-base.img.gz.

In this environment:

* The code is pre-built
* openocd is already installed

This means you can skip a significant amount of tooling and futzing before getting started
on dev work.

To build the code:

```
  cd ~/code/cubegarden/src
  make -j3
```

The -j3 option just multi-threads the build to make it run faster if you have a lot of changes.

The build result will be "build/bm17.[elf](https://en.wikipedia.org/wiki/Executable_and_Linkable_Format)", an object file that can be
loaded using [openOCD](http://openocd.org/) into the cube controller.

If you're using a cube controller, you're in luck, the pinout of the
cube controller exactly matches that of the Raspberry Pi, so plug the
Raspberry Pi directly into the cube controller's header. Be sure to
align pin 1 correctly, or you will damage the hardware.

If you're developing using a BM17 badge, see the next section on how to
wire up the badge to the Raspberry Pi.

## Badge hack: wiring up the pins

We'll use the [GPIO](https://www.w3schools.com/nodejs/nodejs_raspberrypi_gpio_intro.asp)s on the Raspberry PI to communicate with badge over
the [SWD](https://en.wikipedia.org/wiki/JTAG#Serial_Wire_Debug) [bus](https://en.wikipedia.org/wiki/Bus_(computing)) to load the firmware.

```
   40-pin header on the Raspberry Pi (pin 1 is the corner closest to the metal shield with the debossed raspberry pi logo)

           +3V3 1  2   +5V
   GPIO2   TDO  3  4   +5V
   GPIO3        5  6   GND
   GPIO4   TDI  7  8   TXD0    RX
           GND  9  10  RXD0    TX
   GPIO17 SRES  11 12  GPIO18  NMI
   GPIO27  SWD  13 14  GND
   GPIO22  SCK  15 16  GPIO23  TRST
```

* Connect SWD to pin 13
* Connect SWC to pin 15
* Connect SRES to pin 11
* Connect RX to pin 8
* Connect TX to pin 10
* Connect a GND (say, pin 9)

The pin numbers are labelled on the headers on the component side of the board (non-OLED side).

Did your wires fall out? see these photos to help remind you how to wire them up:

https://github.com/rowr111/cubegarden/blob/master/hardware/bm-badge-side.jpg
https://github.com/rowr111/cubegarden/blob/master/hardware/bm-rpi-side.jpg

## Connecting to the controller and debuging your code

It's recommended you open three or four ssh terminals to the Raspberry Pi for the easiest debugging.
The purpose of the terminals are as follows:

1. Terminal to control JTAG, the low-level bus used to push code onto the cube controller CPU.
2. Terminal to control gdb. This is your line-by-line interactive debugger, where you can set
breakpoints and observe variables.
3. Terminal with your code text editor. This is where you're writing/updating your actual code.
4. (optional) Terminal to monitor the serial port. This is only needed if you're doing "printf" style
debugging in addition to the gdb debugging. 

### The JTAG controller

From ~/code/cubegarden/src, run the following command:

    sudo openocd -f bcm-rpi.cfg

The terminal will give status updates about the CPU's operation, etc.; no further interaction necessary. Here's what it looks like:

[![asciicast](https://asciinema.org/a/241414.svg)](https://asciinema.org/a/241414)

### gdb

From ~/code/cubegarden/src, run the following commands:

    gdb
    (gdb) target remote localhost:3333
    (gdb) load build/bm17.elf

* The first command starts gdb.
* The second command connects to openocd, which is a port at localhost:3333
* The third command loads your most recent code build into the badge, permanently overwriting
the previous code contents.

Here's a quick example of using gdb:

[![asciicast](https://asciinema.org/a/241415.svg)](https://asciinema.org/a/241415)

To look at OS threads in GDB, add the symbols from the orchard.elf file you built at load address 0:

    (gdb) add-symbol-file [path-to-orchard.elf] 0

You should now be able to look at threads using "info thr", and change threads with "thr [pid]".

### Text editor

Use emacs, vi, nano, etc. to edit your code.

### Serial port

Use the following command to connect to the serial port:

    screen /dev/ttyS0 115200

If the serial data seems fragmented, then likely you had a previous session you didn't
quit out of correctly. Try screen -r, or killing any other screen processes resident in
the system.

To quit out of a screen session "gracefully", type control-A, then \, and then q

[![asciicast](https://asciinema.org/a/241416.svg)](https://asciinema.org/a/241416)

### Random notes

git key management:

```
 `eval ssh-agent`
 ssh-add ~/.ssh/<key>
```