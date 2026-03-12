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

def read_end(the_port, End):
  n=0;
  total_data=[];data=''
  while n<30:
    #while True:
    try:
      data=the_port.read(1).decode()
      n+=1;
    except:
      break
    if End in data:
      total_data.append(data[:data.find(End)])
      break
    total_data.append(data)
    if len(total_data)>1:
      # check if end_of_data was split
      last_pair=total_data[-2]+total_data[-1]
      if End in last_pair:
        total_data[-2]=last_pair[:last_pair.find(End)]
        total_data.pop()
        break
  return ''.join(total_data)


ser.write("++addr 8\n".encode())  #address 14
ser.write("++auto 1\n".encode())   #do not auto-read
ser.write("++eos 0\n".encode())    #end of string CR+LF

cmd="VOLT 5\n"
ser.write(cmd.encode())
time.sleep(0.1)

cmd="OUTP %s\n" % str(sys.argv[1])
ser.write(cmd.encode())
time.sleep(0.1)
cmd="OUTP?\n"
ser.write(cmd.encode())
data=read_end(ser, '\r')
print(data)


