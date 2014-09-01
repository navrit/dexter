#!/usr/bin/env python
import socket
import time
import struct
import random
UDP_IP = "192.168.100.1"
UDP_PORT = 8192
MESSAGE = "Hello, World!"
print("UDP target IP:", UDP_IP)
print("UDP target port:", UDP_PORT)

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM) # UDP
while 1:
    msg=""
    for pck in range(random.randint(1,100)):
      x=random.randint(0,255)
      y=random.randint(0,255)
      msg+=struct.pack("BBBBBBBB",0xA0, x,0x00,0x00,0x00,(y&0xf) << 4,(x&0xf) << 4 | (y>>4)&0xf ,0xA0 | (x>>4)&0xf)
    print len(msg)/8
    sock.sendto(msg, (UDP_IP, UDP_PORT))
#    time.sleep(0.0001*random.randint(1,100))
    time.sleep(0.0001)    
