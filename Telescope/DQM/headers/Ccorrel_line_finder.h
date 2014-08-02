#ifndef __CCORREL_LINE_FINDER_H__
#define __CCORREL_LINE_FINDER_H__


#include "TGraph.h"
#include "TH1F.h"
#include "TH2F.h"
#include <vector>
#include "Ccorrel_line.h"
#include "../src/Chandy.cpp"
#include "TFile.h"
#include "TFitResultPtr.h"
#include "TF1.h"
#include "math.h"
#include "TCanvas.h"
#include "TLine.h"
#include "TDirectory.h"
#include <sstream>


class Ccorrel_line_finder{
private:
	TH2F*					_plot;
	Ccorrel_line*			_line;
	double					_theta_lower;
	double					_theta_upper;
	std::vector<TH1F*>		_projections;
	std::vector<TH1F*>		_capped_projections;
	std::vector<TH1F*>		_signal_projections;
	int 					_n_projections;
	TGraph*					_stationary_graph;
	TCanvas*				_c1;
	double					_temp_theta;
	int						_nproj_bins;
	bool					_save_switch;
	TDirectory*				_save_dir;


public:
	Ccorrel_line_finder(TH2F*, double, double, int, bool, TDirectory*);
	~Ccorrel_line_finder();
	void					fill_projections();
	void 					varying_projections();
	void					fill_stationary_plot();
	double					find_height_stationary_parameter(int, int&);
	void					fill_signal_projection(int);
	void					save_figs();
	void					set_line();
	void					add_line();

	void					set_plot(TH2F* plot){_plot = plot;}
	void					save_results();
	TH2F*					get_plot(){return _plot;}


	void					set_theta_lower(double t){_theta_lower = t;}
	double					get_theta_lower(){return _theta_lower;}

	void					set_theta_upper(double t){_theta_upper = t;}
	double					get_theta_upper(){return _theta_upper;}

	Ccorrel_line*			get_line(){return _line;};

};

#endif
