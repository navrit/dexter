#ifndef __CCORRELATION_PLOTS_H_INCLUDED__
#define __CCORRELATION_PLOTS_H_INCLUDED__


#include <vector>
#include <time.h>
#include <stdlib.h> 
#include <iostream>
#include <sstream>
#include "time.h"

#include "../src/Chandy.cpp"
#include "../CDQM_options.h"
#include "data_holders/Ctel_chunk.h"
#include "data_holders/Cchip.h"

#include "TCanvas.h"
#include "TGraph.h"
#include "TH1F.h"
#include "TH2F.h"
#include "TFile.h"
#include "TStyle.h"
#include "TColor.h"
#include "TExec.h"
#include "TCanvas.h"
#include "TMatrix.h"
#include "TF1.h"



class Ccorrelation_plots{
public:
	Ctel_chunk *			_tel;
	CDQM_options * 			_ops;
	int						_dir;
	int 					_chip_loop_cut; //save time debugging.

	//This class is general enough to use on pixels or clusters.
	std::string				_pixs_or_clusts; //one of ["pixs", "clusts"].
	std::string				_save_file_name;


	//Reference chip attributes.
	float					_posn_xmin;
	float					_posn_xmax;
	float					_t_xmin;
	float					_t_xmax;
	int						_ref_chipID;

	float					_clust_diff_xmin;
	float					_clust_diff_xmax;

	float					_pix_diff_xmin;
	float					_pix_diff_xmax;

	float					_clust_diff_tmin;
	float					_clust_diff_tmax;

	float					_pix_diff_tmin;
	float					_pix_diff_tmax;

	//General bounds on the plots.
	float 					_xup_cut;
	float 					_xlow_cut;

	float 					_yup_cut;
	float 					_ylow_cut;

	float 					_tup_cut; //t cuts are on delt, which is t - ref_t.
	float 					_tlow_cut;

	float 					_tbgup_cut;
	float 					_tbglow_cut;


	//Plots themselves.
	int 					_nbins;
	std::vector<TH2F*> 		_correlationsLocal;
	std::vector<TH2F*> 		_correlation_backgroundsLocal;
	std::vector<TH2F*> 		_correlations_backgroundlessLocal;
	std::vector<TH1F*> 		_differencesLocal;
	std::vector<TH1F*> 		_differences_backgroundlessLocal;

	std::vector<TH2F*> 		_correlationsGlobal;
	std::vector<TH2F*> 		_correlation_backgroundsGlobal;
	std::vector<TH2F*> 		_correlations_backgroundlessGlobal;
	std::vector<TH1F*> 		_differencesGlobal;
	std::vector<TH1F*> 		_differences_backgroundlessGlobal;

	std::vector<double>		_variance_vector;
	std::vector<double>		_res_vector;


	Ccorrelation_plots(CDQM_options*, std::string, int);
	~Ccorrelation_plots();
	void initialize();
	void execute(Ctel_chunk*);
	void finalize();
 

	void 					save_figs();
	void					tag_to_chips();
	void					set_directories();
	void					poor_mans_resolution();
	void					fill_variance_vector();
	void					fill_res_vector();



	//Graph starters and appenders --------------------------------------------
	void 					init_correlations();
	void 					init_correlation_backgrounds();
	void					init_correlations_minus_backgrounds();
	TH2F*					get_empty_correlation(int, std::string, bool);
	TH1F*					get_empty_differences(int, std::string, bool);


	void					appto_correlation(int, int);
	void					appto_time_correlation(int, int);
	void 					appto_correlations();

	void 					correlation_fill(int ichip, float*, float*, float*, float*);
	void 					background_fill(int ichip, float*, float*, float*, float*);
	void 					appto_correlation_backgrounds();
	void					appto_correlation_background(int);

	void					do_correlations_minus_backgrounds();



	//Misc --------------------------------------------------------------------
	bool					unsuitable_t(float, float, float);
	bool					unsuitable_x(float, float, float);
	bool					unsuitable_tbg(float, float, float);
	bool					suitable_xt(Cpix_hit*, Cpix_hit*);
	bool					suitable_yt(Cpix_hit*, Cpix_hit*);
	int						get_iref_start(float, int);
	int						get_xiref_start(float, int);
	void 					chip_SN_update(int, int, int);
	void					find_chip_SNs();
	void					align_by_differences();
	void					align_by_pix_differences();
	void					align_by_clust_differences();
	void					get_extra_difference();


	//Setters and getters -----------------------------------------------------
	void					set_tel(Ctel_chunk * tel) {_tel = tel; set_ref_chip_byID(_ref_chipID);}
	int 					get_ref_chipID() {return _ref_chipID;}
	void 					set_ref_chip_byID(int); //Big, so defined in .cpp

	int 					get_dir() {return _dir;}
	void 					set_dir(int dir){_dir = dir;}

	Cchip* 					get_ref_chip() {return _tel->get_chip(_ref_chipID);}

	std::string				get_pixs_or_clusts() {return _pixs_or_clusts;}
	void 					set_pixs_or_clusts(std::string pixs_or_clusts){
								_pixs_or_clusts = pixs_or_clusts;}


	int 					get_nbins() {return _nbins;}
	void 					set_nbins(int nbins){_nbins = nbins;}


	float 					get_xup_cut() {return _xup_cut;}
	void 					set_xup_cut_pixs(float x){_xup_cut = x*0.055;}
	void 					set_xup_cut(float x){_xup_cut = x;}

	float 					get_xlow_cut() {return _xlow_cut;}
	void 					set_xlow_cut_pixs(float x){_xlow_cut = x*0.055;}
	void 					set_xlow_cut(float x){_xlow_cut = x;}


	float 					get_yup_cut() {return _yup_cut;}
	void 					set_yup_cut_pixs(float y){_yup_cut = y*0.055;}
	void 					set_yup_cut(float y){_yup_cut = y;}

	float 					get_ylow_cut() {return _ylow_cut;}
	void 					set_ylow_cut_pixs(float y){_ylow_cut = y*0.055;}
	void 					set_ylow_cut(float y){_ylow_cut = y;}


	float 					get_tup_cut() {return _tup_cut;}
	void 					set_tup_cut(float t){_tup_cut = t;}


	float 					get_tlow_cut() {return _tlow_cut;}
	void 					set_tlow_cut(float t){_tlow_cut = t;}


	float 					get_tbgup_cut() {return _tbgup_cut;}
	void 					set_tbgup_cut(float t){_tbgup_cut = t;}


	float 					get_tbglow_cut() {return _tbglow_cut;}
	void 					set_tbglow_cut(float t){_tbglow_cut = t;}


	std::vector<TH2F*> &	get_correlations(){return _correlationsGlobal;}
	std::vector<TH1F*> &	get_differences(){return _differencesGlobal;}
	int						get_ncorrelations(){return _correlationsGlobal.size();}

	std::vector<TH2F*> &	get_correlation_backgrounds(){
								return _correlation_backgroundsGlobal;}

	std::vector<TH2F*> &	get_correlations_backgroundless(){
								return _correlations_backgroundlessGlobal;}
};

#endif
