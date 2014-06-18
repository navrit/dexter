#!/usr/bin/env python

from PIL import Image
im = Image.open("spidr3.png")
(width, height)=im.size


white=(255,255,255,255)
for x in range(0,width,8):
  for y in range(0, height):
    if (x+1 < width and im.getpixel((x+1,y))[3]>0 ) or \
       (x-1 >=0     and  im.getpixel((x-1,y))[3]>0 ):
      im.putpixel((x,y), white)

for y in range(0, height,8):
  for x in range(0,width):
    if (y+1 < height and im.getpixel((x,y+1))[3]>0 ) or \
       (y-1 >=0     and  im.getpixel((x,y-1))[3]>0 ):
      im.putpixel((x,y), white)
    
im.save("out.png")
