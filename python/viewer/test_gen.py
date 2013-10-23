#!/usr/bin/env python
f=open("test00.dat","w")
for x in range(256):
  for y in range(256):
    v=x*y
    f.write("%d "%v)
  f.write("\n")
f.close()


f=open("test01.dat","w")
for x in range(256):
  for y in range(256):
    v=0
    if x>y:v=1
    f.write("%d "%v)
  f.write("\n")
f.close()
    
    
f=open("test02.dat","w")
for x in range(256):
  for y in range(256):
    v=abs(x-128)*abs(y-128)
    f.write("%d "%v)
  f.write("\n")
f.close()


f=open("test03.dat","w")
for y in range(256):
  for x in range(256):
    v=x%32+float(y)/13.2
    f.write("%.2f "%v)
  f.write("\n")
f.close()

from math import sqrt,pow
f=open("test04.dat","w")
for y in range(256):
  for x in range(256):
    v=200.0-sqrt( pow(x-100,2) + pow(y-120,2))
    f.write("%.2f "%v)
  f.write("\n")
f.close()

from math import sin
f=open("test05.dat","w")
for y in range(256):
  for x in range(256):
    v=sin(float(x)/10)+float(y)/255
    f.write("%.2f "%v)
  f.write("\n")
f.close()

from math import cos
f=open("test06.dat","w")
for y in range(256):
  for x in range(256):
    v=sin(float(x)/10)+cos(float(y)/20)
    f.write("%.2f "%v)
  f.write("\n")
f.close()

from math import cos
f=open("test07.dat","w")
for y in range(256):
  for x in range(256):
    b=(x/8)%2
    c=y%13
    v=b+c*abs(x-y)
    
    f.write("%.2f "%v)
  f.write("\n")
f.close()


