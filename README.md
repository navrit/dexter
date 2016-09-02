# README #

Steps to get this repository up and running.

### What is this repository for? ###

* Medipix 3 GUI

### How do I get set up? ###

* Summary of set up
* Configuration
* Dependencies
    * Qt 5.7.0+
    * cmake
    * GCC 64 bit
          * In /usr/bin
    * dlib (19.0+)
          * Compile http://dlib.net/compile.html
          * Copy folder to ../mpx3gui/ - so dlib/ is at same level as mpx3gui/
    * Boost (1.60+)
          * libboost-all-dev
    * Phidget (21)
          * libusb-1.0-0-dev
          * http://www.phidgets.com/docs/OS_-_Linux
    * Openblas 
          * libblas-dev
          * libblas3
    * Lapack
          * liblapack-dev
          * liblapack3
    * ICU http://apps.icu-project.org/icu-jsp/downloadPage.jsp?ver=56.1&base=c&svn=release-56-1
          * Download then extract to top level system folder (/)
* How to run tests
* Deployment instructions

### Contribution guidelines ###

* Writing tests

### Who do I talk to? ###

* John Idarraga 
    * idarraga@amscins.com
* Navrit Bal
    * navrit@amscins.com