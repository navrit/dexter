#include "../headers/Ctrack_maker.h"
#include <sstream>


//-----------------------------------------------------------------------------

Ctrack_maker::Ctrack_maker(CDQM_options * ops){
	std::cout<<"Constructor of Ctrack_maker."<<std::endl;
	_ops = ops;
}






//-----------------------------------------------------------------------------

void Ctrack_maker::initialize(){
	//Default values.
	_save_file_name = _ops->save_file_name;
	_ref_chips = _ops->ref_chips;
	_chip_loop_cut = _ops->chip_cut;
	_tcut = _ops->track_delt;
	_cyl_r = _ops->track_vol_r;
	_theta = _ops->track_vol_theta;
	if (_ops->track_vol_shape == 1) _track_vol_shape = "diabolo";
	else _track_vol_shape = "cylinder";
}



//-----------------------------------------------------------------------------

void Ctrack_maker::finalize(){
}








//-----------------------------------------------------------------------------

void Ctrack_maker::execute(Ctel_chunk * tel){ 
	_tels.push_back(tel);
	std::cout<<"\n\n------- Executing Ctrack_maker -------"<<std::endl;

	for (int itel=0; itel<_tels.size(); itel++){
	 	for (std::vector<int>::iterator i = _ref_chips.begin(); i != _ref_chips.end(); i++){
	 		std::cout<<"Searching over chip: "<<(*i)<<std::endl;
	 		search_chip((*i), itel);
		}
	}

	std::cout<<"Num tracks found: "<<_tels[0]->get_ntracks()<<
		"\tcf ref_chip nclusters: "<<_tels[0]->get_chip(_ref_chips[0])->get_nclusters()<<std::endl;


	_tels.clear();
}








//-----------------------------------------------------------------------------

void Ctrack_maker::search_chip(int ichip, int itel){
 	//Need to search over clusters on this chip. If a cluster is not tracked, form a 
 	//(4)volume around it, add to the volume, and evaluate.
	int n = 0;
	int npouts = 0;
	for (unsigned int i=0; i<100; i++) std::cout<<"_";
	std::cout<<"\n";
 	std::vector<Ccluster*>::iterator iclust;
 	if (_tels[itel]->get_chip(ichip)->get_nclusters() != 0) {
		for (iclust = _tels[itel]->get_chip(ichip)->get_clusters().begin();
			 iclust != _tels[itel]->get_chip(ichip)->get_clusters().end(); iclust++){

			if (n/(float)_tels[itel]->get_chip(ichip)->get_nclusters() >0.01*npouts) {
				std::cout<<"*"<<std::flush;
				npouts++;
			}
			if ((*iclust)->get_tracked() == 0){
				Ctrack_volume * vol = new Ctrack_volume(std::min(_tels[0]->get_nchips(), _chip_loop_cut), _ops->minNClusterPerTrack);
				vol->set_cyl_r(_cyl_r);

				//std::cout<<_cyl_r<<"\t"<<vol->get_cyl_r()<<std::endl;
				vol->set_theta(_theta);
				vol->set_tcut(_tcut);
				vol->set_shape(_track_vol_shape);

				vol->add_seed_cluster(*iclust); //(*iclust) not necassarily tracked yet.
				fill_seeded_vol(vol);
				vol->fit_tracks(_tels[0]->get_ntracks()); //sets the clusters too.
				add_tracks(vol); 

				delete vol;
			}
			n++;
		}
	}
 	std::cout<<"\n";
}








//-----------------------------------------------------------------------------

void Ctrack_maker::fill_seeded_vol(Ctrack_volume * vol){
 	//Fill the volume with clusters inside the volume.
	float vol_TOA = vol->get_seed_clust()->get_gt();


	//Search over tels.
 	for (std::vector<Ctel_chunk*>::iterator itel = _tels.begin();
		itel != _tels.end(); ++itel){


 		//Search all chips.
	 	for (std::vector<Cchip*>::iterator ichip = (*itel)->get_chips().begin();
			ichip != (*itel)->get_chips().end(); ++ichip){

	 		if ((*ichip)->get_nclusters() != 0) {
		 		std::vector<Ccluster*> clusters = (*ichip)->get_clusters();
		 		int istart = (*ichip)->glob_t_to_clustID(vol_TOA - _tcut);
		 		for (int i=istart; i<clusters.size(); i++){
		 			Ccluster * c = clusters[i];

		 			//Now decide on this cluster, of this chip, of this tel.
		 			if (c->get_tracked() == 0) vol->consider_cluster(c);


		 			//Stop looping this chip if too far ahead.
		 			if (c->get_gt() > vol_TOA + _tcut) break;
		 		}
	 		}


	 		//Chip loop cut.
	 		if ((*ichip)->get_ID() == _chip_loop_cut) break;
	 	}
	}

}








//-----------------------------------------------------------------------------

void Ctrack_maker::add_tracks(Ctrack_volume * vol){
 	for (int i=0; i<vol->get_tracks().size(); i++){
 		//ASSUME FIRST TEL.
 		vol->get_tracks()[i]->set_ID(_tels[0]->get_ntracks());
 		_tels[0]->add_track(vol->get_tracks()[i]);
 	}
}








//-----------------------------------------------------------------------------

Ctrack_maker::~Ctrack_maker(){
 	std::cout<<"\n\nDeleting Ctrack_maker."<<std::endl;
 	Chandy::dash_line_break();
}








//-----------------------------------------------------------------------------
