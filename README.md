# Cube Garden Development Instructions

Welcome to the cubegarden readme! This code controls the lighting of a large translucent cube - based on interactions with the cube, sensors onboard can trigger various lighting effects.

* cubegarden is the main repository.
  * it is based off https://github.com/bunnie/chibios-xz (bm17 branch).
* there are three submodules:
  * ChibiOS
  * ChibiOS-Contrib
  * ugfx

# Getting Started - Getting everything connected and your code running on the controller:
## Step 1: Get a Raspberry Pi and connect it
### Raspberry Pi 
The development starting point is a Raspberry Pi 3B+. 
Please use the raspberry pi disk image from https://bunniefoo.com/bunnie/cubegarden-base.img.gz.
In this image things are already set up for you so it's easy to start developing.

Included items:
* The code is already located at ~/code/cubegarden
  * if you are not using the image/getting the code from scratch for some reason, be sure to recurse submodules when cloning the repo: `git clone --recurse-submodules https://github.com/rowr111/cubegarden.git`
* openocd (how we connect to the cube's electronics) is already installed

Notes about connecting/shutting down the raspberry pi:
* you must first plug the rapsberry pi into a hdmi monitor and configure it to be on your wireless network.
  * login:  username is 'pi', pw is (ask us!)
* after the rpi is set up on your wireless network, you can use ssh from a computer on your network to connect to it:
  * this command from Terminal (mac) or powershell (windows) should do the trick: `ssh pi@cubedev.local`
* you **must** shut down the raspberry pi, it is not an arduino that you can unplug freely.  
  * run the command: `sudo shutdown -h now`
    * the raspberry pi will close the ssh connection, you'll see a bit of blinking from the on-board led.  After that is quiet, you can unplug it.

#### GitHub notes for the Raspberry Pi
Regardless of if you are using the existing image or not:
You need a ssh key set up on the raspberry pi in order to commit to github from the raspberry pi, etc.  
* See instructions [here](https://help.github.com/en/articles/adding-a-new-ssh-key-to-your-github-account) for how to generate the key, add to ssh-agent, and add to your github account.
  * ignore the part about checking for existing keys as there's one as part of the cloned rpi image that we don't want to use - just make a new one for yourself.
  
You must also set your username and email address for commits. 
Run the following commands on the Raspberry Pi (adding your own username/email where appropriate):
```
git config -—global user.name "mygithubusername"
git config -—global user.email "mygithubemail"
```
### connecting the raspberry pi to the board
The cubegarden will have its own controller, but you can also use a BM17 badge for development:
* Using the controller specifically manufactured for cubegarden - the rapsberry pi will plug directly into the cube controller's header.
  * Be sure to align pin 1 correctly, or you will damage the hardware.
* Using a BM17 badge - please use the instructions [here](https://github.com/rowr111/cubegarden/wiki/wire-a-badge-to-a-raspberry-pi) to wire the badge to the raspberry pi.
  * **you must plug in the badge to usb power as well has having the battery connected in order to connect**
  * turning the badge off after cubegarden code has been pushed to it:  
    * you must both unplug it from USB power **and** disconnect the battery manually to turn it off.
      * because the UI has been removed in the cubegarden code, there is no way to power down the badge besides disconnecing the battery.
      * to disconnect the battery you must unscrew the case on the badge.

## Step 2: building the cubegarden code
First, we'll get up-to-date code from git and build it:
* Open a ssh connection to the raspberry pi.
* Do a git pull on ~/code/cubegarden to get the latest code. 
* To build the code, navigate to ~/code/cubegarden/src and run the command `make -j3`

```
  cd ~/code/cubegarden/src
  make -j3
```

The -j3 option just multi-threads the build to make it run faster if you have a lot of changes.

The build result will be "build/bm17.[elf](https://en.wikipedia.org/wiki/Executable_and_Linkable_Format)", an object file that can be
loaded using [openOCD](http://openocd.org/) into the cube controller.

Errors: If you get an error about the submodules when building, this may because the submodules are not on the right version.
If this happens, make sure the submodules are on the right version:
* navigate to the ChibiOS dir and run `git checkout 9942787`
* navigate to the ChibiOS-Contrib dir and run `git checkout e6b624e`

## Step 3: Open a connection to the controller
Next, we need to open a connection to the controller so that we can push code, debug, and monitor everything.
From ~/code/cubegarden/src, [run the following command](https://asciinema.org/a/241414):

    sudo openocd -f bcm-rpi.cfg

The terminal will give status updates about the CPU's operation, etc.; no further interaction necessary in this window, just leave it open.

## Step 4: Use GDB to push code and enable debugging
Next, we'll start the software used to push the code and do debugging later:
* Open a new ssh connection to the raspberry pi (don't close the existing)
* From ~/code/cubegarden/src, run the following commands:

```
     gdb
     (gdb) target remote localhost:3333
     (gdb) load build/bm17.elf
```

What these commands do:
* The first command starts gdb.
* The second command connects to openocd, which is a port at localhost:3333
* The third command loads your most recent code build into the badge, permanently overwriting
the previous code contents.

If this succeeds, press c and enter to continue, and the code should be running on the badge.
This is the window from which you can do debugging tasks later - set breakpoints and observe variables, etc.

## Step 5: Connect to the serial port to enable giving the controller direct commands and view debugging output
* Open yet another ssh connection to the raspberry pi (still keep all existing connections open)
* Use the [following command](https://asciinema.org/a/241416) to connect to the serial port (you do not need to navigate anywhere special first):
```
    screen /dev/ttyS0 115200
```
If the serial data seems fragmented, then likely you had a previous session you didn't
quit out of correctly. Try screen -r, or killing any other screen processes resident in
the system.

Everything should clear out and you are now connected directly to the controller. 

**Hooray!  You are set up and the code is running!!**

* If you type '**help**' and press enter you should see a list of commands you can issue to the badge.
* Debugging output you add to the code will also print to this window.
* To quit out of a screen session "gracefully", type control-A, then \, and then q

# Writing Code and Debugging

## Writing Code
There are a few options for writing/editing the code:
* directly on the raspberry pi via text editors like emacs, vi, nano, etc. 
* write code on your own computer using something like VS Code and then push your changes to the raspberry pi:
  * options here include:
    * `rsync -aiv -e ssh yourcodesource  pi@XXX.XX.XX.XXX:code/cubegarden/`

## Debugging using gdb

[Here's a quick example of using gdb](https://asciinema.org/a/241415).

To look at OS threads in GDB, add the symbols from the orchard.elf file you built at load address 0:

    (gdb) add-symbol-file [path-to-orchard.elf] 0

You should now be able to look at threads using "info thr", and change threads with "thr [pid]".
