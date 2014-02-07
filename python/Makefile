TMP_DIR=tmp
SRC_DIR=SpidrTpx3
CFLAGS=-fPIC -I/usr/include/python2.7
CXX=g++
$(PWD) := $(shell pwd)
LDFLAGS=-shared -Wl,-rpath=$(PWD)/../Release

all: $(TMP_DIR)/SpidrTpx3_engine.so subdirs

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

subdirs:
	make -C SpidrTpx3
	make -C tests

ls clean:
	rm -rf tmp/*
	rm $(TMP_DIR)/SpidrTpx3_engine.c
	find . -name '*.pyc' -exec rm -f {} \;
	
fresh:
	find . -name '*~' -exec rm -f {} \;
