#ifndef __CTRACK_MAKER_H_INCLUDED__
#define __CTRACK_MAKER_H_INCLUDED__


#include <vector>
#include "data_holders/Cchip.h"
#include "data_holders/Ccluster.h"
#include "data_holders/Cpix_hit.h"
#include "data_holders/Ctrack.h"
#include "data_holders/Ctrack_volume.h"
#include "data_holders/Ctel_chunk.h"
#include "../CDQM_options.h"
#include "string.h"
#include "TH1F.h"
#include "TFile.h"
#include "TImage.h"


class Ctrack_maker{
public:
	TImage * 					_eventViewPic;
	std::vector<Ctel_chunk*>	_tels;
	int							_chip_loop_cut;
	double						_tcut;
	double						_cyl_rx;
	double						_cyl_ry;
	double						_thetax;
	double						_thetay;
	std::vector<int>			_ref_chips;
	std::string					_save_file_name;
	std::string					_track_vol_shape;
	CDQM_options * 				_ops;
	TH1F * 						h_nClustersPerVol;


	//Member functions --------------------------------------------------------
	Ctrack_maker(CDQM_options*);
	~Ctrack_maker();
	void initialize();
	void execute(Ctel_chunk*);
	void finalize();

	void						initCheckPlots();
	void						fillCheckPlots(Ctrack_volume*);
	void						saveCheckPlots();
	void						make_tracks();
	void						search_chip(int, int);
	void						fill_seeded_vol(Ctrack_volume*);
	void						add_tracks(Ctrack_volume*);
	//void						set_cyl_r(double r){_cyl_r = r;}
	//void						set_theta(double t){_theta = t;}
	void						set_tcut(double t){_tcut = t;}


	//Setters and getters -----------------------------------------------------
};

#endif
