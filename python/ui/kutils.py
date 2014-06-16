import sys
import socket
import getpass
import time
from xml.etree import ElementTree
from xml.dom import minidom
from xml.etree.ElementTree import Element, SubElement, Comment
from xml.dom.minidom import parse, parseString

def n2h(v, unit="",len=5):
  """ number to human representation"""
  prefix=['Y','Z','E','P','T','G','M','k','','m','u','n','p','f','a','z']
  sign=""
  if v<=0:
      sign="-"
      v=abs(v)
  order=1e24
  i=0
  if v<1e-21: return str(v)
  while True:
    if v>=order:
      v/=order
      vint=int(v)
      if vint>=100:
        l="%.0f"%(v)
      elif vint>10:
        l="%.1f"%(v)
      else:
        l="%.2f"%(v)
      l+=prefix[i]
      f="%%%ds"%len
      l=f%l
      if unit!="":
        l+=" [%s]"%unit
      l=sign+l
      return l
    order/=1e3
    i+=1

def prettify(elem):
    """Return a pretty-printed XML string for the Element.
    """
    rough_string = ElementTree.tostring(elem, 'utf-8')
    reparsed = minidom.parseString(rough_string)
    return reparsed.toprettyxml(indent="  ")

def get_time():
    return time.strftime("%H:%M:%S", time.localtime())

def get_date_time():
    return time.strftime("%Y/%m/%d %H:%M:%S", time.localtime())

def get_user_name():
    return getpass.getuser()

def get_host_name():
    return socket.gethostname()