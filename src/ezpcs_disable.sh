#!/bin/sh

if [ ! -d /sys/class/gpio/gpio18 ]
then
    echo "18" > /sys/class/gpio/export
fi

echo "out" > /sys/class/gpio/gpio18/direction
echo "1" > /sys/class/gpio/gpio18/value
