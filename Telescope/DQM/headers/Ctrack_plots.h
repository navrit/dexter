#ifndef __Ctrack_plots_H_INCLUDED__
#define __Ctrack_plots_H_INCLUDED__


#include <vector>
#include "TGraph.h"
#include "TH1F.h"
#include "TH2F.h"
#include "TF1.h"
#include "data_holders/Ctel_chunk.h"
#include "TFile.h"
#include "data_holders/Cchip.h"
#include "../src/Chandy.cpp"
#include "../CDQM_options.h"



class Ctrack_plots{
public:
	Ctel_chunk*					_tel;
	std::string					_save_file_name;
	std::vector<int>			_ref_chips;
	CDQM_options*				_ops;
	float						_tcut;


	TH2F*						_track_hitmap;
	int							_nhitmap_bins;
	TH1F*						_t_dist;
	TH1F*						_mx_dist;
	TH1F*						_my_dist;
	TH1F*						_cx_dist;
	TH1F*						_cy_dist;
	int							_tdist_nbins;


	std::vector<TH2F*>			_trackedClusters;
	std::vector<TH2F*>			_nonTrackedClusters;

	TH1F*						_size_dist;

	float 						_delr;
	float 						_delt;
	float						_ADC_low;
	float						_ADC_high;

	std::vector<TH1F*>			_xresiduals;
	std::vector<TH1F*>			_xresiduals0;
	std::vector<TH1F*>			_xresiduals1;
	std::vector<TH1F*>			_yresiduals;
	std::vector<TH1F*>			_tresiduals;

	//Member functions --------------------------------------------------------
	Ctrack_plots (CDQM_options*);
	~Ctrack_plots();

	void show_residuals();
	void get_GaussResidual(int, int, float &, float &);

	void initialize();
	void execute(Ctel_chunk*);
	void finalize();

	void init_track_dists();
	void init_residuals();
	void appto_all();
	void appto_residuals(int);
	void init_hitmaps();
	void appto_track_dists();
	void make_t_dist();
	void set_directories();
	void save_figs();
	void appto_TrackCluster_plots(); 



	//Setters -----------------------------------------------------------------
	void						set_tel(Ctel_chunk* x){_tel = x;}
	void						set_tdist_nbins(int i){_tdist_nbins = i;}
	void						set_nhitmap_bins(int i){_nhitmap_bins = i;}


};

#endif