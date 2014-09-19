#!/usr/bin/env python
import glob
import tarfile
import os
import subprocess
import urllib2
import re
from urllib2 import urlopen, URLError, HTTPError
from BeautifulSoup import BeautifulSoup
from string import Template

download_tmp="""
setMode -bscan
setCable -p auto
identify
assignfile -p 1 -file $fname
program -p 1
quit
"""
download_mcs="""
setMode -bscan
setCable -port auto
identify
identifyMPM 
attachflash -position 1 -bpi "28F00AG18F"
assignfiletoattachedflash -position 1 -file "$fname"
program -p 1 -dataWidth 16 -rs1 NONE -rs0 NONE -bpionly -e -v -loadfpga 
quit
"""

templates={'bit':download_tmp, 'mcs':download_mcs}

def system_call(cmds):
  proc = subprocess.Popen(cmds, stdout=subprocess.PIPE)
  (out, err) = proc.communicate()
  return out

def get_int(msg,_range,default):
  while True:
    choice=0
    try:
      choice = raw_input(msg)
      if choice =="":
        return default
      choice = int(choice)
      if not (_range[0] <= choice <= _range[1]):
          raise ValueError()
    except ValueError:
      print "Invalid value"
    else:
      return choice

def find_impact(dirname="/opt/Xilinx"):
  impacts=[]
  for root, dirs, files in os.walk(dirname):
    if root.find("bin")>=0 :
      if 'impact' in files:
        impacts.append(os.path.join(root, 'impact'))
  return sorted(impacts)
             


def dlfile(url):
    # Open the url
    try:
        f = urlopen(url)
        print "downloading " + url

        # Open our local file for writing
        with open(os.path.basename(url), "wb") as local_file:
            local_file.write(f.read())

    #handle errors
    except HTTPError, e:
        print "HTTP Error:", e.code, url
    except URLError, e:
        print "URL Error:", e.reason, url
        
def get_firmwares():
  page_url="http://cern.ch/kulis/spidr"
  html_page = urllib2.urlopen(page_url)
  soup = BeautifulSoup(html_page)
  r=[]
  for link in soup.findAll('a'):
    r.append( (link.get('href'), link.getText()) )
  return r

def main():

  impact=system_call(["which","impact"])
  if impact=="":
    print "There is no impact in yours PATH."
    print "Try to setup your enviroment by: "
    print "  source /opt/Xilinx/14.X/ISE_DS/settingsYY.sh"
    impacts=find_impact()
    if len(impacts)>0:
        print "Or use one of the following binaries"
        print " [0] Cancel"
        
        for _id,_impact in enumerate(impacts):
          print " [%d] %s"%(_id+1,_impact)
        c=get_int("Enter choice [0] : ",(0,len(impacts)),0)
        if c==0: 
           return
        else:
           impact=impacts[c-1]
    else:
        print "Unable to find any binaries ..."
        return
  print "\nImpact : %s"%impact
  print
  id=0

  fnames=[]
  print "Online-builds:"
  for link,desc in get_firmwares():
    print " [%d] %s (%s)"%(id,desc,link)
    fnames.append(link)
    id+=1
  print 
  print "Local builds:"
  for fn in sorted(glob.glob("spidr_*.tar.gz"), reverse=True):
    l=fn.split("_")
    if l[0]!='spidr': continue
    print " [%d]  20%s/%s/%s  %s:%s"%(id,l[1][0:2],l[1][2:4],l[1][4:6],l[2],l[3].split(".")[0])
    fnames.append(fn)
    id+=1
  _range=(0,id-1)
  c=get_int("Enter choice [0] : ",_range,0)
  c=0
  fname=fnames[c]
  print "Flashing %d"%c,fname
  dlfile(fname)
  fname=str(fname[fname.rfind("/")+1:])
  print fname
   
  print "Tar contains"
  tar = tarfile.open(fname)
  id=0
  options=[]  
  for ti in tar.getmembers():
    tt=ti.name.split(".")
    ext=tt[-1]
    if ext in ("bit", "mcs"):
      if ti.name.find("with_elf")>=0:
        info=""
        if ext=='bit':info+="volatile memory"
        if ext=='mcs':info+="non-volatile memory"
        print " [%d] %s (%s)"%(id,ti.name,info)
        options.append((ext,ti.name))
        id+=1
  _range=(0,id-1)
  c=get_int("Enter choice [0] : ",_range,0)

  _type,_fname=options[c]
  _dict = dict(fname=_fname)
  print _dict
  tar.extract(_fname)
  tar.close()
  
  s = Template(templates[_type])
  
  f=open("_impact.cmd","w")
  f.write(s.substitute(_dict))
  f.close()
  os.system("impact -batch _impact.cmd")
  
if __name__=="__main__":
  main()
