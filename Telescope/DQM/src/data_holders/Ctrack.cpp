#include "../../headers/data_holders/Ctrack.h"

//Ctrack.cpp - a class to store all information and some methods about a track.

//Author: Dan Saunders
//Date created: 15/10/13
//Last modified: 15/10/13




//-----------------------------------------------------------------------------

Ctrack::Ctrack(){
	_mx = 0.0;
	_cx = 0.0;
	_my = 0.0;
	_cy = 0.0;
	_gTOA = 0.0;
}




//-----------------------------------------------------------------------------

void Ctrack::set_by_clusters(std::vector<int> chip_excludes, bool excluding, int ID){
	//Fit straight line by clusters.
	_ID = ID;
	double S_x = 0.0, S_y = 0.0, S_z = 0.0, S_zx = 0.0, S_zy = 0.0, S_zz = 0.0;
	_gTOA = 0.0;
	//Fill them. Divide by (S=)N thing!
	std::vector<Ccluster*>::iterator iclust;
	int nclusters = 0;
	for (iclust = _clusters.begin(); iclust != _clusters.end(); iclust++){
		Ccluster * c = (*iclust);
		c->trackID = _ID;

		bool valid = true;
		if (excluding){
			for (int i=0; i<chip_excludes.size(); i++){
				if (chip_excludes[i] == c->get_chipID()) valid = false;
			}
		}

		if (valid) {

			S_x += c->get_gx();
			S_y += c->get_gy();
			S_z += c->get_gz();

			S_zz += c->get_gz()*c->get_gz();
			S_zx += c->get_gz() * c->get_gx();
			S_zy += c->get_gz() * c->get_gy();

			_gTOA += c->get_gt();
			//std::cout<<c->get_gt()<<std::endl;
			nclusters++;
		}
	}
	double S = (double) nclusters;
	_nclusters = nclusters;
	_gTOA /= (double) nclusters;
	//_gTOA = _clusters[0]->get_TOA();
	double del = S*S_zz - S_z*S_z;


	//Finally.
	_mx = (S*S_zx - S_z*S_x)/del;
	_my = (S*S_zy - S_z*S_y)/del;

	_cx = (S_zz*S_x - S_z*S_zx)/del;
	_cy = (S_zz*S_y - S_z*S_zy)/del;
}







//-----------------------------------------------------------------------------

int Ctrack::get_clustID_for_chipID(int ichip){
	int ID = -1;
	std::vector<Ccluster*>::iterator iclust;
	for (iclust = _clusters.begin(); iclust != _clusters.end(); iclust++){
		if ((*iclust)->get_chipID() == ichip) ID = (*iclust)->get_ID();
	}

	return ID;
}
