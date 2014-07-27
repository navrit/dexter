//This class contains all the methods required to make various kinds of 
//correlation plots, for use in analysis and the DQM. Can be used for all of
//["x", "y", "t"] directions, as well as for pixels or clusters.

//Author: Dan Saunders
//Date created: 15/10/13
//Last modified: 25/11/13

//Reference chip on the y axis of the th2s.

#include "../headers/Ccorrelation_plots.h"

//-----------------------------------------------------------------------------

Ccorrelation_plots::Ccorrelation_plots(CDQM_options * ops, std::string pixs_or_clusts, int dir){
	_dir = dir;
	std::cout<<"Constructor of Ccorrelation plots, for " << _pixs_or_clusts << " in dir: "<<
		Chandy::get_str_dir(_dir)<<std::endl;

	_ops = ops;
	_pixs_or_clusts = pixs_or_clusts;
}



//-----------------------------------------------------------------------------

void Ccorrelation_plots::initialize(){
	std::cout<<"Initializer of Ccorrelation plots, for " << _pixs_or_clusts << " in dir: "<<Chandy::get_str_dir(_dir)<<std::endl;
	_save_file_name = _ops->save_file_name;
	_chip_loop_cut = _ops->chip_cut;

	//Set default values as if for pixels.
	_ref_chipID = _ops->ref_chips[0];
	if (_pixs_or_clusts == "clusts") {
		if (_dir == 3) _nbins = _ops->clust_correl_tnbins;
		else _nbins = _ops->clust_correl_xynbins;
	}
	else  {
		if (_dir == 3) _nbins = _ops->pix_correl_tnbins;
		else _nbins = _ops->pix_correl_xynbins;
	}


	//Default histogram ranges.
	_posn_xmin = 0.0;
	_posn_xmax = 15.0;

	_clust_diff_xmin = -1.8;
	_clust_diff_xmax = 1.8;

	_pix_diff_xmin = -6.0;
	_pix_diff_xmax = 6.0;

	_clust_diff_tmin = -50.0;
	_clust_diff_tmax = 50.0;

	_pix_diff_tmin = -100.0;
	_pix_diff_tmax = 100.0;

	_t_xmin = -100;
	_t_xmax = 100;


	if (_pixs_or_clusts == "pixs"){
		_xup_cut = _ops->pix_xup*0.055;
		_xlow_cut = _ops->pix_xlow*0.055;
		_yup_cut = _ops->pix_yup*0.055;
		_ylow_cut = _ops->pix_ylow*0.055;

		_tup_cut = _ops->pix_tup;
		_tlow_cut = _ops->pix_tlow;
		_tbgup_cut = _ops->pix_tbgup;
		_tbglow_cut = _ops->pix_tbglow;
	}

	else if (_pixs_or_clusts == "clusts"){
		_xup_cut = _ops->clust_xup;
		_xlow_cut = _ops->clust_xlow;
		_yup_cut = _ops->clust_yup;
		_ylow_cut = _ops->clust_ylow;

		_tup_cut = _ops->clust_tup;
		_tlow_cut = _ops->clust_tlow;
		_tbgup_cut = _ops->clust_tbgup;
		_tbglow_cut = _ops->clust_tbglow;
	}



	init_correlations();
	init_correlation_backgrounds();
}





//-----------------------------------------------------------------------------

void Ccorrelation_plots::finalize(){
	std::cout<<"Finalizer of Ccorrelation plots, for " << _pixs_or_clusts << " in dir: "<<"\t"<<Chandy::get_str_dir(_dir)<<std::endl;
	set_directories();
	save_figs();
}





//-----------------------------------------------------------------------------

void Ccorrelation_plots::set_ref_chip_byID(int ref_chipID){
	_ref_chipID = ref_chipID;
}






//-----------------------------------------------------------------------------

void Ccorrelation_plots::init_correlations(){
	//Initializes the correlation plots, ready for filling.
	std::stringstream ss; 
	ss<<_ops->ref_chips[0];
	std::string correl_temp_title;
	std::string diff_temp_title;
	if (get_dir() ==0 || get_dir() == 1) correl_temp_title = get_pixs_or_clusts() + "_" + Chandy::get_str_dir(get_dir()) + 
								   "_vs_" + ss.str() + "; " + Chandy::get_str_dir(get_dir()) +
								   " (/mm); " + Chandy::get_str_dir(get_dir()) + " (/mm)";

	else correl_temp_title = get_pixs_or_clusts() + "_" + Chandy::get_str_dir(get_dir()) + 
								   "_vs_" + ss.str() + "; " + Chandy::get_str_dir(get_dir()) +
								   " (/DAQ t units); " + Chandy::get_str_dir(get_dir()) + " (/DAQ t units)";
								   							   

	if (get_dir() ==0 || get_dir() == 1) diff_temp_title = get_pixs_or_clusts() + "_" + Chandy::get_str_dir(get_dir()) + 
								   "_minus_" + ss.str() + "; " + Chandy::get_str_dir(get_dir()) +
								   " (/mm); " + Chandy::get_str_dir(get_dir()) + " (/mm)";

	else diff_temp_title = get_pixs_or_clusts() + "_" + Chandy::get_str_dir(get_dir()) + 
								   "_minus_" + ss.str() + "; " + Chandy::get_str_dir(get_dir()) +
								   " (/DAQ t units); " + Chandy::get_str_dir(get_dir()) + " (/DAQ t units)";

	//Cycle over the chips.
	for (int i=0; i<_ops->nchips; i++){

		TH2F* h_correl = get_empty_correlation(i, correl_temp_title, true);
		h_correl->SetMinimum(-1);
		_correlationsLocal.push_back(h_correl);

		h_correl = get_empty_correlation(i, correl_temp_title, false);
		h_correl->SetMinimum(-1);
		_correlationsGlobal.push_back(h_correl);


		//Set up the differences too.
		TH1F* h_diff = get_empty_differences(i, diff_temp_title, true);
		_differencesLocal.push_back(h_diff);

		h_diff = get_empty_differences(i, diff_temp_title, false);
		_differencesGlobal.push_back(h_diff);

		if (i == _chip_loop_cut) break;
	}
}








//-----------------------------------------------------------------------------

void Ccorrelation_plots::init_correlation_backgrounds(){
	//Initializes the correlation plots, ready for filling.

	std::stringstream ss; 
	ss<<_ops->ref_chips[0];
	std::string correl_temp_name = get_pixs_or_clusts() + "_vs_" + ss.str();


	//Cycle over the chips.
	for (int i=0; i<_ops->nchips; i++){
		TH2F* h = get_empty_correlation(i, correl_temp_name, true);
		_correlation_backgroundsLocal.push_back(h);

		h = get_empty_correlation(i, correl_temp_name, false);
		_correlation_backgroundsGlobal.push_back(h);

		if (i == _chip_loop_cut) break;
	}
}








//-----------------------------------------------------------------------------

TH1F* Ccorrelation_plots::get_empty_differences(int ichip, std::string temp_title, bool locVsGlob){
	std::stringstream ss_chip;
	ss_chip<<ichip;
	std::string title = ss_chip.str() + temp_title;
	std::string name = "Chip_" + ss_chip.str();
	if (locVsGlob) {
		name += "Local";
		title += "Local";
	}

	else {
		name += "Global";
		title += "Global";
	}


	TH1F* h_diff;

	if (get_pixs_or_clusts() == "pixs") {
		if (get_dir() != 3) h_diff = new TH1F(name.c_str(), title.c_str(), 271, _pix_diff_xmin, _pix_diff_xmax);
		else h_diff = new TH1F(name.c_str(), title.c_str(), 197, _pix_diff_tmin, _pix_diff_tmax);
	}

	else {
		if (get_dir() != 3) h_diff = new TH1F(name.c_str(), title.c_str(), 105, _clust_diff_xmin, _clust_diff_xmax);
		else h_diff = new TH1F(name.c_str(), title.c_str(), 197, _clust_diff_tmin, _clust_diff_tmax);
	}

	return h_diff;
}








//-----------------------------------------------------------------------------

TH2F* Ccorrelation_plots::get_empty_correlation(int ichip, 
	std::string temp_name, bool locVsGlob){
	//Initializes a correlation plot, ready for filling.
	float chip1_min, chip1_max, chip2_min, chip2_max;

	if (_dir != 3) {
		//Assume ok alignment.
		chip1_min = _posn_xmin;
		chip1_max = _posn_xmax;

		chip2_min = _posn_xmin;
		chip2_max = _posn_xmax;
	}

	else {
		chip1_min = _t_xmin;
		chip1_max = _t_xmax;
		chip2_min = _t_xmin;
		chip2_max = _t_xmax;
	}

	//Make the hist.
	std::stringstream ss; 
	ss<<ichip;
	std::string locOrGlob;
	TH2F* h;
	if (locVsGlob) {
		locOrGlob = "Local";
		h = new TH2F(("Chip_" + ss.str() + locOrGlob).c_str(),
				 (ss.str() + temp_name + locOrGlob).c_str(),
				  get_nbins(), 0, 256,
		    	  get_nbins(), 0, 256);
	}
	else {
		locOrGlob = "Global";
		h = new TH2F(("Chip_" + ss.str() + locOrGlob).c_str(),
				 (ss.str() + temp_name + locOrGlob).c_str(),
				  get_nbins(), chip1_min, chip1_max,
		    	  get_nbins(), chip2_min, chip2_max);
	}

	

	return h;
}








//-----------------------------------------------------------------------------

void Ccorrelation_plots::execute(Ctel_chunk * tel){
	_tel = tel;
	clock_t t_start, t_end;
	t_start = clock();
	appto_correlations();
	t_end = clock();

	std::cout<<"Correls time taken (s): "<<((float)t_end - (float)t_start)/CLOCKS_PER_SEC<<std::endl;
	appto_correlation_backgrounds();
	do_correlations_minus_backgrounds();
}








//-----------------------------------------------------------------------------

void Ccorrelation_plots::do_correlations_minus_backgrounds(){
	if (get_dir()!=3){
		_correlations_backgroundlessLocal.clear();
		_differences_backgroundlessLocal.clear();
		_correlations_backgroundlessGlobal.clear();
		_differences_backgroundlessGlobal.clear();

		//Method to subtract the backgrounds (which must have  qalready been found)
		//from the correlation plots, using a method in Chandy.
		for (int iplot = 0; iplot < _correlationsLocal.size(); iplot++){

			//need to weight the second histo more (by the factor w), hence
			//w<1, so the ratio of number hits in signal+back to back.
			float w = _correlationsLocal[iplot]->GetEntries()/
					  _correlation_backgroundsLocal[iplot]->GetEntries();
			TH2F* h = Chandy::subtract_TH2Fs(_correlationsLocal[iplot], 
											 _correlation_backgroundsLocal[iplot],
											 w, true, _correlationsLocal[iplot]->GetName());

			_correlations_backgroundlessLocal.push_back(h);

			TH1F* h_diff = Chandy::project_2dhist(h, 0.7854, get_nbins(), _tel->get_chip(iplot)->get_name());
			_differences_backgroundlessLocal.push_back(h_diff);

			w = _correlationsGlobal[iplot]->GetEntries()/
					  _correlation_backgroundsGlobal[iplot]->GetEntries();
			h = Chandy::subtract_TH2Fs(_correlationsGlobal[iplot], 
											 _correlation_backgroundsGlobal[iplot],
											 w, true, _correlationsGlobal[iplot]->GetName());

			_correlations_backgroundlessGlobal.push_back(h);

			//do the differences.
			h_diff = Chandy::project_2dhist(h, 0.7854, get_nbins(), _tel->get_chip(iplot)->get_name());
			_differences_backgroundlessGlobal.push_back(h_diff);
		}
	}
	
}








//-----------------------------------------------------------------------------

void Ccorrelation_plots::appto_correlations(){
	//Fills the raw correlation plots over all chips. Contains various tricks
	//for speed.
	for (int ichip = 0; ichip != _tel->get_nchips(); ++ichip){
		if (_tel->get_chip(_ref_chipID)->get_npix_hits() == 0) continue;
		else {
			if (get_pixs_or_clusts() == "pixs") {
				if (_tel->get_chip(ichip)->get_npix_hits() != 0) {
					if (get_dir()==3) appto_time_correlation(ichip, _ref_chipID);
					else appto_correlation(ichip, _ref_chipID);

					if (ichip == _chip_loop_cut) break;
				}
			}
			else {
				if (_tel->get_chip(ichip)->get_nclusters() != 0) {
					if (get_dir()==3) appto_time_correlation(ichip, _ref_chipID);
					else appto_correlation(ichip, _ref_chipID);

					if (ichip == _chip_loop_cut) break;
				}
			}
		}


		
		
		if (ichip == _chip_loop_cut) break;
	}
}









//-----------------------------------------------------------------------------

void Ccorrelation_plots::appto_time_correlation(int ichip1, int ichip2){
	//Its handy to pass variables in. Slight workaround i know.
	//Fills the raw correlation plots over all chips. Contains various tricks
	//for speed.
	float gposn[4], lposn[4], ref_gposn[4], ref_lposn[4];
	int N, Nref;


	//Cycle over both sets of pixels indexs.
	if (get_pixs_or_clusts() == "pixs"){
		N = _tel->get_chip(ichip1)->get_npix_hits();
		Nref = _tel->get_chip(ichip2)->get_npix_hits();
	}

	else {
		N = _tel->get_chip(ichip1)->get_nclusters();
		Nref = _tel->get_chip(ichip2)->get_nclusters();
	}

	for (int i=0; i < N; i++){

		//Start the loop over pixels two at the farthest index back for 
		//the t window - saves time. Weary of the one thingy. Approx.
		if (get_pixs_or_clusts() == "pixs") _tel->get_chip(ichip1)->get_pix_hits_by_glob_x()[i]->get_lposn(lposn);
		else _tel->get_chip(ichip1)->get_clusters_by_glob_x()[i]->get_lposn(lposn);

		_tel->get_chip(ichip1)->lposn_to_gposn(lposn, gposn);
		//gposn contains the x global position of the chip1 pix/clust.

		int iref_start = 0;
		//iref_start = get_xiref_start(gposn[0] + _xlow_cut, ichip2);


		//std::cout<<iref_start<<"\t"<<i;
		for (int iref=iref_start; iref<Nref; iref++){

			if (get_pixs_or_clusts() == "pixs") _tel->get_chip(ichip2)->get_pix_hits_by_glob_x()[iref]->get_lposn(ref_lposn);
			else _tel->get_chip(ichip2)->get_clusters_by_glob_x()[iref]->get_lposn(ref_lposn);
			_tel->get_chip(ichip2)->lposn_to_gposn(ref_lposn, ref_gposn);

		

			//Ask if too far ahead in time (ifso, break and save time).
			
			correlation_fill(ichip1, gposn, ref_gposn, lposn, ref_lposn);
//			if (unsuitable_x(gposn[0], ref_gposn[0], _xup_cut)) {
//				//std::cout<<"\t"<<iref<<std::endl;
//				break;
//			}
		}
	}
}









//-----------------------------------------------------------------------------

void Ccorrelation_plots::appto_correlation(int ichip1, int ichip2){
	//Its handy to pass variables in. Slight workaround i know.
	//Fills the raw correlation plots over all chips. Contains various tricks
	//for speed.
	float gposn[4], lposn[4], ref_gposn[4], ref_lposn[4];
	int N, Nref;

	//Cycle over both sets of pixels indexs.
	if (get_pixs_or_clusts() == "pixs"){
		N = _tel->get_chip(ichip1)->get_npix_hits();
		Nref = _tel->get_chip(ichip2)->get_npix_hits();
	}

	else {
		N = _tel->get_chip(ichip1)->get_nclusters();
		Nref = _tel->get_chip(ichip2)->get_nclusters();
	}

	for (int i=0; i < N; i++){

		//Start the loop over pixels two at the farthest index back for 
		//the t window - saves time. Weary of the one thingy. Approx.
		if (get_pixs_or_clusts() == "pixs") _tel->get_chip(ichip1)->get_pix_hits()[i]->get_lposn(lposn);
		else _tel->get_chip(ichip1)->get_clusters()[i]->get_lposn(lposn);

		_tel->get_chip(ichip1)->lposn_to_gposn(lposn, gposn);

		int iref_start = 0;
		iref_start = get_iref_start(gposn[3] + _tlow_cut, ichip2);
		for (int iref=iref_start; iref<Nref; iref++){

			if (get_pixs_or_clusts() == "pixs") _tel->get_chip(ichip2)->get_pix_hits()[iref]->get_lposn(ref_lposn);
			else _tel->get_chip(ichip2)->get_clusters()[iref]->get_lposn(ref_lposn);

			_tel->get_chip(ichip2)->lposn_to_gposn(ref_lposn, ref_gposn);

			//Ask if too far ahead in time (ifso, break and save time).
			if (unsuitable_t(gposn[3], ref_gposn[3], _tup_cut)) break;
			correlation_fill(ichip1, gposn, ref_gposn, lposn, ref_lposn);
			
		}
	}
}









//-----------------------------------------------------------------------------

void Ccorrelation_plots::appto_correlation_backgrounds(){
	//Fills the raw correlation plots over all chips. Contains various tricks
	//for speed.
	if (get_dir()!=3){
		for (int ichip = 0; ichip != _tel->get_nchips(); ++ichip){
			if (_tel->get_chip(_ref_chipID)->get_npix_hits() == 0) continue;
			else {
				if (get_pixs_or_clusts() == "pixs") {
					if (_tel->get_chip(ichip)->get_npix_hits() != 0) {
						appto_correlation_background(ichip);
						if (ichip == _chip_loop_cut) break;
					}
				}
				else {
					if (_tel->get_chip(ichip)->get_nclusters() != 0) {
						appto_correlation_background(ichip);
						if (ichip == _chip_loop_cut) break;
					}
				}
			}
		}
	}
}









//-----------------------------------------------------------------------------

void Ccorrelation_plots::appto_correlation_background(int ichip){
	//Fills the raw correlation plots over all chips. Contains various tricks
	//for speed.
	float gposn[4], lposn[4], ref_gposn[4], ref_lposn[4];
	int N, Nref;

	//Cycle over both sets of pixels indexs.
	if (get_pixs_or_clusts() == "pixs"){
		N = _tel->get_chip(ichip)->get_npix_hits();
		Nref = get_ref_chip()->get_npix_hits();
	}

	else {
		N = _tel->get_chip(ichip)->get_nclusters();
		Nref = get_ref_chip()->get_nclusters();
	}

	for (int i=0; i < N; i++){

		//Start the loop over pixels two at the farthest index back for 
		//the t window - saves time. Weary of the one thingy. Approx.
		if (get_pixs_or_clusts() == "pixs") _tel->get_chip(ichip)->get_pix_hits()[i]->get_lposn(lposn);
		else _tel->get_chip(ichip)->get_clusters()[i]->get_lposn(lposn);

		_tel->get_chip(ichip)->lposn_to_gposn(lposn, gposn);


		int iref_start = 0;
		if (get_dir() != 3) iref_start = get_iref_start(gposn[3] + _tbglow_cut, _ref_chipID);
		for (int iref=iref_start; iref<Nref; iref++){

			if (get_pixs_or_clusts() == "pixs") get_ref_chip()->get_pix_hits()[iref]->get_lposn(ref_lposn);
			else get_ref_chip()->get_clusters()[iref]->get_lposn(ref_lposn);

			get_ref_chip()->lposn_to_gposn(ref_lposn, ref_gposn);

			//Ask if too far ahead in time (ifso, break and save time).
			if (unsuitable_tbg(gposn[3], ref_gposn[3], _tbgup_cut)) break;
			//At this stage, both pixels are switched on, and 
			//near the tbounds. Send to another function to check
			//bound and fill.
			background_fill(ichip, gposn, ref_gposn, lposn, ref_lposn);
			
		}
	}
}









//-----------------------------------------------------------------------------

void Ccorrelation_plots::background_fill(int ichip, float gposn[4], 
	float ref_gposn[4], float lposn[4], float ref_lposn[4]){
	float delx = gposn[0] - ref_gposn[0];
	float dely = gposn[1] - ref_gposn[1];
	float delt = -(gposn[3] - ref_gposn[3]);


	//Case of making a x or y correlation.
	//Ask if inside tbg cuts.
	if (delt > _tbglow_cut && delt < _tbgup_cut){
		if (get_dir() == 0){
			//Ask if inside y cuts.
			if (dely > _ylow_cut && dely < _yup_cut){
				_correlation_backgroundsGlobal[ichip]->Fill(gposn[0], ref_gposn[0]);
				_correlation_backgroundsLocal[ichip]->Fill(0.055*lposn[0], 0.055*ref_lposn[0]);
			}
		}


		else if (get_dir() == 1){
			//Ask if inside x cuts.
			if (delx > _xlow_cut && delx < _xup_cut){
				_correlation_backgroundsGlobal[ichip]->Fill(gposn[1], ref_gposn[1]);
				_correlation_backgroundsLocal[ichip]->Fill(0.055*lposn[1], 0.055*ref_lposn[1]);
			}
		}
	}



	//Case of making a t correlation - invert spatial cuts. 
	if (get_dir() == 3){
		if (delx < _xlow_cut && delx > _xup_cut &&
			dely < _ylow_cut && dely > _yup_cut){
			_correlation_backgroundsGlobal[ichip]->Fill(gposn[3], ref_gposn[3]);
			_correlation_backgroundsLocal[ichip]->Fill(lposn[3], ref_lposn[3]);
		}
	}
}	











//-----------------------------------------------------------------------------

void Ccorrelation_plots::correlation_fill(int ichip, float gposn[4], 
	float ref_gposn[4], float lposn[4], float ref_lposn[4]){

	float delxLoc = 0.055*(ref_lposn[0] - lposn[0]);
	float delyLoc = 0.055*(ref_lposn[1] - lposn[1]);
	float deltLoc = ref_lposn[2] - lposn[2];

	float delxGlob = ref_gposn[0] - gposn[0];
	float delyGlob = ref_gposn[1] - gposn[1];
	float deltGlob = ref_gposn[3] - gposn[3];


	//Case of making a x or y correlation.
	//Ask if inside t cuts.
	if (deltGlob > _tlow_cut && deltGlob < _tup_cut){
		if (get_dir() == 0){
			//Ask if inside y cuts.
			if (delyGlob > _ylow_cut && delyGlob < _yup_cut){
				_correlationsGlobal[ichip]->Fill(gposn[0], ref_gposn[0]);
				_differencesGlobal[ichip]->Fill(delxGlob);

				_correlationsLocal[ichip]->Fill(lposn[0], ref_lposn[0]);
				_differencesLocal[ichip]->Fill(delxLoc);
			}
		}


		else if (get_dir() == 1){
			//Ask if inside x cuts.
			if (delxGlob > _xlow_cut && delxGlob < _xup_cut){
				_correlationsGlobal[ichip]->Fill(gposn[1], ref_gposn[1]);
				_differencesGlobal[ichip]->Fill(delyGlob);

				_correlationsLocal[ichip]->Fill(lposn[1], ref_lposn[1]);
				_differencesLocal[ichip]->Fill(delyLoc);
			}
		}
	}



	//Case of making a t correlation.
	if (get_dir() == 3){
		//std::cout<<"tref : t : delta t\t"<<ref_gposn[4]<<"\t"<<gposn[4]<<"\t"<<deltGlob<<std::endl;
		if (delxGlob > _xlow_cut && delxGlob < _xup_cut &&
			delyGlob > _ylow_cut && delyGlob < _yup_cut){

			_correlationsGlobal[ichip]->Fill(gposn[3], ref_gposn[3]);
			_differencesGlobal[ichip]->Fill(deltGlob);

			_correlationsLocal[ichip]->Fill(lposn[2], ref_lposn[2]);
			_differencesLocal[ichip]->Fill(deltLoc);
		}
	}
}	








//-----------------------------------------------------------------------------

int Ccorrelation_plots::get_xiref_start(float x, int irefchip){
	//Returns the index of the pixel corresponding to the given TOA minus 
	//the _t_bound by using the TOA_to_ID functions in the Cchip.

	//If here, always the reference chip.
	int irpix_start = 0;
	if (get_pixs_or_clusts() == "pixs"){
		irpix_start = _tel->get_chip(irefchip)->glob_x_to_pixID(x);
	}

	else irpix_start = _tel->get_chip(irefchip)->glob_x_to_clustID(x);
	

	// float temp_lposn[4], temp_gposn[4];
	// _tel->get_chip(irefchip)->get_pix_hits_by_glob_x()[irpix_start]->get_lposn(temp_lposn);
	// _tel->get_chip(irefchip)->lposn_to_gposn(temp_lposn, temp_gposn);
	// std::cout<<x<<"\t"<<temp_gposn[0]<<std::endl;
	return irpix_start;		
}








//-----------------------------------------------------------------------------

int Ccorrelation_plots::get_iref_start(float t, int irefchip){
	//Returns the index of the pixel corresponding to the given TOA minus 
	//the _t_bound by using the TOA_to_ID functions in the Cchip.

	//If here, always the reference chip.
	int irpix_start = 0;
	if (get_pixs_or_clusts() == "pixs"){
		irpix_start = _tel->get_chip(irefchip)->glob_t_to_pixID(t);
	}

	else irpix_start = _tel->get_chip(irefchip)->glob_t_to_clustID(t);
	
	return irpix_start;		
}








//-----------------------------------------------------------------------------

bool Ccorrelation_plots::unsuitable_x(float x1, float x2, float x_bound){
	//Checks whether pixel2 is too far ahead of pixel1 in time - useful,
	//as breaks the second pixel loop, speeding up massively.
	//std::cout<<t1<<"\t"<<t2<<"\t"<<t2 - t1<<"\t"<<t_bound<<std::endl;
	if ((x2 - x1) > x_bound) return true;
	return false;
}








//-----------------------------------------------------------------------------

bool Ccorrelation_plots::unsuitable_t(float t1, float t2, float t_bound){
	//Checks whether pixel2 is too far ahead of pixel1 in time - useful,
	//as breaks the second pixel loop, speeding up massively.
	//std::cout<<t1<<"\t"<<t2<<"\t"<<t2 - t1<<"\t"<<t_bound<<std::endl;
	if ((t2 - t1) > t_bound) return true;
	return false;
}








//-----------------------------------------------------------------------------

bool Ccorrelation_plots::unsuitable_tbg(float t1, float t2, float t_bound){
	//Checks whether pixel 1 is 
	if (get_dir() != 3){
		if ((t2 - t1) > t_bound) return true;
	}
	return false;
}








//-----------------------------------------------------------------------------
void Ccorrelation_plots::tag_to_chips(){
	for (int i=0; i<_tel->get_nchips(); i++){
		TH2F* h_copy;
		if (_correlations_backgroundlessGlobal.size() > 0) h_copy = (TH2F*)_correlations_backgroundlessGlobal[i]->Clone("my_hist");

		if (i == _chip_loop_cut) break;
	}

}








//-----------------------------------------------------------------------------

void Ccorrelation_plots::save_figs(){
	TFile* save_file = new TFile(_save_file_name.c_str(),"update");
	std::string temp_name = Chandy::get_str_dir(get_dir()) + "_" + get_pixs_or_clusts() + "_correlations/";
	std::string name;


	name = temp_name + "Global/Correlations";
	save_file->cd(name.c_str());
	for (int iplot = 0; iplot<_correlationsGlobal.size(); iplot++){
		_correlationsGlobal[iplot]->Write("", TObject::kOverwrite);
	}



	name = temp_name + "Global/Backgrounds";
	save_file->cd(name.c_str());
	for (int iplot = 0; iplot<_correlation_backgroundsGlobal.size(); iplot++){
		_correlation_backgroundsGlobal[iplot]->Write("", TObject::kOverwrite);
	}



	name = temp_name + "Global/Correlations_minus_backgrounds";
	save_file->cd(name.c_str());
	for (int iplot = 0; iplot<_correlations_backgroundlessGlobal.size(); iplot++){
		_correlations_backgroundlessGlobal[iplot]->Write("", TObject::kOverwrite);
	}



	name = temp_name + "Global/Differences";
	save_file->cd(name.c_str());
	for (int iplot = 0; iplot<_differencesGlobal.size(); iplot++){
		_differencesGlobal[iplot]->Write("", TObject::kOverwrite);
	}



	name = temp_name + "Global/Differences_minus_backgrounds";
	save_file->cd(name.c_str());
	for (int iplot = 0; iplot<_differences_backgroundlessGlobal.size(); iplot++){
		_differences_backgroundlessGlobal[iplot]->Write("", TObject::kOverwrite);
	}



	name = temp_name + "Local/Correlations";
	save_file->cd(name.c_str());
	for (int iplot = 0; iplot<_correlationsLocal.size(); iplot++){
		_correlationsLocal[iplot]->Write("", TObject::kOverwrite);
	}



	name = temp_name + "Local/Backgrounds";
	save_file->cd(name.c_str());
	for (int iplot = 0; iplot<_correlation_backgroundsLocal.size(); iplot++){
		_correlation_backgroundsLocal[iplot]->Write("", TObject::kOverwrite);
	}



	name = temp_name + "Local/Correlations_minus_backgrounds";
	save_file->cd(name.c_str());
	for (int iplot = 0; iplot<_correlations_backgroundlessLocal.size(); iplot++){
		_correlations_backgroundlessLocal[iplot]->Write("", TObject::kOverwrite);
	}



	name = temp_name + "Local/Differences";
	save_file->cd(name.c_str());
	for (int iplot = 0; iplot<_differencesLocal.size(); iplot++){
		_differencesLocal[iplot]->Write("", TObject::kOverwrite);
	}



	name = temp_name + "Local/Differences_minus_backgrounds";
	save_file->cd(name.c_str());
	for (int iplot = 0; iplot<_differences_backgroundlessLocal.size(); iplot++){
		_differences_backgroundlessLocal[iplot]->Write("", TObject::kOverwrite);
	}


	save_file->Close();
	std::cout<<"- Saved cor "<<Chandy::get_str_dir(get_dir())<<" plts -\n";
}








//-----------------------------------------------------------------------------

void Ccorrelation_plots::set_directories(){
	//Make a directory to save everything in this instance.
	TFile* save_file = new TFile(_save_file_name.c_str(),"update");

	save_file->cd();
	std::string name = Chandy::get_str_dir(get_dir()) + "_" + get_pixs_or_clusts() + "_correlations";
	//std::cout<<"Dan! "<<name<<std::endl;

	TDirectory* instance_direc = save_file->mkdir(name.c_str());
	instance_direc->cd();


	//Make the subdirectories.

	instance_direc->mkdir("Local/Correlations");
	instance_direc->mkdir("Local/Backgrounds");
	instance_direc->mkdir("Local/Correlations_minus_backgrounds");
	instance_direc->mkdir("Local/Differences");
	instance_direc->mkdir("Local/Differences_minus_backgrounds");

	instance_direc->mkdir("Global/Correlations");
	instance_direc->mkdir("Global/Backgrounds");
	instance_direc->mkdir("Global/Correlations_minus_backgrounds");
	instance_direc->mkdir("Global/Differences");
	instance_direc->mkdir("Global/Differences_minus_backgrounds");

	save_file->Close();
}








//-----------------------------------------------------------------------------

void Ccorrelation_plots::align_by_pix_differences(){
	//Sets the position of a chip to the of the highest bin.
	float gposn[4];
	double shift;


	for (std::vector<Cchip*>::iterator ichip = _tel->get_chips().begin();
		 ichip != _tel->get_chips().end(); ++ichip){

		(*ichip)->get_gposn(gposn);
		shift = _differencesGlobal[(*ichip)->get_ID()]->GetBinCenter(
			_differencesGlobal[(*ichip)->get_ID()]->GetMaximumBin());

		gposn[get_dir()] += shift;
		(*ichip)->set_gposn(gposn);

		if ((*ichip)->get_ID() == _chip_loop_cut) break;
	}

}








//-----------------------------------------------------------------------------

void Ccorrelation_plots::align_by_clust_differences(){
	//Sets the position of a chip to the of the highest bin.
	float gposn[4];
	double shift;

	for (std::vector<Cchip*>::iterator ichip = _tel->get_chips().begin();
		 ichip != _tel->get_chips().end(); ++ichip){

		(*ichip)->get_gposn(gposn);
		shift = _differencesGlobal[(*ichip)->get_ID()]->GetBinCenter(
			_differencesGlobal[(*ichip)->get_ID()]->GetMaximumBin());

		gposn[get_dir()] += shift;
		(*ichip)->set_gposn(gposn);

		if ((*ichip)->get_ID() == _chip_loop_cut) break;
	}

}








//-----------------------------------------------------------------------------

void Ccorrelation_plots::align_by_differences(){
	//Sets the position of a chip to the of the highest bin.
	if (_pixs_or_clusts == "pixs") align_by_pix_differences();
	else align_by_clust_differences();

}








//-----------------------------------------------------------------------------

void Ccorrelation_plots::poor_mans_resolution(){
	if (_differencesGlobal.size() != 0){
		fill_variance_vector();
		get_extra_difference();

		fill_res_vector();

	}
}








//-----------------------------------------------------------------------------

void Ccorrelation_plots::get_extra_difference(){
	// std::string s ="temp";
	// TH2F* h_correl = get_empty_correlation(0, s);
	// TH1F* h_diff = get_empty_differences(0, s);


	// appto_correlation(0, 1);


	// TF1 * fit;
	// h_diff->Fit("gaus", "Q");
	// fit = h_diff->GetFunction("gaus");
	// _variance_vector.push_back(fit->GetParameter(2)*fit->GetParameter(2));
	
	// delete fit;
	// delete h_correl;
	// delete h_diff;
}








//-----------------------------------------------------------------------------

void Ccorrelation_plots::fill_res_vector(){
	_res_vector.clear();
	int N = std::min(_tel->get_nchips(), _chip_loop_cut);

	//First get (2)V_Ref
	float V_ref = 0.5*(_variance_vector[0] - _variance_vector[_variance_vector.size()-1] + _variance_vector[1]);


	//Now fill res vector, noting that the V_ref will be placed later.
	for (int i=0; i<N; i++){
		_res_vector.push_back(pow(_variance_vector[i] - V_ref, 0.5));
	}

	_res_vector[_ref_chipID] = pow(V_ref, 0.5);

	//print out for check.

	for (int i=0; i<_res_vector.size(); i++){
		if (i==_ref_chipID) std::cout<<"Res of chip "<<i<<": "<<1000*_res_vector[i]<<" um (ref)"<<std::endl;
		else std::cout<<"Res of chip "<<i<<": "<<1000*_res_vector[i]<<" um"<<std::endl;
	}
	/*	
	int N = std::min(_tel->get_nchips(), _chip_loop_cut);
	TMatrix foom(N, N);

	for (int i=0; i<N; i++){
		for (int j=0; j<N; j++){
			if (i==j) foom[i][j] = 1.0;
			else foom[i][j] = 0.0;

			if (j==_ref_chipID) foom[i][j] += 1;
		}
	}

	foom[_ref_chipID, _ref_chipID] = 0;
	foom.Print();

	TMatrix vars(N, 1);
	for (int i=0; i<N; i++) vars[i] = _variance_vector[i];

	//Invert.
	foom = foom.Invert();
	
	foom.Print();
	// TMatrix res = foom*vars;
	// res.print();

	//Time variance vector.

	*/
}








//-----------------------------------------------------------------------------

void Ccorrelation_plots::fill_variance_vector(){
	_variance_vector.clear();
	//Fits a gaussian to each _differences hist, and puts the width into the
	//variance vector.
	TF1 * fit;

	for (int i=0; i<_tel->get_nchips(); i++){
		_differencesGlobal[i]->Fit("gaus", "Q");
		fit = _differencesGlobal[i]->GetFunction("gaus");
		_variance_vector.push_back(fit->GetParameter(2)*fit->GetParameter(2)); //we want the variance.
		if (i==_chip_loop_cut) break;
	}
}








//-----------------------------------------------------------------------------

Ccorrelation_plots::~Ccorrelation_plots(){
	//Make a directory to save everything in this instance.
	std::cout<<__LINE__<<std::endl;
	for (int iplot = 0; iplot<_correlationsLocal.size(); iplot++)
		delete _correlationsLocal[iplot];

	std::cout<<__LINE__<<std::endl;

	for (int iplot = 0; iplot<_correlation_backgroundsLocal.size(); iplot++)
		delete _correlation_backgroundsLocal[iplot];

	for (int iplot = 0; iplot<_correlations_backgroundlessLocal.size(); iplot++)
		delete _correlations_backgroundlessLocal[iplot];

	for (int iplot = 0; iplot<_differencesLocal.size(); iplot++)
		delete _differencesLocal[iplot];
	std::cout<<__LINE__<<std::endl;

	for (int iplot = 0; iplot<_differences_backgroundlessLocal.size(); iplot++)
		delete _differences_backgroundlessLocal[iplot];

	for (int iplot = 0; iplot<_correlationsGlobal.size(); iplot++)
		delete _correlationsGlobal[iplot];
	std::cout<<__LINE__<<std::endl;

	for (int iplot = 0; iplot<_correlation_backgroundsGlobal.size(); iplot++)
		delete _correlation_backgroundsGlobal[iplot];
	std::cout<<__LINE__<<std::endl;

	for (int iplot = 0; iplot<_correlations_backgroundlessGlobal.size(); iplot++)
		delete _correlations_backgroundlessGlobal[iplot];

	for (int iplot = 0; iplot<_differencesGlobal.size(); iplot++)
		delete _differencesGlobal[iplot];
	std::cout<<__LINE__<<std::endl;

	for (int iplot = 0; iplot<_differences_backgroundlessGlobal.size(); iplot++)
		delete _differences_backgroundlessGlobal[iplot];
	std::cout<<__LINE__<<std::endl;


}








//-----------------------------------------------------------------------------
