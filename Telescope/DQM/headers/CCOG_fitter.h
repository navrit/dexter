#ifndef __CCOG_fitter_H_INCLUDED__
#define __CCOG_fitter_H_INCLUDED__


#include <vector>
#include "data_holders/Cchip.h"
#include "data_holders/Ccluster.h"
#include "data_holders/Cpix_hit.h"
#include "data_holders/Ctrack.h"
#include "data_holders/Ctrack_volume.h"
#include "data_holders/Ctel_chunk.h"
#include "Ctrack_plots.h"
#include "../CDQM_options.h"
#include "string.h"
#include "TH1F.h"
#include "TFile.h"
#include "TGraphErrors.h"


class CCOG_fitter{
private:
	Ctel_chunk*					_tel;
	int							_chip_loop_cut;
	std::string					_save_file_name;
	CDQM_options * 				_ops;
	int							_SampleNum;

	float						_alpha_low;
	float						_alpha_high;
	int							_nalphas;

	float						_alpha_step;

	std::vector<TGraph*>	_gxs;
	std::vector<TGraph*>	_gys;

	std::vector<std::vector< float > > _xrs; // By alpha, then by chip.
	std::vector<std::vector< float > > _del_xrs;

	std::vector<std::vector< float > > _yrs; // By alpha, then by chip.
	std::vector<std::vector< float > > _del_yrs;


public:
	//Member functions --------------------------------------------------------
	CCOG_fitter(CDQM_options*);
	~CCOG_fitter();
	void initialize();
	void execute(Ctel_chunk*);
	void finalize();

	void plot_method();
	void fill_plot_data();

	void set_cluster_positions(float);
	void set_directories();
	void save_figs();

};

#endif