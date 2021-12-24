#!/usr/bin/python3

"""
Update the cube garden data
"""

import time # for sleep and timestamps
from datetime import datetime # for deriving human-readable dates for logging
import subprocess
import argparse
from pathlib import Path
import uuid
import random

# all this plumbing, just to get my IP address. :-P
import os
import socket
import fcntl
import struct
def get_ip_address(ifname):
     s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
     try:
         addr = socket.inet_ntoa(fcntl.ioctl(
            s.fileno(),
            0x8915, # SIOCGIFADDR
            struct.pack('256s', ifname[:15]) )[20:24])
         return addr
     except:
         return None

logfile=None

def do_update_cmd(cmd, timeout=60, cwd=None):
    global environment
    global logfile
    cmd_str = ''
    for item in cmd:
         cmd_str += item
         cmd_str += ' '
     
    result = subprocess.run(cmd, capture_output=True, timeout=timeout, env=environment, cwd=cwd)
    stdout = result.stdout.decode("utf-8").splitlines()
    stderr = result.stderr.decode("utf-8").splitlines()
    print("do_update_cmd: " + cmd_str)
    print(result.stdout.decode("utf-8"))
    print(result.stderr.decode("utf-8"))
    if logfile:
        logfile.write("do_update_cmd: {}\n".format(cmd_str))
        logfile.write(result.stdout.decode("utf-8"))
        logfile.write(result.stderr.decode("utf-8"))
     
def main():
    global logfile
    parser = argparse.ArgumentParser(description="Cubegarden hack driver")
    parser.add_argument(
        "-v", "--version", help="Version 0", default=True, action="store_false"
    )
    parser.add_argument(
        "-l", "--log", help="When present, suppress log output to /home/pi/cubelog/", default=True, action="store_false"
    )
    args = parser.parse_args()

    if args.log:
        try:
             logfile = open('/home/pi/cubelog/{}_{}_{:%Y%b%d_%H-%M-%S}.log'.format(hex(uuid.getnode()), random.randint(0, 100000), datetime.now()), 'w')
        except:
             logfile = None # don't crash if the fs is full, the show must go on!
    else:
        logfile = None
    
    print("hello, waiting for boot to finish (20 seconds)")
    time.sleep(20)
    print("wait done, now fetching")

    # copy test logs to a restricted user on ci.betrusted.io via pre-loaded private key not in repo
    # scp -o StrictHostKeyChecking=no -i ~/testlogs_pi * testlogs@ci.betrusted.io:

    do_update_cmd(['git', 'pull', 'origin', 'master'], timeout=60)
    
    # copy the latest firmware
    do_update_cmd(['scp', '-o', 'StrictHostKeyChecking=no', '-i', '/home/pi/testlogs_pi', '-r', 'testlogs@ci.betrusted.io:cubes/cube.elf', '/home/pi/code/cubegarden/src/build/cube.elf'], timeout=30)
    # check the md5
    do_update_cmd(['md5sum', '/home/pi/code/cubegarden/src/build/cube.elf'], timeout=30)
    # and size
    do_update_cmd(['ls', '-l', '/home/pi/code/cubegarden/src/build/cube.elf'], timeout=30)

    # now copy the logfile back to the host
    if logfile:
        logfile.flush()
    do_update_cmd(['scp', '-o', 'StrictHostKeyChecking=no', '-i', '/home/pi/testlogs_pi', '-r', '/home/pi/cubelog/', 'testlogs@ci.betrusted.io:cubes/'], timeout=30)
    

if __name__ == "__main__":
    try:
        print("Updating...")
        main()
    except KeyboardInterrupt:
        pass
     
