//CDQM_options.cpp - a class to contain a set of 
//options for the DQM (histogram cuts, bin numbers etc). This class should be
//edited when running the DQM from the command line. If running from the GUI,
//these parameters can be set in the 'DQM options' tab.
//Author: Dan Saunders
//Date created: 07/01/13
//Last modified: 21/01/13


#ifndef __CDQM_OPTIONS_H_INCLUDED__
#define __CDQM_OPTIONS_H_INCLUDED__

#include <iostream>
#include <string>
#include <vector>

#include "TFile.h"
#include "TH1F.h"
#include <sstream>



class CDQM_options{
public:
	bool presentDUT;
	int eventTrackN;
	double tEventStep;
	int nEvents;
	int PSNumFix;
	int runNumber;
	int nchips;
	int nPixHitCut;
	std::vector<int> algorithms;
	double tcut;
	bool truncate;
	std::string save_file_name;
	std::string save_file_nameLongScale;
	bool replaceLongScalesFile;
	std::string alignment_save_file_name;
	int chip_cut; //produce plots until reaching this cut - useful for tests.
	std::vector<int> ref_chips; //used during correlation plots for comparing two chips.

	bool algorithms_contains(int);
	std::vector<int> _MaskedPixelsX;
	std::vector<int> _MaskedPixelsY;
	std::vector<int> _MaskedPixelsChipID;
	bool maskedPixel(int, int);
	

	//Options to do methods prior to filling the DQM plots (so act on a single 
	//chunk of data). pixel_run specifies old data, and poor_man_align sets chips
	//to have equal positional averages of hitmaps.
	bool remove_hot_pixels;
	int remove_frequency;
	bool load_previous_alignment;
	bool also_order_pixs_x;


	//Pixel hits ______________________________________________________________
	//Parameters for plots made by Cpixel_plots.
	int pix_ToT_nbins;
	int pix_hitmap_nbins; //(sq root of).


	//Ccorrelation_plots (with on-off switch).
	bool align_by_pix_differences; //auto saves to above file.
	int pix_correl_xynbins;
	int pix_correl_tnbins;


	//Separation cuts (in units of pixels).
	int pix_xlow;
	int pix_xup;
	int pix_ylow;
	int pix_yup;
	double pix_tlow;
	double pix_tup;
	double pix_tbglow;
	double pix_tbgup;



	//Clustering ______________________________________________________________
	double clust_maker_tlow;
	double clust_maker_tup;
	double COGweight;
	bool DanCorrectPosn;


	//Ccluster_plots.
	int clust_ToT_nbins;
	int clust_hitmap_nbins; //(sq root of).
	int nsample_clusters;


	//Cluster correlations options (finer cuts).
	bool align_by_clust_differences; //auto saves to above file.
	int clust_correl_xynbins;
	int clust_correl_tnbins;


	//Separation cuts (in units of pixels).
	double clust_xlow;
	double clust_xup;
	double clust_ylow;
	double clust_yup;
	double clust_tlow;
	double clust_tup;
	double clust_tbglow;
	double clust_tbgup;



	//Tracking ________________________________________________________________
	double track_vol_rx;
	double track_vol_thetax;
	double track_vol_ry;
	double track_vol_thetay;
	double track_delt;
	int minNClusterPerTrack;

	int track_vol_shape;
	int track_hitmap_nbins;
	int track_tdist_nbins;
	int track_resolution_nbins;



	//Dumping _________________________________________________________________
	bool dump_tel;
	bool dump_check;




	//Py Plots _________________________________________________________________
	bool include_py_plots;
	double Pytel_drawer_tlower;
	double Pytel_drawer_tupper;



	//Methods _________________________________________________________________
	CDQM_options(int r=0);
	void print_ops();
	void save_ops();
	void load_ops();
};

#endif
