#ifndef __CCLUSTER_PLOTS_H_INCLUDED__
#define __CCLUSTER_PLOTS_H_INCLUDED__


#include <vector>
#include "TGraph.h"
#include "TH1F.h"
#include "TH2F.h"
#include "data_holders/Ctel_chunk.h"
#include "TFile.h"
#include "data_holders/Cchip.h"
#include "../src/Chandy.cpp"
#include "../CDQM_options.h"


class Ccluster_plots{
public:
	CDQM_options * 				_ops;
	Ctel_chunk *				_tel;
	std::string					_label;
	std::string					_save_file_name;
	int							_chip_loop_cut;
	int							_ref_chip;
	int							_SampleNum;


	//Plots (with their cuts) samples APPEND to.
	std::vector<TH1F*> 			_ToT_dists;
	std::vector<TH1F*> 			_ToT_Size1_dists;
	std::vector<TH1F*> 			_ToT_Size2_dists;
	std::vector<TH1F*> 			_ToT_Size3_dists;
	std::vector<TH1F*> 			_ToT_Size4_dists;
	double 						_ToT_low;
	double 						_ToT_up;
	int 						_ToT_nbins;

	std::vector<TH1F*> 			_t_dists;
	double 						_t_low; //safe auto.
	double 						_t_up; //safe auto.
	int							_tdist_nbins;

	std::vector<TH1F*> 			_size_dists;
	double 						_size_low; //safe auto.
	double 						_size_up; //safe auto.
	int 						_size_nbins;

	TH1F *						_z_distribution;
	TH2F*						_zVsX_distribution;
	TH2F*						_zVsY_distribution;
	bool						_z_distributions_initialized; //handy for singles.
	double 						_z_low; 
	double 						_z_up; 
	int 						_z_nbins;


	std::vector<TH2F*> 			_hitmapsGlobal;
	std::vector<TH2F*> 			_hitmapsLocal;
	std::vector<TH1F*> 			_hitmapsGlobalXDist;
	std::vector<TH1F*> 			_hitmapsGlobalYDist;
	std::vector<TH2F*> 			_sampleHitmaps;
	int							_hitmap_nbins;


	std::vector<TH1F*> 			_DCorrectChis;
	std::vector<TH1F*> 			_PredictedLeftShare;
	std::vector<TH1F*> 			_PredictedRightShare;
	std::vector<TH1F*> 			_ActualRightShare;
	std::vector<TH1F*> 			_ActualLeftShare;

	std::vector<TH1F*> 			_DiffLeftShare;
	std::vector<TH1F*> 			_DiffRightShare;

	std::vector<TH1F*> 			_pixDeltaTs;
	std::vector<TH2F*> 			_pixDeltaTsVsCharge;




	//Plots (with their cuts) samples REPLACE.
	std::vector<TGraph*> 		_t_orders; //safe auto.
	std::vector<TGraph*> 		_x_orders; //safe auto.

	std::vector<TH2F*>			_cluster_samples; 
	int							_ncluster_samples; //square number nice.
	int							_root_ncluster_samples;
	int							_sample_spacing;


	//Member functions --------------------------------------------------------
	Ccluster_plots (CDQM_options*);
	~Ccluster_plots();
	void initialize();
	void execute(Ctel_chunk*);
	void finalize();

	void						init_sharers();
	void						fill_cluster_sample_h(TH2F*, std::vector<Ccluster*> &);
	bool						cluster_close_enough(double, double, double);

	//Appendable plots (need to be init).
	void						init_all();
	void						init_ToT_dists();
	void						init_t_dists();
	void						init_size_dists();
	void						init_z_dists();
	void						init_hitmaps();

	void						appto_sharers();
	void						appto_all();
	void 						appto_ToT_dists();
	void 						appto_t_dists();
	void 						appto_size_dists();
	void						appto_z_dists();
	void						appto_hitmaps();
	void						appto_sampleHitmaps();
	void						init_pixDeltaTs();
	void						appto_pixDeltaTs();


	//Replaceable plots (not init).
	void						make_cluster_t_orders();
	void						make_cluster_x_orders();
	void						make_cluster_samples();


	void						save_figs();
	void						set_directories();
};

#endif
