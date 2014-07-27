//CDQM.cpp. This simple class (no header needed) runs the DQM. A set of options 
//must be provided to the constructor either via the command line main.cpp 
//function, or via the GUI. 

//**Note that users should never need to edit this script, rather the input 
//options only.

//Author: Dan Saunders
//Date created: 07/01/13
//Last modified: 21/01/13


#ifndef __CDQM_H_INCLUDED__
#define __CDQM_H_INCLUDED__

#include <fstream>
#include <iostream>
#include <vector>
#include <sstream>

#include "../src/Chandy.cpp"
#include "../CDQM_options.h"
#include "Cold_tel_getter.h"
#include "CPS_tel_getter.h"
#include "Ccorrelation_plots.h"
#include "Ccorrel_line_finder.h"
#include "Ccorrel_aligner.h"
#include "Cpoor_mans_aligner.h"
#include "Cpixel_plots.h"
#include "Cchip_plots.h"
#include "Ccluster_plots.h"
#include "data_holders/Ctel_chunk.h"
#include "Ccluster_maker.h"
#include "Ctrack_maker.h"
#include "Ctrack_plots.h"
#include "CCOG_fitter.h"

#include "TH1F.h"
#include "TH2F.h"
#include "TFile.h"
#include "TError.h"
#include "TDirectory.h"
#include "TBrowser.h"


class CDQM{
public:	
	clock_t								_t1;
	std::vector<int>					_algorithms;
	CDQM_options * 						_ops;
	Ctel_chunk * 						_tel;
	int									_sampleNum;


	// Possible algorithm pointers.
	Cold_tel_getter *					_old_tel_getter;
	CPS_tel_getter *					_PS_tel_getter;

	Cpixel_plots *						_pixel_plots;
	Ccorrelation_plots * 				_x_pix_correlations;
	Ccorrelation_plots * 				_y_pix_correlations;
	Ccorrelation_plots *			 	_t_pix_correlations;

	Ccluster_maker * 					_cluster_maker;

	Ccluster_plots *					_cluster_plots;
	Ccorrelation_plots * 				_x_clust_correlations;
	Ccorrelation_plots * 				_y_clust_correlations;
	Ccorrelation_plots * 				_t_clust_correlations;

	Cchip_plots *						_chip_plots;

	Ctrack_maker * 						_track_maker;
	Ctrack_plots *						_track_plots;

	CCOG_fitter * 						_cog_fitter;

	// Ccorrel_aligner * 					_aligner;




	//Methods _________________________________________________________________
	CDQM(CDQM_options*); 
	//~CDQM();
	void 								make_new_save_file();
	void								include_py_plots();

	void initialize();
	void execute(double);
	void executeEventLoop(double);
	void finalize();

	void _prepareLongScalePlots();
	void _appendLongScalePlots(double);


};
#endif
