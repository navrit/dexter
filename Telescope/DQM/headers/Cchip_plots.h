#ifndef __Cchip_plots_H_INCLUDED__
#define __Cchip_plots_H_INCLUDED__


#include <vector>
#include "TGraph.h"
#include "TH1F.h"
#include "TH2F.h"
#include "data_holders/Ctel_chunk.h"
#include "TFile.h"
#include "data_holders/Cchip.h"
#include "../src/Chandy.cpp"
#include "sstream"
#include "../CDQM_options.h"


class Cchip_plots{
private:
	Ctel_chunk*					_tel;
	std::string					_save_file_name;
	int							_chip_loop_cut;
	CDQM_options*				_ops;


	//Plots (with their cuts) samples REPLACE.
	std::vector<TGraph*> 		_pix_t_to_IDs;
	std::vector<TGraph*> 		_pix_x_to_IDs;
	std::vector<TGraph*> 		_clust_t_to_IDs;
	std::vector<TGraph*> 		_clust_x_to_IDs;

public:
	//Member functions --------------------------------------------------------
	Cchip_plots (CDQM_options*);
	~Cchip_plots();

	void initialize();
	void execute(Ctel_chunk*);
	void finalize();

	void						plot_cluster_t_to_ID();
	void						plot_cluster_x_to_ID();

	void						plot_pix_t_to_ID();
	void						plot_pix_x_to_ID();

	void						plot_all();

	void						save_figs();
	void						set_directories();


};

#endif