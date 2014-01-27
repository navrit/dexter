#!/usr/bin/env python
import svgwrite

f=open("timepix3.xml","w")
dwg = svgwrite.Drawing('timepix3.svg', profile='tiny')
DIAM=200.00
dwg.add(svgwrite.shapes.Circle(center=(0, 0), r=DIAM/2,fill="#CCCCCC",stroke="black", stroke_width=0.2))
f.write('<wafer diameter="%.3f">\n'%DIAM)
X2LETER=['M','L','K','J','I','H','G','F','E','D','C','B','A']

DIE_W=14.2
DIE_H=16.69
j=0
for YM in range(1,11+1):
  Y=12-YM
  if Y in (1,) :     MINX,MAXX=6,8
  if Y in (11,) :    MINX,MAXX=5,9
  if Y in (2,) :   MINX,MAXX=4,10
  if Y in (3,10) :     MINX,MAXX=3,11
  if Y in (4,8,9) :  MINX,MAXX=2,12
  if Y in (5,6,7) :  MINX,MAXX=1,13
  
  for XM in range(MINX,MAXX+1):
    X=MAXX-XM+MINX
    j+=1
    name="%s%d"%(X2LETER[X-1],Y)
    x0=((14-X)-7-0.5)*DIE_W
    y0=((12-Y)-6-0.5)*DIE_H
    f.write('  <die name="%s" x="%7.3f" y="%7.3f" w="%.3f" h="%.3f"/>\n'%(name,x0,y0,DIE_W,DIE_H))
    dwg.add(svgwrite.shapes.Rect(insert=(x0, y0), size=(DIE_W,DIE_H), fill='#BBBBFF',stroke="red", stroke_width=0.3))
    dwg.add(dwg.text(name, x=[x0+DIE_W/2], y=[y0+0.7*DIE_H],text_anchor="middle",font_size = "6px"))

f.write('<notch angle="180.0" length="5.0"/>\n')
f.write('<chuck x="-100" y="-100"/>\n')
f.write('<home name="G11"/>\n')

f.write('</wafer>\n')
print j

f.close()
dwg.save()
