#!/usr/bin/python3
import sys
import time
import serial
import socket
ser=serial.Serial(
port='/dev/ttyGPIB',
baudrate=460800,
timeout=.1
)
ser.flushInput()

ser.write("++addr 7\n".encode())   #address 14
ser.write("++auto 0\n".encode())   #do not auto-read
ser.write("++eos 3\n".encode())    #end of string CR+LF

ser.write("AP +13.0 DM\n".encode())
time.sleep(.5)
cmd="FR %.6f MZ\n" % float(sys.argv[1])
ser.write(cmd.encode())

ser.close()
