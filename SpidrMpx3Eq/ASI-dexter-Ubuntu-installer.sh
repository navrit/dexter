#!/bin/bash
echo "\n"
echo "Navrit Bal"
echo "2016-11-16 \n"

echo "\n Installer for ASI Dexter support packages \n"
sudo apt-get update
sudo apt-get -qy install qtbase5-dev
sudo apt-get -qy install qt5-default
sudo apt-get -qy install libqt5opengl5
sudo apt-get -qy install libqt5opengl5-dev
sudo apt-get -qy install qt5-qmake
sudo apt-get -qy install qtbase5-gles-dev
sudo apt-get -qy install libqt5core5a
sudo apt-get -qy install libboost-all-dev
sudo apt-get -qy install libopenblas-dev
sudo apt-get -qy install liblapack-dev
sudo apt-get -qy install python-setuptools
sudo apt-get -qy install python-pip
sudo apt-get -qy install cmake
sudo apt-get -qy install libusb-dev

sudo apt-get autoremove
