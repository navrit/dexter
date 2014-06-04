PYTHON_INCLUDEDIR=$(shell pkg-config --cflags  python2)
TMP_DIR=tmp
SRC_DIR=SpidrTpx3
CFLAGS=-fPIC  ${PYTHON_INCLUDEDIR}
CXX=g++
$(PWD) := $(shell pwd)
LDFLAGS=-shared -Wl,-rpath=$(PWD)/../Release

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

	$(CXX) $(LDFLAGS) -L../Release -o $(TMP_DIR)/SpidrTpx3_engine.so $(TMP_DIR)/SpidrTpx3_engine.o $(TMP_DIR)/udp_server.o -lSpidrTpx3Lib -lboost_thread -lboost_serialization -lboost_iostreams -lboost_filesystem -lboost_system

# -lboost_thread -lboost_serialization -lboost_iostreams -lboost_filesystem -lboost_system

.PHONY: subdirs


SpidrTpx3/tpx3.so:SpidrTpx3/tpx3.o
	$(CXX) -shared $< -o $@

SpidrTpx3/tpx3.o:SpidrTpx3/tpx3.c
	$(CXX) -c $(CFLAGS) $< -o $@

SpidrTpx3/tpx3.c: SpidrTpx3/tpx3.py
	cython $<


ls clean:
	@rm -rf tmp/* SpidrTpx3/tpx3.o SpidrTpx3/tpx3.c SpidrTpx3/tpx3.so
	@find . -name '*.pyc' -exec rm -f {} \;
	
fresh:
	find . -name '*~' -exec rm -f {} \;
