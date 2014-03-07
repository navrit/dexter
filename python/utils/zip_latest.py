#!/usr/bin/env python
import os
import sys
import zipfile
import shutil
import glob


def main():
  if len(sys.argv)!=3:
    print sys.argv[0],' out_file.zip directory'
    return
  zipname=sys.argv[1]
  d=sys.argv[2]
  skip_info=""
  zip = zipfile.ZipFile(zipname, 'w')
  for fn in glob.glob(d+"*"):
    last=fn+"/last"
    if os.path.islink(last):
      for root, dirs, files in os.walk(last):
        for file in files:
            if file=="details.zip": continue
            full_name=os.path.join(root, file)
            size=os.path.getsize(full_name)
            if size<1024*1024:
               zip.write(full_name)
            else:
              skip_info+="file:%s  size:%d\n"%(full_name,size)
  if skip_info!="":
    fs=open(d+"/skipped.txt","w")
    fs.write(skip_info)
    fs.close()
    zip.write(d+"/skipped.txt")
  zip.close()


if __name__=="__main__":
  main()

