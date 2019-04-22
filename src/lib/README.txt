Files in this directory are needed to compile the code, but are specific to the version of raspbian you are using and the version of GCC (gnu compiler collection). As such they may not be included by default.

Make sure the link below matches the gcc version of your current build system - these instructions assume raspbian 9 (Stretch) and GCC 6.3.0.

https://developer.arm.com/-/media/Files/downloads/gnu-rm/6-2017q2/gcc-arm-none-eabi-6-2017-q2-update-linux.tar.bz2?revision=2cc92fb5-3e0e-402d-9197-bdfc8224d8a5?product=GNU Arm Embedded Toolchain,64-bit,,Linux,6-2017-q2-update

You should download this tarball, extract the files, and then navigate to the directories that contain the libraries we need that match the badge's CPU.

Copy the following files to the same directory as this readme:
./arm-none-eabi/lib/thumb/v7e-m/fpv4-sp/hard/libc_nano.a
./lib/gcc/arm-none-eabi/6.3.1/thumb/v7e-m/fpv4-sp/hard/libgcc.a
./arm-none-eabi/lib/thumb/v7e-m/fpv4-sp/hard/libm.a

The compiler expects two of the filenames to be slightly different, so you will need to additionally create symbolic links (shortcuts) to these files. 
Run the following commands to create the links in the same directory:
ln -s libgcc.a armv7e-m-fpu-libgcc.a
ln -s libc_nano.a libc-nano-armv7e-m-fpu.a
