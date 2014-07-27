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


class CDQM_options{
public:
	double tEventStep;
	int nEvents;
	int PSNumFix;
	int runNumber;
	int nchips;
	int nPixHitCut;
	std::vector<int> algorithms;
	float tcut;
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
	float pix_tlow;
	float pix_tup;
	float pix_tbglow;
	float pix_tbgup;



	//Clustering ______________________________________________________________
	float clust_maker_tlow;
	float clust_maker_tup;
	float COGweight;
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
	float clust_xlow;
	float clust_xup;
	float clust_ylow;
	float clust_yup;
	float clust_tlow;
	float clust_tup;
	float clust_tbglow;
	float clust_tbgup;



	//Tracking ________________________________________________________________
	float track_vol_r;
	float track_vol_theta;
	float track_delt;
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
	float Pytel_drawer_tlower;
	float Pytel_drawer_tupper;



	//Methods _________________________________________________________________
	CDQM_options();
	void print_ops();
	void save_ops();
	void load_ops();
};

#endif
