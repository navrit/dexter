#include "../../headers/data_holders/Cchip.h"


//Cchip.cpp - a class to store all information on a timepix3/Velopix.

//Author: Dan Saunders
//Date created: 15/10/13
//Last modified: 15/10/13




//-----------------------------------------------------------------------------



Cchip::Cchip(CDQM_options * ops){
	_ops = ops;
	//std::cout<<"Constructor of a chip."<<std::endl;
	_name = "my_chip";
	_ID = 0;
	_gposn[0] = 0.0; _gposn[1] = 0.0; _gposn[2] = 0.0; _gposn[3] = 0.0;
	_orientation[0] = 0.0; _orientation[1] = 0.0; _orientation[2] = 0.0;
	_rotn = 0.0;
	_npix_hits = 0;
	_nclusters = 0;
	_pixel_width = 0.055; //in mm's.
	_pixel_height = 0.055; //in mm's.
	_chip_width = 14.08; //in mm's.
	_chip_height = 14.08; //in mm's.

	_n_pixs_side = 256;
	
	_size_pixs[0] = 256; _size_pixs[1] = 256; //in pixs.
	_size_mms[0] = 256 * get_pixel_width(); 
	_size_mms[1] = 256 * get_pixel_width();

	_hit_map = new std::vector< std::vector< std::vector<int> > >();
	_hit_map_filled = false;

	_tpixzero = 0.0;
	_tpixstep = 100.0;
	_num_pix_tsteps = 500;
	nthingy = 2;
	_num_clust_tsteps = _ops->nPixHitCut/nthingy;



	_xpixzero = 0.0;
	_xpixstep = 100.0;
	_num_pix_xsteps = _ops->nPixHitCut/nthingy;

}




//-----------------------------------------------------------------------------

void Cchip::setup_pix_hits_by_glob_x(){
	if (get_npix_hits() != 0) {
		_pix_hits_by_glob_x.clear();
		//Fills the alternative order of clusters (by x position as opposed to t) - 
		//very useful for drawing t correlations.

		//Start by filling a copy.
		for (int i=0; i<_npix_hits; i++) _pix_hits_by_glob_x.push_back(_pix_hits[i]);
		
		x_order_pix_hits_comb();
		fill_glob_xs_to_pixIDs();
	}
}







//-----------------------------------------------------------------------------

void Cchip::setup_clusters_by_glob_x(){
	if (get_nclusters() != 0) {
		_clusters_by_glob_x.clear();
		//Fills the alternative order of clusters (by x position as opposed to t) - 
		//very useful for drawing t correlations.

		//Start by filling a copy.
		for (int i=0; i<_nclusters; i++) _clusters_by_glob_x.push_back(_clusters[i]);
		
		x_order_clusts_comb();
		fill_glob_xs_to_clustIDs();
	}
}







//-----------------------------------------------------------------------------

void Cchip::fill_glob_xs_to_clustIDs(){
	//Fills the map between ts and pixIDs (relative to the beginning of the 
	//current set of pix_hits TOAs).
	_glob_xs_to_clustIDs.clear();

	_xclustzero = _clusters_by_glob_x[0]->get_gx();
	_xcluststep = (_clusters_by_glob_x.back()->get_gx() - _xclustzero)/(float)_num_clust_xsteps;
	_glob_xs_to_clustIDs.push_back(0);


	int istep = 1; //dummy counter.
	//Cycle over pixels to, finding a corresponding pix_hit_ID for each tstep.
	float x;
	for (int i=1; i<_nclusters; i++){
		while (_clusters_by_glob_x[i]->get_gx()-_xclustzero > istep*_xcluststep){ //i.e. if a new step.
			istep++;
			_glob_xs_to_clustIDs.push_back(i);
		}
	}
}







//-----------------------------------------------------------------------------

void Cchip::fill_glob_xs_to_pixIDs(){
	//Fills the map between ts and pixIDs (relative to the beginning of the 
	//current set of pix_hits).
	_glob_xs_to_pixIDs.clear();

	//Start by getting the lower x and step width.
	_xpixzero = get_pix_dir(_pix_hits_by_glob_x[0], 0);
	_xpixstep = (get_pix_dir(_pix_hits_by_glob_x.back(), 0) - _xpixzero)/(float)_num_pix_xsteps;
	_glob_xs_to_pixIDs.push_back(0);


	int istep = 1; //dummy counter.
	float x;
	//Cycle over pixels to, finding a corresponding pix_hit_ID for each tstep.
	for (int i=1; i<_npix_hits; i++){
		x = get_pix_dir(_pix_hits_by_glob_x[i], 0);
		while (x - _xpixzero > istep*_xpixstep){ //i.e. if a new step (OR TWO!).
			//Find how many jumps were actually made.
			istep++;
			_glob_xs_to_pixIDs.push_back(i);
			
		}
	}
}







//-----------------------------------------------------------------------------

void Cchip::fill_loc_ts_to_pixIDs(){
	//Fills the map between ts and pixIDs (relative to the beginning of the 
	//current set of pix_hits TOAs).
	_loc_ts_to_pixIDs.clear();

	_tpixzero = _pix_hits[0]->get_TOA();
	_tpixstep = (_pix_hits.back()->get_TOA() - _tpixzero)/(float)_num_pix_tsteps;
	_loc_ts_to_pixIDs.push_back(0);


	int istep = 1; //dummy counter.
	int i=0;
	//Cycle over pixels to, finding a corresponding pix_hit_ID for each tstep.
	for (std::vector<Cpix_hit*>::iterator ipix = get_pix_hits().begin()+1;
		 ipix != get_pix_hits().end(); ipix++){

		while ((*ipix)->get_TOA()-_tpixzero > istep*_tpixstep){ //i.e. if a new step.
			istep++;
			_loc_ts_to_pixIDs.push_back(i);
		}
		i++;
	}
}







//-----------------------------------------------------------------------------

int Cchip::glob_t_to_pixID(float t){
	//Returns the ID of a pix_hit nearest (and less than) the given time t,
	//as long as time ordering of pix_hits is maintained.

	//first move into local codn system.
	t -= _gposn[3];


	int t_ID = floor((t-_tpixzero)/_tpixstep);
	if (t_ID < 0) t_ID = 0;
	else if (t_ID > _num_pix_tsteps-1) t_ID = _num_pix_tsteps-1;
	return _loc_ts_to_pixIDs[t_ID];
}







//-----------------------------------------------------------------------------

void Cchip::fill_loc_ts_to_clustIDs(){
	//Fills the map between ts and pixIDs (relative to the beginning of the 
	//current set of pix_hits TOAs).
	if (get_nclusters() != 0) {
		_loc_ts_to_clustIDs.clear();

		_tclustzero = _clusters[0]->get_TOA();
		_tcluststep = (_clusters.back()->get_TOA() - _tclustzero)/(float)_num_clust_tsteps;
		_loc_ts_to_clustIDs.push_back(0);


		int istep = 1; //dummy counter.
		//Cycle over pixels to, finding a corresponding pix_hit_ID for each tstep.
		int i = 0;
		for (std::vector<Ccluster*>::iterator iclust = get_clusters().begin()+1;
			 iclust != get_clusters().end(); iclust++){

			while ((*iclust)->get_TOA()-_tclustzero > istep*_tcluststep){ //i.e. if a new step.
				istep++;
				_loc_ts_to_clustIDs.push_back(i);
			}
			i++;
		}
	}
}







//-----------------------------------------------------------------------------

int Cchip::glob_t_to_clustID(float t){
	//Returns the ID of a pix_hit nearest (and less than) the given time t,
	//as long as time ordering of pix_hits is maintained.

	t -= _gposn[3];

	int t_ID = floor((t-_tclustzero)/_tcluststep);
	if (t_ID < 0) t_ID = 0;
	else if (t_ID > _num_clust_tsteps-1) t_ID = _num_clust_tsteps-1;
	return _loc_ts_to_clustIDs[t_ID];
}







//-----------------------------------------------------------------------------

int Cchip::glob_x_to_pixID(float x){
	//Returns the ID of a pix_hit nearest (and less than) the given time t,
	//as long as time ordering of pix_hits is maintained.

	_xpixzero = get_pix_dir(_pix_hits_by_glob_x[0], 0);

	int x_ID = floor((x-_xpixzero)/_xpixstep);
	if (x_ID < 0) x_ID = 0;
	else if (x_ID > _num_pix_xsteps-1) x_ID = _num_pix_xsteps-1;
	return _glob_xs_to_pixIDs[x_ID];
}







//-----------------------------------------------------------------------------

int Cchip::glob_x_to_clustID(float x){
	//Returns the ID of a pix_hit nearest (and less than) the given time t,
	//as long as time ordering of pix_hits is maintained.

	_xclustzero = _clusters_by_glob_x[0]->get_gx();

	int x_ID = floor((x -_xclustzero)/_xcluststep);
	if (x_ID < 0) x_ID = 0;
	else if (x_ID > _num_clust_xsteps-1) x_ID = _num_clust_xsteps-1;
	return _glob_xs_to_clustIDs[x_ID];
}







//-----------------------------------------------------------------------------

int Cchip::num_active_hits(){
	//Returns the number of hits that are still valid.

	int N=0;
	for (std::vector<Cpix_hit*>::iterator ipix = get_pix_hits().begin();
		 ipix != get_pix_hits().end(); ipix++){
		if ((*ipix)->get_valid()) N+=1;
	} 
	return N;
}







//-----------------------------------------------------------------------------

void Cchip::rm_hot_pixels(int sigma_cut){
	//Turns off all pix_hits that are beyond a frequency cut measured in sigmas.


	//Start by getting a pixel frequency map.
	if (_hit_map->size() == 0) fill_hit_map();


	for (int i=0; i<_hit_map->size(); i++){
		for (int j=0; j<(*_hit_map)[0].size(); j++){
			if ((*_hit_map)[i][j].size()>=sigma_cut){

				//Turn these pixel hits off.
				for (int k=0; k<(*_hit_map)[i][j].size(); k++){
					Cpix_hit * ipix = _pix_hits[(*_hit_map)[i][j][k]];
					ipix->set_valid(false);
				}
			}
		}
	}
	num_active_hits();
	fill_hit_map();
}







//-----------------------------------------------------------------------------

void Cchip::fill_hit_map(){
	//Fills a frequency hit map over a pixel.
	_hit_map->clear();


	//Make the empty map.
	for (int icol = 0; icol < _size_pixs[0]; icol++){
		_hit_map->push_back(std::vector<std::vector<int> >());

		//Fill this column.
		for (int irow = 0; irow < _size_pixs[1]; irow++){
			_hit_map->operator[](icol).push_back(std::vector<int>());
		}
	}

	
	//Now fill it with pixel pointers.
	for (std::vector<Cpix_hit*>::iterator ipix = get_pix_hits().begin();
		 ipix != get_pix_hits().end(); ipix++){
		int column = (*ipix)->get_column();
		int row = (*ipix)->get_row();

		if ((*ipix)->get_valid()){
			(*_hit_map)[column][row].push_back((*ipix)->get_ID());
		}
	}
	_hit_map_filled = true;
}







//-----------------------------------------------------------------------------

int Cchip::get_min_cluster_ADC(){
	//assume the first is the smallest, then iterate.
	int x = _clusters[0]->get_ADC();

	//make an iterator for the clusters vector. Pointer to pointer.
	for (std::vector<Ccluster*>::iterator iclus = _clusters.begin();
			iclus != _clusters.end(); ++iclus){

		//if smaller, then replace.
		if ((*iclus)->get_ADC() < x) x = (*iclus)->get_ADC();
	}
	return x;
}







//-----------------------------------------------------------------------------


int Cchip::get_max_cluster_ADC(){
	//Assume the first is the biggest, then iterate.
	int x = _clusters[0]->get_ADC();

	//Make an iterator for the clusters vector. Pointer to pointer.
	for (std::vector<Ccluster*>::iterator iclus = _clusters.begin();
			iclus != _clusters.end(); ++iclus){

		//If bigger, then replace.
		if ((*iclus)->get_ADC() > x) x = (*iclus)->get_ADC();
	}
	return x;
}







//-----------------------------------------------------------------------------

int Cchip::get_min_pixel_ADC(){
	//assume the first is the smallest, then iterate.
	int x = _pix_hits[0]->get_ADC();

	//make an iterator for the clusters vector. Pointer to pointer.
	for (std::vector<Cpix_hit*>::iterator ipix = _pix_hits.begin();
			ipix != _pix_hits.end(); ++ipix){

		//If smaller, then replace.
		if ((*ipix)->get_ADC() < x) x = (*ipix)->get_ADC();
	}

	return x;
}







//-----------------------------------------------------------------------------

int Cchip::get_max_pixel_ADC(){
	//assume the first is the smallest, then iterate.
	int x = _pix_hits[0]->get_ADC();

	//make an iterator for the clusters vector. Pointer to pointer.
	for (std::vector<Cpix_hit*>::iterator ipix = _pix_hits.begin();
			ipix != _pix_hits.end(); ++ipix){

		//If bigger, then replace.
		if ((*ipix)->get_ADC() > x) x = (*ipix)->get_ADC();
	}

	return x;
}







//-----------------------------------------------------------------------------

void Cchip::print_details(){
	//Handy function for printing chip details out.
	std::cout<<"\nChip details:"<<std::endl;
	std::cout<<"chip name:"<<"\t"<<_name<<std::endl;
	std::cout<<"chip ID:"<<"\t"<<_ID<<std::endl;
	std::cout<<"posn:"<<"\t\t"<<_gposn[0]<<" "<<_gposn[1]<<" "<<_gposn[2]<<std::endl;
	std::cout<<"orientation:"<<"\t"<<_orientation[0]<<" "<<_orientation[1]<<" "<<_orientation[2]<<std::endl;
	std::cout<<"rotn:"<<"\t\t"<<_rotn<<std::endl;
	std::cout<<"npix_hits:"<<"\t"<<_npix_hits<<std::endl;
	std::cout<<"n_clusters:"<<"\t"<<_nclusters<<"\n"<<std::endl;
}







//-----------------------------------------------------------------------------

float Cchip::get_average_cluster_ADC(){
	float mu = 0.0;
	for (std::vector<Ccluster*>::iterator iclus = _clusters.begin();
			iclus != _clusters.end(); ++iclus){
		mu += (*iclus)->get_ADC();
	}

	mu /= _nclusters;
	return mu;
}







//-----------------------------------------------------------------------------

float Cchip::get_average_pixel_ADC(){
	float mu = 0.0;
	for (std::vector<Cpix_hit*>::iterator ipix = get_pix_hits().begin();
			ipix != get_pix_hits().end(); ++ipix){
		mu += (*ipix)->get_ADC();
	}

	mu /= get_npix_hits();
	return mu;
}







//-----------------------------------------------------------------------------

float Cchip::get_std_cluster_ADC(){
	float sig = 0.0;
	float mu = get_average_cluster_ADC();
	float my_mu_sq = mu*mu;
	for (std::vector<Ccluster*>::iterator iclus = get_clusters().begin();
			iclus != get_clusters().end(); ++iclus){
		sig += pow((double)((*iclus)->get_ADC() - my_mu_sq), 2.0);
	}

	sig /= get_nclusters();
	sig = pow(sig, 0.5);
	return (float) sig;
}







//-----------------------------------------------------------------------------

void Cchip::set_std_and_average_cluster_ADC(float & mu, float & sig){
	sig = 0.0;
	mu = get_average_cluster_ADC();
	float my_mu_sq = mu*mu;
	for (std::vector<Ccluster*>::iterator iclus = _clusters.begin();
			iclus != _clusters.end(); ++iclus){
		sig += (float)pow((double)((*iclus)->get_ADC() - my_mu_sq), 2.0);
	}

	sig /= get_nclusters();
	sig = pow(sig, 0.5);
	sig = (float) sig;
}







//-----------------------------------------------------------------------------

void Cchip::set_std_and_average_pixel_ADC(float & mu, float & sig){
	sig = 0.0;
	mu = get_average_pixel_ADC();
	float my_mu_sq = mu*mu;
	for (std::vector<Cpix_hit*>::iterator ipix = _pix_hits.begin();
			ipix != _pix_hits.end(); ++ipix){
		sig += (float)pow((double)((*ipix)->get_ADC() - my_mu_sq), 2.0);
	}

	sig /= get_npix_hits();
	sig = pow(sig, 0.5);
	sig = (float) sig;
}







//-----------------------------------------------------------------------------

void Cchip::reset_cluster_IDs(){
	//sets the cluster_IDs to be the index in the list.
	for (int iclus = 0; iclus<get_nclusters(); iclus++){
		get_clusters()[iclus]->set_ID(iclus);
	}
}







//-----------------------------------------------------------------------------

void Cchip::reset_pixel_IDs(){
	//sets the pixel_IDs to be the index in the list.
	for (int ipix = 0; ipix<get_npix_hits(); ipix++){
		_pix_hits[ipix]->set_ID(ipix);
	}
}







//----------------------------------------------------------------------------

void Cchip::gposn_to_lposn(float gposn[4], float lposn[4]){
	float spatial_lposn[3] = {gposn[0]-_gposn[0], gposn[1]-_gposn[1], gposn[2]-_gposn[2]};
	Chandy::R_x(spatial_lposn, -_orientation[0]);
	Chandy::R_y(spatial_lposn, -_orientation[1]);
	Chandy::R_z(spatial_lposn, -_orientation[2]);

	lposn[0] = spatial_lposn[0]/get_pixel_width(); 
	lposn[1] = spatial_lposn[1]/get_pixel_height();
	lposn[2] = 0.0;
	lposn[3] = gposn[3] - _gposn[3];
}







//-----------------------------------------------------------------------------

void Cchip::lposn_to_gposn(float lposn[4], float gposn[4]){
	//Returns the global position of a given posn in this local codn system.

	//Start by putting lposn into a 3 vector, in l codn system, in mm.
	float spatial_gposn[3] = {lposn[0] * get_pixel_width(),
					  		  lposn[1] * get_pixel_height(),
					  		  0.0}; //zposn in a chip frame is zero by definition.


	//First rotate.
	Chandy::R_z(spatial_gposn, _orientation[2]);
	Chandy::R_y(spatial_gposn, _orientation[1]);
	Chandy::R_x(spatial_gposn, _orientation[0]);


	//Finally, translate.
	gposn[0] = spatial_gposn[0] + _gposn[0]; 
	gposn[1] = spatial_gposn[1] + _gposn[1]; 
	gposn[2] = spatial_gposn[2] + _gposn[2];
	gposn[3] = lposn[3] + _gposn[3];

}







//-----------------------------------------------------------------------------

std::vector< std::vector< std::vector<int> > >* Cchip::get_hit_map(){
	if (_hit_map_filled == false) fill_hit_map();	
	return _hit_map;
}







//-----------------------------------------------------------------------------

void Cchip::x_order_pix_hits_comb(){
	//GLOBAL.
	//Need to do several iterations over the cluslters according to some
	//shrink factor. Initially this is the length of the list divided by
	//1.3, and we divide by 1.3 each time until we do an iteration where
	//the shrink factor is one.

	int N = _npix_hits;
	float s_factor = 1.3;
	Cpix_hit* temp_pixel;
	int gap = N / s_factor;
	bool swapped = false;


	//Start the iterations.
	while (gap > 1 || swapped){
		if (gap > 1) gap /= s_factor;

		swapped = false; //reset per iteration.

		//do the iterations.
		for (int i = 0; gap + i < N; ++i) {

			if (get_pix_dir(_pix_hits_by_glob_x[i], 0) > get_pix_dir(_pix_hits_by_glob_x[i+gap], 0)){

				//Do the swap.
				temp_pixel = _pix_hits_by_glob_x[i];
				_pix_hits_by_glob_x[i] = _pix_hits_by_glob_x[i + gap];
				_pix_hits_by_glob_x[i + gap] = temp_pixel;

				swapped = true;
			}
		}
	}
}






//-----------------------------------------------------------------------------

void Cchip::x_order_clusts_comb(){

	//Need to do several iterations over the cluslters according to some
	//shrink factor. Initially this is the length of the list divided by
	//1.3, and we divide by 1.3 each time until we do an iteration where
	//the shrink factor is one.

	int N = _nclusters;
	float s_factor = 1.3;
	Ccluster* temp_clust;
	int gap = N / s_factor;
	bool swapped = false;


	//start the iterations.
	while (gap > 1 || swapped){
		if (gap > 1) gap /= s_factor;

		swapped = false; //reset per iteration.

		//do the iterations.
		for (int i = 0; gap + i < N; ++i) {
			if (_clusters_by_glob_x[i]->get_gx() > _clusters_by_glob_x[i+gap]->get_gx()){

				//Do the swap.
				temp_clust = _clusters_by_glob_x[i];
				_clusters_by_glob_x[i] = _clusters_by_glob_x[i + gap];
				_clusters_by_glob_x[i + gap] = temp_clust;

				swapped = true;
			}
		}
	}
}






//-----------------------------------------------------------------------------

void Cchip::time_order_pix_hits_comb(){

	//Need to do several iterations over the cluslters according to some
	//shrink factor. Initially this is the length of the list divided by
	//1.3, and we divide by 1.3 each time until we do an iteration where
	//the shrink factor is one.
	if (get_npix_hits() != 0) {
		int N = _npix_hits;
		float s_factor = 1.3;
		Cpix_hit* temp_pixel;
		int gap = N / s_factor;
		bool swapped = false;


		//start the iterations.
		while (gap > 1 || swapped){
			if (gap > 1) gap /= s_factor;

			swapped = false; //reset per iteration.

			//do the iterations.
			for (int i = 0; gap + i < N; ++i) {
				if (_pix_hits[i]->get_TOA() > _pix_hits[i + gap]->get_TOA()){

					//Do the swap.
					temp_pixel = _pix_hits[i];
					_pix_hits[i] = _pix_hits[i + gap];
					_pix_hits[i + gap] = temp_pixel;

					swapped = true;
				}
			}
		}
		reset_pixel_IDs();
		fill_loc_ts_to_pixIDs();
		std::cout<<"Chip: "<<get_ID()<<"\tt_lowest: "<<_pix_hits[0]->get_TOA()<<"\tt_greatest: "<<_pix_hits[_pix_hits.size()-1]->get_TOA()<<std::endl;
	}
}






//-----------------------------------------------------------------------------

void Cchip::time_order_clusts_comb(){

	//Need to do several iterations over the cluslters according to some
	//shrink factor. Initially this is the length of the list divided by
	//1.3, and we divide by 1.3 each time until we do an iteration where
	//the shrink factor is one.
	if (get_nclusters()!=0) {
		int N = _nclusters;
		float s_factor = 1.3;
		Ccluster* temp_clust;
		int gap = N / s_factor;
		bool swapped = false;


		//start the iterations.
		while (gap > 1 || swapped){
			if (gap > 1) gap /= s_factor;

			swapped = false; //reset per iteration.

			//do the iterations.
			for (int i = 0; gap + i < N; ++i) {
				if (	_clusters[i]->get_TOA()
						>
						_clusters[i + gap]->get_TOA()){

					//Do the swap.
					temp_clust = _clusters[i];
					_clusters[i] = _clusters[i + gap];
					_clusters[i + gap] = temp_clust;

					swapped = true;
				}
			}
		}
		reset_cluster_IDs();
		fill_loc_ts_to_clustIDs();
	}
}






//-----------------------------------------------------------------------------

void Cchip::fill_cluster_sample(int N, std::vector<Ccluster*> & cluster_sample){
	//Returns a vector of size N containing a sample of clusters (ID evenly spaced).

	if (get_nclusters() > N) {
		int spacer = get_nclusters()/N;
		for (int i=0; i<N; i++) {
			cluster_sample.push_back(get_clusters()[i*spacer]);
		}
	}

}






//-----------------------------------------------------------------------------

float Cchip::get_pix_dir(Cpix_hit * pix, int dir){
	float temp_lposn[4], temp_gposn[4];
	pix->get_lposn(temp_lposn);
	this->lposn_to_gposn(temp_lposn, temp_gposn);
	return temp_gposn[dir];
}






//-----------------------------------------------------------------------------

void Cchip::truncate(int n){
	//Removes all bar first n pix hits (t ordered already).
	// for (int i=n; n<_npix_hits; i++){
	// 	delete _pix_hits[i];
	// }

	while (_pix_hits.size() > n) _pix_hits.pop_back();

	std::cout<<"Truncated from "<<_npix_hits<<" to "<<_pix_hits.size()<<std::endl;
	_npix_hits = n;
	fill_loc_ts_to_pixIDs();
	setup_pix_hits_by_glob_x();
}






//-----------------------------------------------------------------------------

Cchip::~Cchip(){
	//Need to delete the pixels, clusters, correl lines and TH2Fs.

	for (std::vector<Cpix_hit*>::iterator ipix = get_pix_hits().begin();
			ipix != get_pix_hits().end(); ++ipix){
		if ((*ipix) != NULL) delete (*ipix);
	}


	for (std::vector<Ccluster*>::iterator iclus = _clusters.begin();
			iclus != _clusters.end(); ++iclus){
		if (((*iclus)) != NULL) delete (*iclus);
	}

}






//-----------------------------------------------------------------------------


