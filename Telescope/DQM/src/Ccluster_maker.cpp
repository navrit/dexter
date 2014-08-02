
#include "../headers/Ccluster_maker.h"
#include <sstream>


//-----------------------------------------------------------------------------

 Ccluster_maker::Ccluster_maker(CDQM_options * ops){
	std::cout<<"Constructor of Ccluster_maker"<<std::endl;

	 _ops = ops;
	 _tel = NULL;
	 _tlow = -0;
	 _tup = 200;
	 _num_pixs_not_clusted = 0;
	 _save_file_name = "Default_file_name";
	 _COGweight = 1.0;
}



//-----------------------------------------------------------------------------

void Ccluster_maker::initialize(){
	std::cout<<"Initializer of Ccluster_maker"<<std::endl;
	//Default values.
	_save_file_name = _ops->save_file_name;
	_chip_loop_cut = _ops->chip_cut;
	_tlow = _ops->clust_maker_tlow;
	_tup = _ops->clust_maker_tup;
	_COGweight = _ops->COGweight;
}








//-----------------------------------------------------------------------------

 void Ccluster_maker::execute(Ctel_chunk * tel){
 	std::cout<<"\n\n------- Executing Ccluster_maker -------"<<std::endl;
 	_tel = tel;
 	_chips = _tel->get_chips();

 	for (int i=0; i<_chips.size(); i++){
 		make_chip_clusters(_chips[i]);
 		_chips[i]->fill_loc_ts_to_clustIDs();
 		_chips[i]->setup_clusters_by_glob_x();

 		std::cout<<"\N clusts on chip "<<i<<": "<<_chips[i]->get_nclusters();
 		std::cout<<"\t cf number of pix_hits/3: "<<_chips[i]->get_npix_hits()/(double)3<<std::endl;
 		
 		if (i==_chip_loop_cut) break;
 	}

 	std::cout<<"\n"<<std::endl;
}




//-----------------------------------------------------------------------------

void Ccluster_maker::finalize(){
}






//-----------------------------------------------------------------------------

 void Ccluster_maker::make_chip_clusters(Cchip* chip){
 	//Since we have time information, as we cycle through each pixel_hit, we 
 	//can define a small window in which to search for its cluster. For now,
 	//we just search until we find the edge around all clusters. Also allow
 	//for searching to be done around a seed by providing some cut for the
 	//searching to begin.

 	//Start by iterating over pixel_hits, defining a window around each (wide
 	//enough to contain one cluster).
	
 	_num_pixs_not_clusted = chip->get_npix_hits();
 	if (chip->get_npix_hits() != 0) {
 		for (std::vector<Cpix_hit*>::iterator ipix = chip->get_pix_hits().begin();
 			ipix != chip->get_pix_hits().end(); ipix++){


			//Ask if as big as a seed and not already clustered. Currently allows noisy.
			if (!(*ipix)->get_clustered()){
				std::vector<Cpix_hit*> mini_event;

				//Get all pixels in the mini event centered (according to the t cuts) around this pixel.
				fill_mini_event((*ipix), chip, mini_event);

				//Now evalute whats in the mini_event.
				eval_mini_event(mini_event, chip);
			}
		}
	}

 	chip->_num_clust_tsteps = chip->get_nclusters()/chip->nthingy;
 	chip->_num_clust_xsteps = chip->get_nclusters()/chip->nthingy;
	//test_loop(chip->get_pix_hits());
}








//-----------------------------------------------------------------------------

 void Ccluster_maker::fill_mini_event(Cpix_hit* pix, Cchip* chip, std::vector<Cpix_hit*> & mini_event){
	mini_event.push_back(pix);

	//Cycle over these pixels. 
	for (int i=pix->get_ID(); i<chip->get_npix_hits(); i++){


		//Check not already clustered.
		if (!chip->get_pix_hits()[i]->get_clustered()
			&& chip->get_pix_hits()[i]!= pix)
			mini_event.push_back(chip->get_pix_hits()[i]);
		

		//Check if not too far ahead (just for time saving).
		if (chip->get_pix_hits()[i]->get_TOA() - pix->get_TOA() > _tup) break;
	}	
}








//-----------------------------------------------------------------------------

 void Ccluster_maker::eval_mini_event(std::vector<Cpix_hit*> & pixs, Cchip* chip){
 	//Each mini event results in the creation of a single cluster.

	//This will always result in a new cluster.
	Ccluster * cluster = new Ccluster(_ops);
	cluster->set_chipID(chip->get_ID());
	cluster->set_ID(chip->get_nclusters());
	chip->add_cluster(cluster);

	Cpix_hit * seed_pix = pixs[0];
	cluster->add_pixel(seed_pix);

	seed_pix->set_clustered(true);
	_num_pixs_not_clusted--;


	//Add all those touching the square edge (not corners) of the seed from the mini event.
	add_edge_pixels(cluster, pixs, seed_pix);
	cluster->set_size(cluster->get_pix_hits().size());


	cluster->set_by_pixels(_COGweight); //sets other cluster attributes.
	double temp_lposn[4], temp_gposn[4];
	cluster->get_lposn(temp_lposn);
	chip->lposn_to_gposn(temp_lposn, temp_gposn);
	cluster->set_gposn(temp_gposn);

	
}








//-----------------------------------------------------------------------------

 Cpix_hit * Ccluster_maker::get_first_seed_pix(std::vector<Cpix_hit*> & pixs){
 	//Assume its the first.
 	Cpix_hit * seed_pix = pixs[0];
	for (std::vector<Cpix_hit*>::iterator ipix = pixs.begin(); ipix != pixs.end(); ipix++){
		if ((*ipix)->get_ADC() > seed_pix->get_ADC() && !(*ipix)->get_clustered()){
			seed_pix = (*ipix);
		}
	}

	return seed_pix;
}








//-----------------------------------------------------------------------------

void Ccluster_maker::add_edge_pixels(Ccluster * cluster, std::vector<Cpix_hit*> & pixs, 
	Cpix_hit * center_pix){

	//This function is often nested inside itself. Searches a plus sign shape around 
	//the center pixel.
	for (std::vector<Cpix_hit*>::iterator ipix = pixs.begin(); ipix != pixs.end(); ipix++){
		if (!(*ipix)->get_clustered()){ //not already clustered.
			if (pixels_touch_side(center_pix, (*ipix))){
				cluster->add_pixel((*ipix));
				(*ipix)->set_clustered(true);
				_num_pixs_not_clusted--;
				add_edge_pixels(cluster, pixs, (*ipix)); //iterative nests.
			}
		}
	}
}








//-----------------------------------------------------------------------------

bool Ccluster_maker::pixels_touch_side(Cpix_hit* pix1, Cpix_hit * pix2){
 	//Returns true if the two pixels share an edge.
 	bool touching = false;

 	//See if above eachother.
 	if (abs(pix1->get_row() - pix2->get_row()) == 1 &&
 		pix1->get_column() - pix2->get_column() == 0) touching = true;


 	else if (abs(pix1->get_column() - pix2->get_column()) == 1 &&
 		     pix1->get_row() - pix2->get_row() == 0) touching = true;



  	else if (abs(pix1->get_column() - pix2->get_column()) == 1 &&
 		     abs(pix1->get_row() - pix2->get_row()) == 1) touching = true;


 //  	if (pix1->get_chipID() == 8){
 //  		std::cout<<"\n"<<std::endl;
	//   	std::cout<<pix1->get_row()<<"\t"<<pix2->get_row()<<std::endl;
	//     std::cout<<pix1->get_column()<<"\t"<<pix2->get_column()<<std::endl;
	//     std::cout<<pix1->get_TOA()<<"\t"<<pix2->get_TOA()<<std::endl;
	//     std::cout<<touching<<std::endl;
	// }
 	return touching;
}








//-----------------------------------------------------------------------------

bool Ccluster_maker::pixels_touch_side_n_diag(Cpix_hit* pix1, Cpix_hit * pix2){
 	//Returns true if the two pixels share an edge.
 	bool touching = false;
 	int del_row = pix1->get_row() - pix2->get_row();
 	int del_col = pix1->get_column() - pix2->get_column();
 	if (del_row*del_row + del_col*del_col <=2) touching = true;

 	return touching;
}








//-----------------------------------------------------------------------------

void Ccluster_maker::test_loop(std::vector<Cpix_hit*> pixs){
	int num_close = 0;
 	for (std::vector<Cpix_hit*>::iterator ipix1 = pixs.begin(); ipix1 != pixs.end(); ipix1++){
 		for (std::vector<Cpix_hit*>::iterator ipix2 = pixs.begin(); ipix2 != pixs.end(); ipix2++){
 			if ((*ipix1) != (*ipix2)){
 				//X.
 				if (  fabs((*ipix1)->get_column() - (*ipix2)->get_column()) <= 1 &&
 					  fabs((*ipix1)->get_row() - (*ipix2)->get_row()) <= 1 &&
 					  fabs((*ipix1)->get_TOA() - (*ipix2)->get_TOA()) <= 0.1){
 					num_close++;
 				}
 			}
 		}
 	}
}








//-----------------------------------------------------------------------------

 Ccluster_maker::~Ccluster_maker(){
 	std::cout<<"Deleting Ccluster_maker."<<std::endl;
}








//-----------------------------------------------------------------------------
