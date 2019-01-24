#!/bin/bash
# Author: Navrit Bal
# Created: 2019-01-24
# Last modified: 2019-01-24
# Purpose: Prepare a linux machine for Dexter development by
#           Download Qt if it's not there already
#           Extract Qt, basic error check
#           Install OS specific packages
#           Configure, make and make install
#           Print ending advice

FILE_STRING=qt-everywhere-src-5.12.0.tar.xz

clear
echo "[START] -----------------------------------------------------------------"
spd-say 'Starting Dexter installation'
echo "[INFO] Starting Dexter developer setup BASH script - static build - $0 - PID = $$"
echo "[INFO] Note: no arguments are ever used"

TMP_FOLDER=temporary-dexter-dev-setup
mkdir -p /tmp/$TMP_FOLDER
cd /tmp/$TMP_FOLDER
echo "[INFO] Working temporary directory = $(pwd)"

# Download Qt if it's not there already ----------------------------------------
FILE_URL=https://download.qt.io/archive/qt/5.12/5.12.0/single/$FILE_STRING
if [ -s "$FILE_STRING" ]; then
   echo "[INFO] Qt already downloaded - $FILE_STRING exists and is not empty"
else
   echo "[INFO] Downloading Qt - $FILE_STRING does not exist, or is empty"
   echo
   wget -N $FILE_URL
   echo
   if [ $? -eq 0 ]; then
      echo "[INFO] Finished Qt download"
   else
      echo "[FAIL] Could not download: $FILE_URL"
      exit 1
   fi
fi

# Extract Qt -------------------------------------------------------------------
echo "[INFO] Extracting Qt - slow operation"
tar -xf $FILE_STRING --checkpoint=.10000
echo
if [ $? -eq 0 ]; then
    echo "[INFO] Done extracting Qt"
else
    echo "[FAIL] Could not extract the Qt archive: $FILE_STRING"
    exit 1
fi

# Extract Qt folder name and change directory to it
Qt_static_build_folder=${FILE_STRING:0:-7}
cd /tmp/$TMP_FOLDER/$Qt_static_build_folder

# Basic error check ------------------------------------------------------------
if [ `ls -l | grep -v ^l | wc -l` -eq 55 ]; then
    echo "[INFO] Correct number of files"
else
    echo "[WARN] Incorrect number of files? Files = $(ls -l | grep -v ^l | wc -l)"
fi

# Make installation folder for the static build of Qt
mkdir -p ~/$Qt_static_build_folder
if [ $? -eq 0 ]; then
    echo "[INFO] Made static build folder: ~/$Qt_static_build_folder"
else
    echo "[FAIL] Could not create the folder: ~/$Qt_static_build_folder"
    exit 1
fi

# Install OS specific packages -------------------------------------------------
OS="$(hostnamectl | grep "Operating System: *")"
echo "[INFO] From hostnamectl $OS)"
echo
if [[ $OS == *"Fedora"* ]]; then
    sudo dnf update
    sudo dnf install openblas-devel.x86_64 lapack.x86_64 gcc libusb-devel.x86_64 mesa-libGL-devel libtiff-devel.x86_64 dlib-devel.x86_64 boost-devel.x86_64 cppzmq-devel.x86_64 glib2-devel.x86_64 glibc-devel.x86_64 pulseaudio-libs-devel.x86_64

elif [[ $OS == *"Ubuntu"* ]]; then
    URL_libpng12=http://security.ubuntu.com/ubuntu/pool/main/libp/libpng/libpng12-0_1.2.54-1ubuntu1.1_amd64.deb
    libpng12_filename=libpng12-0_1.2.54-1ubuntu1.1_amd64.deb

    sudo apt update -qy

    # Optional zsh install --> sudo apt install -qy zsh; sh -c "$(curl -fsSL https://raw.githubusercontent.com/robbyrussell/oh-my-zsh/master/tools/install.sh)"; chsh -s /usr/bin/zsh

    # Ubuntu 18.04 or 18.10
    if [[ $OS == *"18"* ]]; then
        sudo apt install -qy make curl gcc libusb-dev libzmq5 libpng-tools libpcre16-3 libxkbcommon-dev
        wget -N $URL_libpng12
        if [ $? -eq 0 ]; then
            echo
        else
            echo "[FAIL] Could not download libpng12-0 from $URL_libpng12"
            exit 1
        fi
        sudo dpkg -i $libpng12_filename
        if [ $? -eq 0 ]; then
            echo
        else
            echo "[FAIL] Could not install $libpng12_filename with sudo dpkg -i"
            exit 1
        fi
        sudo apt install -qyf
    # Ubuntu 17.04 or 17.10
    elif [[ $OS == *"17"* ]]; then
        sudo apt install -qy git curl libusb-1.0.0-dev libxcb-xinerama0 libopenblas-dev libboost-dev libjasper-dev
        wget -N $URL_libpng12
        if [ $? -eq 0 ]; then
            echo
        else
            echo "[FAIL] Could not download libpng12-0 from $URL_libpng12"
            exit 1
        fi
        sudo dpkg -i $libpng12_filename
        if [ $? -eq 0 ]; then
            echo
        else
            echo "[FAIL] Could not install $libpng12_filename with sudo dpkg -i"
            exit 1
        fi
        sudo apt install -qyf
    # Ubuntu 16.04 or 16.10
    elif [[ $OS == *"16" ]]; then
        sudo apt install -qy git curl libusb-1.0.0-dev libgstreamer0.10-0 libgstreamer-plugins-base0.10-0 libxcb-xinerama0 libopenblas-dev libboost-dev libczmq-dev libzmq3-dev libzmqpp-dev
    fi

    PHIDGETS_URL=https://www.phidgets.com/downloads/phidget21/libraries/linux/libphidget/libphidget_2.1.8.20151217.tar.gz
    PHIDGETS_FILE_STRING=libphidget_2.1.8.20151217
    if [ -s "$PHIDGETS_FILE_STRING" ]; then
       echo "[INFO] Phidgets already downloaded - $PHIDGETS_FILE_STRING exists and is not empty"
    else
       echo "[INFO] Downloading Phidgets - $PHIDGETS_FILE_STRING does not exist, or is empty"
       echo
       wget -N $PHIDGETS_URL
       echo
       if [ $? -eq 0 ]; then
          echo "[INFO] Finished Phidgets download"
       else
          echo "[FAIL] Could not download: $PHIDGETS_URL"
          exit 1
       fi
    fi

    phidgets_log_file=libphidget.log
    touch $phidgets_log_file
    tar xf libphidget_2.1.8.20151217.tar.gz > $phidgets_log_file
    cd libphidget-2.1.8.20151217
    ./configure >> $phidgets_log_file
    make -j$(getconf _NPROCESSORS_ONLN) >> $phidgets_log_file
    sudo make -j$(getconf _NPROCESSORS_ONLN) install >> $phidgets_log_file

elif [[ $OS == *"CentOS"* || $OS == *"Red Hat Enterprise Linux"* ]]; then
    sudo yum update
    sudo yum install libxcb-devel.i686 qt5-qtbase-static.x86_64 qt5base gstreamer-plugins-base-devel.i686 gstreamer-plugins-base-devel.x86_64 gstreamer-devel.i686 gstreamer-devel.x86_64 boost-devel.i686 blas blas-devel lapack lapack-devel libxcb-devel.i686 libxcb.i686

elif [[ $OS == *"Debian"* ]]; then
    echo
    echo "[FAIL] Debian is not supported"
    exit 1
else
    echo
    echo "[WARN] Not supported OS... :\("
fi

# Configure, make and make install ---------------------------------------------
echo
echo "[WARN] You need ~20GB free in order for this to expand"
echo "$(df -h .)"
echo

cd /tmp/$TMP_FOLDER/$Qt_static_build_folder
./configure -static -prefix ~/$Qt_static_build_folder-static-build -opensource -confirm-license -qt-xcb -fontconfig -qt-pcre -qt-libpng -qt-libjpeg -nomake tests -nomake examples -skip qtwebview -skip qtserialbus -skip qtpurchasing -skip qtlocation -skip qtdoc -skip qtandroidextras -skip qtgamepad
if [ $? -eq 0 ]; then
    echo "[INFO] Configuration successful"
else
    echo "[FAIL] Could not configure"
    exit 1
fi

make -j$(getconf _NPROCESSORS_ONLN)
if [ $? -eq 0 ]; then
    echo "[INFO] Make successful"
else
    echo "[FAIL] Could not make"
    exit 1
fi

make -j$(getconf _NPROCESSORS_ONLN) install
if [ $? -eq 0 ]; then
    echo "[INFO] make install successful"
else
    echo "[FAIL] Could not make install"
    exit 1
fi

# Print ending advice ----------------------------------------------------------
echo
echo "[INFO] Now you have a static build of Qt, note everything is hard-coded so do not move the folder"
echo "[INFO] In QtCreator add a new kit with the new qmake (from ~/$Qt_static_build_folder-static-build)"
echo "[INFO] Tools -> Options -> Build & Run -> Qt Versions  and  Tools -> Options -> Build & Run -> Kits"
echo "[INFO] Under Projects, Build & Run, select Add Kit and choose the new kit"
echo
echo "[INFO] Tip: add Add -j\$(getconf _NPROCESSORS_ONLN) to the Make arguments"
echo "[INFO] Tip: Use Release mode unless you need the debugger"
echo
echo "[END] $(date) -----------------------------------------------------------"
spd-say 'Get back to work'
# Exit with success
exit 0
