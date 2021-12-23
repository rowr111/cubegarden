#!/usr/bin/python3

import subprocess
import argparse
import atexit
import RPi.GPIO as GPIO
import time

GPIO_RUN=19   # above the ready
GPIO_READY=26 # above the ground
GPIO_START=21 # opposite the ground

def run_start(dummy):
    GPIO.output(GPIO_RUN, 1)
    while GPIO.input(GPIO_START) == GPIO.LOW:
        time.sleep(0.1)
        
    result = subprocess.run(['sudo', '/usr/local/bin/openocd', '-f', '/home/pi/code/cubegarden/src/bcm-rpi-prog.cfg'], timeout=10)
            
    print("{}".format(result))
    GPIO.output(GPIO_RUN, 0)
    
def main():
    parser = argparse.ArgumentParser(description="Cubegarden hack driver")
    parser.add_argument(
        "-v", "--version", help="Version 0", default=True, action="store_false"
    )
    args = parser.parse_args()
    
    GPIO.setmode(GPIO.BCM)
    GPIO.setup(GPIO_START, GPIO.IN, pull_up_down=GPIO.PUD_UP)
    GPIO.setup(GPIO_READY, GPIO.OUT)
    GPIO.output(GPIO_READY, 1)
    GPIO.setup(GPIO_RUN, GPIO.OUT)
    GPIO.output(GPIO_RUN, 0)

    GPIO.setup(23, GPIO.OUT)
    GPIO.setup(18, GPIO.OUT)
    GPIO.output(23, 1)
    GPIO.output(18, 1)
    
    GPIO.add_event_detect(GPIO_START, GPIO.FALLING, callback=run_start)

    print("hello")
    while True:
        time.sleep(0.1)

def cleanup():
    GPIO.output(GPIO_READY, 0)
    GPIO.cleanup()
            
if __name__ == "__main__":
    atexit.register(cleanup)
    try:
        print("Tester main loop starting...")
        main()
    except KeyboardInterrupt:
        pass
