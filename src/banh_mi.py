#!/usr/bin/python3

import subprocess
import argparse
import atexit
import RPi.GPIO as GPIO
import time

GPIO_START=21

def run_start(dummy):
    print("start pushed")
    
def main():
    parser = argparse.ArgumentParser(description="Cubegarden hack driver")
    parser.add_argument(
        "-v", "--version", help="Version 0", default=True, action="store_false"
    )
    args = parser.parse_args()
    
    GPIO.setmode(GPIO.BCM)
    GPIO.setup(GPIO_START, GPIO.IN, pull_up_down=GPIO.PUD_UP)

    GPIO.add_event_detect(GPIO_START, GPIO.FALLING, callback=run_start)

    print("hello")
    while True:
        time.sleep(0.1)

def cleanup():
    GPIO.cleanup()
            
if __name__ == "__main__":
    atexit.register(cleanup)
    try:
        print("Tester main loop starting...")
        main()
    except KeyboardInterrupt:
        pass
