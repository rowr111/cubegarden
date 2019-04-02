# Cube Garden Development Instructions
This code is based off https://github.com/bunnie/chibios-xz (bm17 branch).

## Getting Started

We assume you are building on a Raspberry Pi (so ARM-native) device,
and using gcc6. 

1. Check out https://github.com/rowr111/cubegarden (git checkout https://github.com/rowr111/cubegarden cubegarden)
2. Change to the "src" dir
3. Run "make -j3".  If you're cross-compiling it, add " TRGT=arm-none-eabi-" to the command.
4. If you get a complaint about stubs-soft.h, create an empty file of that name in the directory where the error message is pointing to and the error will go away.

The build result will be "build/bm17.elf", an object file that can be
loaded using openOCD into the badge.

## Connecting the SWD via OpenOCD

We'll use the GPIOs on the Raspberry PI to communicate with badge over
the SWD bus to load the firmware.

* Connect SWD to "GPIO 21"
* Connect SWC to "GPIO 20"
* Connect SRES to "GPIO 12"
* Connect a GND

The pin numbers are labelled on the headers on the component side of the board (non-OLED side).

It's recommended you solder any headers with the pins facing toward the component side (not toward
the OLED side) so that firmware updates are still possible using a pogo pin jig. The pogo pins
come down from the OLED side, so putting the pins facing outward will (a) cause you to be poked
by the pins as you wear the badge and (b) interfere with the pogo pins for reflashing using the
production jig.

You need to compile OpenOCD from source, and enable "bcm2835gpio".  Install the toolchain.  If you're using Raspbian, it's something like this:

    sudo apt-get install build-essential libtool gdb which
    git clone --recursive git://git.code.sf.net/p/openocd/code openocd
    cd openocd
    ./bootstrap
    ./configure --enable-bcm2835gpio --enable-sysfsgpio --disable-werror
    make
    sudo make install

Then, run OpenOCD:

    cd chibios-bm17/src
    sudo openocd -f bcm-rpi.cfg

## Loading the ELF File

In gdb, run "load [path-to-build]/build/bm17.elf" to upload the new OS to the board.

## Attaching GDB for debugging

If you've compiled your program using a normal toolchain, you can use GDB to load code and debug the software.  From your Desktop, run:

    arm-none-eabi-gdb -ex "target remote 192.168.xxx.xxx:3333" [path-to-your-program].elf

To look at OS threads in GDB, add the symbols from the orchard.elf file you built at load address 0:

    (gdb) add-symbol-file [path-to-orchard.elf] 0

You should now be able to look at threads using "info thr", and change threads with "thr [pid]".
