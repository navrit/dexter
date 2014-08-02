#ifndef __CPIXEL_PLOTS_H_INCLUDED__
#define __CPIXEL_PLOTS_H_INCLUDED__


#include <vector>
#include "TGraph.h"
#include "TH1F.h"
#include "TH2F.h"
#include "data_holders/Ctel_chunk.h"
#include "TFile.h"
#include "data_holders/Cchip.h"
#include "../src/Chandy.cpp"
#include "../CDQM_options.h"


class Cpixel_plots{
public:
	Ctel_chunk*					_tel;
	CDQM_options * 				_ops;
	std::string					_label;
	std::string					_save_file_name;
	int							_chip_loop_cut;


	//Plots (with their cuts).
	std::vector<TH1F*> 			_ADC_dists;
	double 						_ADC_low;
	double 						_ADC_up;
	int 						_ADC_nbins;

	std::vector<TH1F*> 			_t_dists;
	double 						_t_low; //safe auto.
	double 						_t_up; //safe auto.

	std::vector<TH1I*> 			_occupancies;
	double 						_occupancy_low; //safe auto.
	double 						_occupancy_up; //safe auto.

	TH1F*						_z_distribution;
	bool						_z_distribution_initialized; //handy for singles.
	double 						_z_low; //safe auto.
	double 						_z_up; //safe auto.
	int 						_z_nbins;

	std::vector<TGraph*> 		_t_orders; //safe auto.
	std::vector<TGraph*> 		_x_orders; //safe auto.

	std::vector<TH2F*> 			_hitmaps;
	int							_hitmap_nbins;


	//Member functions --------------------------------------------------------
	Cpixel_plots (CDQM_options *);
	~Cpixel_plots();
	void initialize();
	void execute(Ctel_chunk *);
	void finalize();


	void						init_ADC_dists();
	void						init_t_dists();
	void						init_z_dists();
	void						init_occupancies();
	void						init_hitmaps();

	void						appto_all();
	void 						appto_ADC_dists();
	void 						appto_t_dists();
	void						appto_z_dist();
	void						appto_occupancies();
	void						appto_hitmaps();

	void						make_pixel_t_orders();
	void						make_pixel_x_orders();


	void						save_figs();
	void						set_directories();



	//Setters -----------------------------------------------------------------
	void 						set_ADC_low(double x){_ADC_low = x;}
	void 						set_ADC_up(double x){_ADC_up = x;}
	void 						set_ADC_nbins(int x){_ADC_nbins = x;}
	void 						set_hitmap_nbins(int x){_hitmap_nbins = x;}
	void 						set_z_nbins(int x){_z_nbins = x;}
	void						set_tel(Ctel_chunk* x){_tel = x;}


};

#endif