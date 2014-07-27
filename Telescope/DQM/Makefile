#compiler.
CC = g++
CFLAGS=-c -Wall  -std=c++0x `root-config --cflags --libs`

all: runDQM 

runDQM: main.o Ctel_chunk.o Cchip.o Ccluster.o Ctrack.o Cpix_hit.o Chandy.o \
	Cold_tel_getter.o CPS_tel_getter.o Ccorrelation_plots.o Ccorrel_line_finder.o \
	Ccorrel_aligner.o Cpixel_plots.o Cpoor_mans_aligner.o CDQM_options.o \
	CDQM.o Ccluster_maker.o Ccluster_plots.o Cchip_plots.o Ctrack_volume.o Ctrack_maker.o \
	Ctrack_plots.o CCOG_fitter.o 

	$(CC) main.o Ctel_chunk.o Cchip.o Ccluster.o Ctrack.o Cpix_hit.o Chandy.o  \
	Cold_tel_getter.o CPS_tel_getter.o Ccorrelation_plots.o Ccorrel_line_finder.o CDQM.o \
	Ccorrel_aligner.o Cpixel_plots.o Cpoor_mans_aligner.o CDQM_options.o Ccluster_maker.o \
	Ccluster_plots.o Cchip_plots.o Ctrack_volume.o Ctrack_maker.o Ctrack_plots.o CCOG_fitter.o -o \
	runDQM `root-config --cflags --libs` 
	@echo "\n\nDone."


main.o: main.cpp 
	$(CC) $(CFLAGS) main.cpp

CDQM.o: src/CDQM.cpp 
	$(CC) $(CFLAGS) src/CDQM.cpp

CCOG_fitter.o: src/CCOG_fitter.cpp 
	$(CC) $(CFLAGS) src/CCOG_fitter.cpp

Chandy.o: src/Chandy.cpp
	$(CC) $(CFLAGS) src/Chandy.cpp

Cchip_plots.o: src/Cchip_plots.cpp
	$(CC) $(CFLAGS) src/Cchip_plots.cpp

Ctrack_plots.o: src/Ctrack_plots.cpp
	$(CC) $(CFLAGS) src/Ctrack_plots.cpp

CDQM_options.o: CDQM_options.cpp
	$(CC) $(CFLAGS) CDQM_options.cpp
		
Cold_tel_getter.o: src/Cold_tel_getter.cpp
	$(CC) $(CFLAGS) src/Cold_tel_getter.cpp

CPS_tel_getter.o: src/CPS_tel_getter.cpp
	$(CC) $(CFLAGS) src/CPS_tel_getter.cpp

Ccluster_maker.o: src/Ccluster_maker.cpp
	$(CC) $(CFLAGS) src/Ccluster_maker.cpp
	
Ccorrelation_plots.o: src/Ccorrelation_plots.cpp 
	$(CC) $(CFLAGS) src/Ccorrelation_plots.cpp
	
Ccorrel_line_finder.o: src/Ccorrel_line_finder.cpp 
	$(CC) $(CFLAGS) src/Ccorrel_line_finder.cpp

Ccorrel_aligner.o: src/Ccorrel_aligner.cpp 
	$(CC) $(CFLAGS) src/Ccorrel_aligner.cpp

Cpoor_mans_aligner.o: src/Cpoor_mans_aligner.cpp 
	$(CC) $(CFLAGS) src/Cpoor_mans_aligner.cpp

Cpixel_plots.o: src/Cpixel_plots.cpp 
	$(CC) $(CFLAGS) src/Cpixel_plots.cpp

Ctrack_maker.o: src/Ctrack_maker.cpp 
	$(CC) $(CFLAGS) src/Ctrack_maker.cpp

Ccluster_plots.o: src/Ccluster_plots.cpp 
	$(CC) $(CFLAGS) src/Ccluster_plots.cpp





#data holders ----------------------------------------------------------------#
	
Ctel_chunk.o: src/data_holders/Ctel_chunk.cpp 
	$(CC) $(CFLAGS) src/data_holders/Ctel_chunk.cpp

Ctrack_volume.o: src/data_holders/Ctrack_volume.cpp 
	$(CC) $(CFLAGS) src/data_holders/Ctrack_volume.cpp
	
Cchip.o: src/data_holders/Cchip.cpp
	$(CC) $(CFLAGS) src/data_holders/Cchip.cpp
	
Ccluster.o: src/data_holders/Ccluster.cpp
	$(CC) $(CFLAGS) src/data_holders/Ccluster.cpp
	
Ctrack.o: src/data_holders/Ctrack.cpp
	$(CC) $(CFLAGS) src/data_holders/Ctrack.cpp

Cpix_hit.o: src/data_holders/Cpix_hit.cpp
	$(CC) $(CFLAGS) src/data_holders/Cpix_hit.cpp
	

$(OBJDIR):
	mkdir $(OBJDIR)


clean:
	rm -rf *o runDQM
	@echo "\n\nCleaned."
