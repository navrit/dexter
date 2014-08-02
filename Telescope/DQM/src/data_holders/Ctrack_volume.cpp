#include "../../headers/data_holders/Ctrack_volume.h"
//Ctrack_volume.cpp - a class to store all information and some methods about a track.

//Author: Dan Saunders
//Date created: 15/10/13
//Last modified: 15/10/13




//-----------------------------------------------------------------------------

Ctrack_volume::Ctrack_volume(int nchips, int minNClusters){
	_nchips = nchips;
	_minNClusters = minNClusters;
	for (int i=0; i<_nchips; i++) {
		std::vector<Ccluster*> dummy;
		_clusters.push_back(dummy);
	}

//	_theta = 0.002;
//	_cyl_r = 0.06;
	_shape = "cylinder";
}







//-----------------------------------------------------------------------------

void Ctrack_volume::consider_cluster(Ccluster * c){
	//Takes a cluster, and considers whether its inside this track volume.
	//If so, add to the current cluster list. If not, do nothing.
	if (cluster_inside(c)) add_cluster(c);
}







//-----------------------------------------------------------------------------

bool Ctrack_volume::cluster_inside(Ccluster * c){
	bool inside = false;
	if (_shape == "cylinder") inside = cluster_inside_cylinder(c);
	else if (_shape == "diabolo") inside = cluster_inside_diabolo(c);

	return inside;
}







//-----------------------------------------------------------------------------

bool Ctrack_volume::cluster_inside_diabolo(Ccluster * c){
	bool inside = false;

	//double r = get_delr(c);
	double delx = fabs(_seed_clust->get_gx() - c->get_gx());
	double dely = fabs(_seed_clust->get_gy() - c->get_gy());


//	if (r < diabolo_r(c->get_gz()) &&
//		c->get_gt() > _seed_clust->get_gt() - _tcut &&
//		c->get_gt() < _seed_clust->get_gt() + _tcut) inside = true;

	if (delx < diabolo_rx(c->get_gz()) && dely < diabolo_ry(c->get_gz()) &&
		c->get_gt() > _seed_clust->get_gt() - _tcut &&
		c->get_gt() < _seed_clust->get_gt() + _tcut)
			inside = true;

	return inside;
}


//
//
////-----------------------------------------------------------------------------
//
//double Ctrack_volume::diabolo_r(double z){
//	double delz = fabs(_seed_clust->get_gz() - z); //z distance from pinch point.
//	return delz*_theta + _cyl_r;
//}
//



//-----------------------------------------------------------------------------

double Ctrack_volume::diabolo_rx(double z){
	double delz = fabs(_seed_clust->get_gz() - z); //z distance from pinch point.
	return delz*_thetax + _cyl_rx;
}



//-----------------------------------------------------------------------------

double Ctrack_volume::diabolo_ry(double z){
	double delz = fabs(_seed_clust->get_gz() - z); //z distance from pinch point.
	return delz*_thetay + _cyl_ry;
}





//-----------------------------------------------------------------------------

bool Ctrack_volume::cluster_inside_cylinder(Ccluster * c){
//	bool inside = false;
//
//	double r = get_delr(c);
//	//std::cout<<r<<"\t"<<_cyl_r<<std::endl;
//	if (r < _cyl_r &&
//		c->get_gt() > _seed_clust->get_gt() - _tcut &&
//		c->get_gt() < _seed_clust->get_gt() + _tcut) inside = true;
//
//	return inside;
}






//-----------------------------------------------------------------------------

void Ctrack_volume::fit_tracks(int ID){
	//For now, we'll take the closest on each plane, with the condition of 
	//having all bar 1 cluster tracks.
	//std::cout<<"\n\n"<<_seed_clust->get_gx()<<"\t"<<_seed_clust->get_gy()<<"\t"<<_seed_clust->get_gz()<<"\t"<<_seed_clust->get_gt()<<"\t"<<_seed_clust->get_chipID()<<std::endl;


	int num_clusters_left = 0; //Counter to see if enough to form a track.
	//Filter the farthest clusters until just one per chip.
	for (int i=0; i<_nchips; i++){
		if (_clusters[i].size() > 1) pop_far_clusters(_clusters[i]);
		num_clusters_left += _clusters[i].size();
	}


	if (num_clusters_left >= _minNClusters){
		Ctrack * t = new Ctrack();
		for (int i=0; i<_nchips; i++){
			if (_clusters[i].size() != 0) t->add_cluster(_clusters[i][0]);
			if (_clusters[i].size() != 0) _clusters[i][0]->set_tracked(1);
		}
		std::vector<int> non_excludes;
		t->set_by_clusters(non_excludes, false, ID);

		_tracks.push_back(t);
	}
}






//-----------------------------------------------------------------------------

void Ctrack_volume::pop_far_clusters(std::vector<Ccluster*> & clusts){
	//Assume closest is first.
	Ccluster * c = clusts[0];
	double closest_r = get_delr(c);


	//Find the closest, then empty the vector, and put the closest back.
	for (int i=1; i<clusts.size(); i++){
		double rc = get_delr(clusts[i]);
		if (rc < closest_r){ //If closer.
			closest_r = rc;
			c = clusts[i];
		}
	}

	clusts.clear();
	clusts.push_back(c);
}






//-----------------------------------------------------------------------------

double Ctrack_volume::get_delr(Ccluster * c){

	double delx = c->get_gx() - _seed_clust->get_gx();
	double dely = c->get_gy() - _seed_clust->get_gy();
	return pow(delx*delx + dely*dely, 0.5);
}






//-----------------------------------------------------------------------------

int Ctrack_volume::totalNClusters(){
	int n=0;
	for (unsigned int i=0; i<_clusters.size(); i++){
		n+=_clusters[i].size();
	}
	return n;
}






//-----------------------------------------------------------------------------
