#ifndef __CPOOR_MANS_ALIGNER_H_INCLUDED__
#define __CPOOR_MANS_ALIGNER_H_INCLUDED__


#include "data_holders/Ctel_chunk.h"
#include "data_holders/Cpix_hit.h"
#include "TFile.h"
#include "data_holders/Cchip.h"


class Cpoor_mans_aligner{
private:
	Ctel_chunk*							_tel;
	int									_ref_chip;
	int									_chip_loop_cut;

	float								_ref_xbar;
	float								_ref_ybar;




public:
	Cpoor_mans_aligner (Ctel_chunk *, int, int chip_loop_cut = 10);
	void								align_all();
	void								align_chip(Cchip*);
	void								find_av_positions(Cchip*, float&, float&);
	void								find_ref_av_positions();
};

#endif