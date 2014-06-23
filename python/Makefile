#DEBUG=1
ifdef DEBUG
PYTHON_INCLUDEDIR=$(shell  python2.7-dbg-config --cflags)
LDFLAGS=-shared -Wl,-rpath=$(PWD)/../Debug  -L../Debug
CFLAGS=-fPIC -ggdb  ${PYTHON_INCLUDEDIR} 
else
PYTHON_INCLUDEDIR=$(shell  python2.7-config --cflags)
LDFLAGS=-shared -Wl,-rpath=$(PWD)/../Release  -L../Release
CFLAGS=-fPIC   ${PYTHON_INCLUDEDIR} 
endif
TMP_DIR=tmp
SRC_DIR=SpidrTpx3
#-ggdb -I/usr/lib/debug/usr/lib/python2.7/dist-packages/numpy/core
#/usr/lib/python2.7/dist-packages/numpy/core/include/
CXX=g++

$(PWD) := $(shell pwd)

#-L/usr/lib/python2.7/dist-packages/numpy/core/

all: $(TMP_DIR)/SpidrTpx3_engine.so SpidrTpx3/tpx3.so

$(TMP_DIR)/udp_server.o:$(SRC_DIR)/udp_server.cpp
	$(CXX) $(CFLAGS) -c $< -o $@

PYTHON_PARAMS=
ifdef CERN_PROBESTATION
PYTHON_PARAMS=" CERN_PROBESTATION"
endif

$(TMP_DIR)/SpidrTpx3_engine.c:$(SRC_DIR)/SpidrTpx3_engine_bind.py
	PYTHONPATH=$(PWD) python  $<  $(PYTHON_PARAMS) > $@

$(TMP_DIR)/SpidrTpx3_engine.o:$(TMP_DIR)/SpidrTpx3_engine.c
	$(CXX) $(CFLAGS) -c $< -o $@

	
	
$(TMP_DIR)/SpidrTpx3_engine.so:$(TMP_DIR)/SpidrTpx3_engine.o $(TMP_DIR)/udp_server.o

	$(CXX) $(LDFLAGS)  -o $(TMP_DIR)/SpidrTpx3_engine.so $(TMP_DIR)/SpidrTpx3_engine.o $(TMP_DIR)/udp_server.o -lSpidrTpx3Lib -lboost_thread -lboost_serialization -lboost_iostreams -lboost_filesystem -lboost_system

# -lboost_thread -lboost_serialization -lboost_iostreams -lboost_filesystem -lboost_system

.PHONY: subdirs


SpidrTpx3/tpx3.so:SpidrTpx3/tpx3.o
	$(CXX) -shared $< -o $@
SpidrTpx3/tpx3.o:SpidrTpx3/tpx3.c
	$(CXX) -c $(CFLAGS) $< -o $@
SpidrTpx3/tpx3.c: SpidrTpx3/tpx3.py
	cython $<

ui/tpx3.so:ui/tpx3.py
	cython $<
	$(CXX) -c $(CFLAGS) ui/tpx3.c -o ui/tpx3.so
	$(CXX) -shared ui/tpx3.so -o $@

ui/PixBitmap.so:ui/PixBitmap.py
	cython $<
	$(CXX) -c $(CFLAGS) ui/PixBitmap.c -o ui/PixBitmap.o
	$(CXX) -shared ui/PixBitmap.o -o $@



gui: ui/MainWnd.ui
	pyside-uic ui/MainWnd.ui -o ui/MainWnd.py
	pyside-uic ui/BitmapForm.ui -o ui/BitmapForm.py
	pyside-uic ui/EqualizeForm.ui -o ui/EqualizeForm.py
	pyside-uic ui/AboutWnd.ui -o ui/AboutWnd.py
	pyside-uic ui/SPIDRAboutDlg.ui -o ui/SPIDRAboutDlg.py
	pyside-uic ui/SPIDRSettingsDlg.ui -o ui/SPIDRSettingsDlg.py
	pyside-uic ui/AboutWnd.ui -o ui/AboutWnd.py
	pyside-rcc ui/resources.qrc -o ui/resources_rc.py

gui_fast:ui/tpx3.so ui/PixBitmap.so
	

ls clean:
	@rm -rf tmp/* SpidrTpx3/tpx3.o SpidrTpx3/tpx3.c SpidrTpx3/tpx3.so
	@find . -name '*.pyc' -exec rm -f {} \;
	
fresh:
	find . -name '*~' -exec rm -f {} \;
