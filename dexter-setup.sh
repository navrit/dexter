#!/bin/bash
# Author: Navrit Bal
# Company: Amsterdam Scientific Instruments B.V.
# Created: 2019-01-24

function usage() {
  printf '%s\n'   "Last modified: 2019-01-29"
  printf '\n%s' "Usage: "${0##*/}": [-m dev]"
  printf '\n%s'   "Purpose: Prepare a linux machine for using or development of Dexter"

  printf '\n\n\t%s' "-m, --mode [dev]"
  printf '\n\t\t%s' "Choose user only (default) or developer setup"
  printf '\n\t%s'   "-h, --help"
  printf '\n\t\t%s' "Display this message and exit (1)"


  printf '\n\n%s'     "Examples:"
  printf '\n\t%s'     ""${0##*/}"        - user setup (install packages only)"
  printf '\n\t%s\n\n' ""${0##*/}" -m dev - full developer setup"
}

function get_OS() {
  local OS="$(hostnamectl | awk 'BEGIN { FS=":"; } /Operating System: (.*)/ { print $2; }')"

  echo "$OS"
}

function say_something() {
  local OS=$(get_OS)
  local string="$1"

  if [[ $OS == *"Fedora"* ]]; then
    espeak "$string"
  elif [[ $OS == *"Ubuntu"* ]]; then
    spd-say "$string"
  elif [[ $OS == *"CentOS"* ]] || [[ $OS == *"Red Hat Enterprise Linux"* ]]; then
    espeak "$string"
  else
    espeak "$string"
  fi
}

function print_introductory_messages() {
  clear
  echo "[START] -----------------------------------------------------------------"
  say_something "Starting Dexter installation"
  echo "[INFO] Starting Dexter developer setup BASH script - static build - $0 - PID = $$"
}

function handle_mode_option() {
  local mode=$1

  if [[ "$mode" == *"dev"* ]]; then
  	printf '\n%s\n' "[INFO] Developer mode - continuing"
  else
    print_end_messages_user
    exit 0
  fi
}

function get_Qt_URL() {
  local OS=$(get_OS)

  URL_Qt_5_12_0=https://download.qt.io/archive/qt/5.12/5.12.0/single/qt-everywhere-src-5.12.0.tar.xz
  URL_Qt_5_7_1=https://download.qt.io/archive/qt/5.7/5.7.1/single/qt-everywhere-opensource-src-5.7.1.tar.xz

  if [[ $OS == *"Fedora"* ]]; then
    echo "$URL_Qt_5_12_0"
  elif [[ $OS == *"Ubuntu"* ]]; then
    echo "$URL_Qt_5_12_0"
  elif [[ $OS == *"CentOS"* ]] || [[ $OS == *"Red Hat Enterprise Linux"* ]]; then
    echo "$URL_Qt_5_7_1"
  elif [[ $OS == *"Debian"* ]]; then
    exit 1
  else
    echo
    echo "[WARN] Not supported OS... :\("
    exit 1
  fi
}

function get_Qt_file_string() {
  local OS=$(get_OS)

  Qt5_12_0=qt-everywhere-src-5.12.0.tar.xz
  Qt5_7_1=qt-everywhere-opensource-src-5.7.1.tar.xz

  if [[ $OS == *"Fedora"* ]]; then
    echo "$Qt5_12_0"
  elif [[ $OS == *"Ubuntu"* ]]; then
    echo "$Qt5_12_0"
  elif [[ $OS == *"CentOS"* ]] || [[ $OS == *"Red Hat Enterprise Linux"* ]]; then
    echo "$Qt5_7_1"
  elif [[ $OS == *"Debian"* ]]; then
    exit 1
  else
    echo
    echo "[WARN] Not supported OS... :\("
    exit 1
  fi
}

function install_fedora_packages() {
  local OS=$1
  echo "[INFO] Only confirmed for Fedora 29"
  sudo dnf update
  sudo dnf install openblas-devel.x86_64 lapack.x86_64 gcc libusb-devel.x86_64 mesa-libGL-devel libtiff-devel.x86_64 dlib-devel.x86_64 boost-devel.x86_64 cppzmq-devel.x86_64 glib2-devel.x86_64 glibc-devel.x86_64 pulseaudio-libs-devel.x86_64
}

function install_ubuntu_libpng12() {
  local URL_libpng12=http://security.ubuntu.com/ubuntu/pool/main/libp/libpng/libpng12-0_1.2.54-1ubuntu1.1_amd64.deb
  local libpng12_filename=libpng12-0_1.2.54-1ubuntu1.1_amd64.deb

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
}

function install_ubuntu_packages() {
  local OS=$1

  sudo apt update -qy

  # Optional zsh install --> sudo apt install -qy zsh; sh -c "$(curl -fsSL https://raw.githubusercontent.com/robbyrussell/oh-my-zsh/master/tools/install.sh)"; chsh -s /usr/bin/zsh

  # Ubuntu 18.04 or 18.10
  if [[ $OS == *"18"* ]]; then
    sudo apt install -qy git curl make gcc libusb-dev libtiff-dev libzmq5 libzmq3-dev libpng-tools libpcre16-3 libxkbcommon-dev libfontconfig1-dev libgl2ps-dev libgles2-mesa libgl2ps1.4 libx11-xcb-dev mesa-common-dev libgles2-mesa-dev
    install_ubuntu_libpng12
    # Ubuntu 17.04 or 17.10
  elif [[ $OS == *"17"* ]]; then
    sudo apt install -qy git curl libusb-1.0.0-dev libxcb-xinerama0 libopenblas-dev libboost-dev libjasper-dev
    install_ubuntu_libpng12
    # Ubuntu 16.04 or 16.10
  elif [[ $OS == *"16" ]]; then
    sudo apt install -qy git curl libusb-1.0.0-dev libgstreamer0.10-0 libgstreamer-plugins-base0.10-0 libxcb-xinerama0 libopenblas-dev libboost-dev libczmq-dev libzmq3-dev libzmqpp-dev
  fi
}

function install_CentOS_RHEL_packages() {
  local OS=$1
  echo "[INFO] Only confirmed for CentOS 7"
  sudo yum update
  sudo yum install gcc-c++.x86_64 libxcb-devel.i686 qt5-qtbase-static.x86_64 qt5base gstreamer-plugins-base-devel.i686 gstreamer-plugins-base-devel.x86_64 gstreamer-devel.i686 gstreamer-devel.x86_64 boost-devel.i686 blas blas-devel lapack lapack-devel libxcb-devel.i686 libxcb.i686 libusbx-devel
}

function install_debian_packages() {
  echo
  echo "[FAIL] Debian is not supported"
}

function install_phidgets() {
  local URL_Phidgets=https://www.phidgets.com/downloads/phidget21/libraries/linux/libphidget/
  local file_string_Phidgets=libphidget_2.1.8.20170607
  local log_file_Phidgets=libphidget.log

  mkdir -p /tmp/$tmp_working_folder && cd "$_"
  echo "[INFO] Working temporary directory = $(pwd)"

  if [ -s "$file_string_Phidgets" ]; then
    echo "[INFO] Phidgets already downloaded - $file_string_Phidgets exists and is not empty"
  else
    echo "[INFO] Downloading Phidgets - $file_string_Phidgets does not exist, or is empty"
    echo
    wget -N $URL_Phidgets/"$file_string_Phidgets".tar.gz
    echo
    if [ $? -eq 0 ]; then
      echo "[INFO] Finished Phidgets download"
    else
      echo "[FAIL] Could not download: $URL_Phidgets/$file_string_Phidgets.tar.gz"
      exit 1
    fi
  fi

  touch $log_file_Phidgets

  echo "[START INSTALLATION] Date = $(date)" > $log_file_Phidgets

  tar -xf "$file_string_Phidgets.tar.gz" >> $log_file_Phidgets
  if [ $? -eq 0 ]; then
    echo "[INFO] Untarred $file_string_Phidgets.tar.gz"
  else
    echo "[FAIL] Directory = $(pwd)"
    exit 1
  fi

  extracted_file_string=$(echo $file_string_Phidgets | tr _ -)

  cd "/tmp/$tmp_working_folder/$extracted_file_string"
  if [ $? -eq 0 ]; then
    echo "[INFO] Now in $(pwd)"
  else
    echo "[FAIL] Directory = $(pwd)"
    exit 1
  fi

  ./configure >> $log_file_Phidgets
  make -j$(getconf _NPROCESSORS_ONLN) >> $log_file_Phidgets
  sudo make -j$(getconf _NPROCESSORS_ONLN) install >> $log_file_Phidgets
}

function install_dependencies() {
  local OS=$(get_OS)

  echo "[INFO] From hostnamectl "$OS")"
  echo

  if [[ $OS == *"Fedora"* ]]; then
    install_fedora_packages "$OS"
  elif [[ $OS == *"Ubuntu"* ]]; then
    install_ubuntu_packages "$OS"
  elif [[ $OS == *"CentOS"* ]] || [[ $OS == *"Red Hat Enterprise Linux"* ]]; then
    install_CentOS_RHEL_packages "$OS"
  elif [[ $OS == *"Debian"* ]]; then
    install_debian_packages
    exit 1
  else
    echo
    echo "[WARN] Not supported OS... :\("
    exit 1
  fi

  install_phidgets
}

function get_Qt() {
  # Download Qt if it's not there already ----------------------------------------

  cd /tmp/$tmp_working_folder

  if [ -s $Qt_file_string ]; then
    echo "[INFO] Qt already downloaded - $Qt_file_string exists and is not empty"
  else
    echo "[INFO] Downloading Qt - $Qt_file_string does not exist, or is empty"
    echo
    wget -N $URL_Qt
    echo
    if [ $? -eq 0 ]; then
      echo "[INFO] Finished Qt download"
    else
      echo "[FAIL] Could not download: $URL_Qt"
      exit 1
    fi
  fi
}

function extract_Qt() {
  echo "[INFO] Extracting Qt - slow operation"
  tar -xf "$Qt_file_string" --checkpoint=.10000
  echo
  if [ $? -eq 0 ]; then
    echo "[INFO] Done extracting Qt"
  else
    echo "[FAIL] Could not extract the Qt archive: $Qt_file_string"
    exit 1
  fi

  # Extract Qt folder name and change directory to it
  Qt_static_build_folder=${Qt_file_string:0:-7}
  cd /tmp/$tmp_working_folder/$Qt_static_build_folder
}

function verify_Qt() {
  if [ $(ls -l | grep -v ^l | wc -l) > 50 ]; then
    echo "[INFO] Appears to have the roughly correct number of files"
  else
    echo "[WARN] Incorrect number of files? Files = $(ls -l | grep -v ^l | wc -l)"
  fi
}

# Argument: Qt_static_build_folder - where to install the static build of Qt
function make_build_folder_for_static_Qt() {
  mkdir -p ~/$1-static-build
  if [ $? -eq 0 ]; then
    echo "[INFO] Made static build folder: ~/$1"
  else
    echo "[FAIL] Could not create the folder: ~/$1"
    exit 1
  fi
}

function configure_Qt_with_version() {
  version="$1"
  echo "Configuring for Qt $version"

  if [[ $version == "5_12_0" ]]; then
    ./configure -static \
                -prefix ~/$Qt_static_build_folder-static-build \
                -opensource \
                -confirm-license \
                -qt-xcb -qt-pcre -qt-libpng -qt-libjpeg -fontconfig -system-freetype \
                -nomake tests -nomake examples -no-feature-accessibility \
                -skip wayland \
                -skip qt3d -skip qtactiveqt -skip qtandroidextras \
                -skip qtcanvas3d -skip qtconnectivity \
                -skip qtdatavis3d -skip qtdeclarative -skip qtdoc \
                -skip qtgamepad -skip qtgraphicaleffects -skip qtimageformats \
                -skip qtlocation -skip qtmacextras -skip qtmultimedia \
                -skip qtnetworkauth \
                -skip qtpurchasing -skip qtquickcontrols -skip qtquickcontrols2 \
                -skip qtremoteobjects \
                -skip qtscript -skip qtscxml -skip qtsensors -skip qtserialbus \
                -skip qtserialport -skip qtsvg \
                -skip qtspeech \
                -skip qtvirtualkeyboard -skip qtwebchannel -skip qtwebengine \
                -skip qtwebglplugin \
                -skip qtwebview -skip qtwinextras -skip qtxmlpatterns
  elif [[ $version == "5_7_1" ]]; then
    ./configure -static \
                -prefix ~/$Qt_static_build_folder-static-build \
                -opensource \
                -confirm-license \
                -qt-xcb -qt-pcre -qt-libpng -qt-libjpeg -fontconfig \
                -nomake tests -nomake examples \
                -skip qt3d -skip qtactiveqt -skip qtandroidextras \
                -skip qtcanvas3d -skip qtconnectivity \
                -skip qtdatavis3d -skip qtdeclarative -skip qtdoc \
                -skip qtgamepad -skip qtgraphicaleffects -skip qtimageformats \
                -skip qtlocation -skip qtmacextras -skip qtmultimedia \
                -skip qtpurchasing -skip qtquickcontrols -skip qtquickcontrols2 \
                -skip qtscript -skip qtscxml -skip qtsensors -skip qtserialbus \
                -skip qtserialport -skip qtsvg \
                -skip qtvirtualkeyboard -skip qtwebchannel -skip qtwebengine \
                -skip qtwebview -skip qtwinextras -skip qtxmlpatterns
  else
    "[FAIL] Internal script bug - unexpected Qt version supplied = $version"
    exit 1
  fi
}

function configure_Qt() {
  local OS=$(get_OS)
  echo
  echo "[WARN] You need ~20GB free in order for this to expand"
  echo "$(df -h .)"
  echo

  cd /tmp/$tmp_working_folder/$Qt_static_build_folder
  # Could use different compilers: -platform linux-clang, linux-g++
  # Skipping pretty much everything I think it's possible to get away with

  if [[ $OS == *"Fedora"* ]]; then
    configure_Qt_with_version "5_12_0"
  elif [[ $OS == *"Ubuntu"* ]]; then
    configure_Qt_with_version "5_12_0"
  elif [[ $OS == *"CentOS"* ]] || [[ $OS == *"Red Hat Enterprise Linux"* ]]; then
    configure_Qt_with_version "5_7_1"
  else
    echo
    echo "[WARN] Not supported OS..."
    exit 1
  fi

  if [ $? -eq 0 ]; then
    echo "[INFO] Configuration successful"
  else
    echo "[FAIL] Could not configure"
    exit 1
  fi
}

function make_Qt() {
  make -j$(expr $(getconf _NPROCESSORS_ONLN) - 1)
  if [ $? -eq 0 ]; then
    echo "[INFO] Make successful"
  else
    echo "[WARN] Could not make? Make return code = $?"
    # exit 1 ?
  fi
}

function install_Qt() {
  make -j$(expr $(getconf _NPROCESSORS_ONLN) - 1) install
  if [ $? -eq 0 ]; then
    echo "[INFO] make install successful"
  else
    echo "[WARN] Could not make install? Make install return code = $?"
    # exit 1 ?
  fi
}

function print_end_messages_dev() {
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
  say_something "Why are you not working"
}

function print_end_messages_user() {
  printf '\n%s\n' "[INFO] Now the Dexter user setup has completed, you should be able to launch the Dexter executable now"
  printf '\n%s\n' "[END] $(date) -----------------------------------------------------------"
  say_something "Why are you not working"
}

# ------------------------------------------------------------------------------

tmp_working_folder=dexter-dev-setup
Qt_static_build_folder=uninitialised_variable

while [ "$1" != "" ]; do
    case $1 in
        -m | --mode )   mode=$2
                        shift
                        shift
                        ;;
        -h | --help )   usage
                        exit 1
                        ;;
        * )             usage
                        exit 1
                        ;;
    esac
done


print_introductory_messages
install_dependencies
handle_mode_option "$mode"

URL_Qt=$(get_Qt_URL)
Qt_file_string=$(get_Qt_file_string)
get_Qt
extract_Qt
verify_Qt # Note: this is a very weak check as of 2019-01-26
make_build_folder_for_static_Qt "$Qt_static_build_folder"
configure_Qt
make_Qt
install_Qt
print_end_messages_dev

exit 0 # Exit with success
