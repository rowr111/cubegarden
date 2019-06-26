# Cube Garden Development Instructions

Welcome to the cubegarden readme! This code controls the lighting of a large translucent cube - based on interactions with the cube, sensors onboard can trigger various lighting effects.  

There is also code for a cube master controller, that takes advantage of the BM17 heart badge's UI in order to select effect patterns and make changes to all the cubes at once. 

* cubegarden is the main repository.
  * it is based off https://github.com/bunnie/chibios-xz (bm17 branch).
* there are three submodules:
  * ChibiOS
  * ChibiOS-Contrib
  * ugfx
  
## Requirements and Setup
There are detailed set up instructions [on the wiki](https://github.com/rowr111/cubegarden/wiki/getting-everything-connected-and-the-cube-code-running-on-the-controller) for both getting everything set up with a raspberry pi, and for getting the cubegarden code running on either the cubegarden board (for cubes only) or a BM17 badge (for the master controller only).

You will need:  
* a raspberry pi 3B+ connected to your local wireless internet
* a cube board and/or BM17 heart badge


