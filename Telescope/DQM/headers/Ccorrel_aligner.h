#ifndef __CINIT_ALIGNER_H_INCLUDED__
#define __CINIT_ALIGNER_H_INCLUDED__


#include "Ccorrel_line.h"
#include "Ccorrelation_plots.h"
#include "Ccorrel_line_finder.h"
#include "data_holders/Ctel_chunk.h"

#include "TH2F.h"
#include "TFile.h"

#include "stdlib.h"
#include "string.h"
#include <vector>
#include <sstream>


class Ccorrel_aligner{
private:
	Ctel_chunk *				_tel;
	int							_ref_chip;
	int							_chip_loop_cut;		
	std::string					_save_file_name;
	Ccorrelation_plots *		_x_correlation_plots;
	Ccorrelation_plots *		_y_correlation_plots;
	int							_n_proj_bins;



public:
	Ccorrel_aligner (Ctel_chunk*, std::string, Ccorrelation_plots*, Ccorrelation_plots*,
		int, int, int);

	void						align_all();
	void						align_direction(int);
	void						align_ts();
	void						find_line(int, int, TDirectory*);
	bool						on_correlations(int, int);
	void						ts_fill(int, Cpix_hit*, Cpix_hit*);
	void						save_figs();
	bool						unsuitable_t(Cpix_hit*, Cpix_hit*);
	void						clean_directories();
};

#endif