//Author: Dan Saunders
//Date created: 15/10/13
//Last modified: 6/11/13


#ifndef __CTRACK_INCLUDED__
#define __CTRACK_INCLUDED__

#include <iostream>
#include "Ccluster.h"
#include <vector>


class Ctrack{
private:
	//Track parameters.
	int						_ID;
	double 					_mx;
	double 					_cx;
	double 					_my;
	double					_cy;
	double 					_gTOA;
	int						_nclusters;

	//Handy to know which clusters form this track.
	std::vector<Ccluster*>	_clusters;


public:
	Ctrack(); //constructor.
	void					set_by_clusters(std::vector<int>, bool, int);
	int						get_clustID_for_chipID(int);
	double					gx(double z){return _mx*z + _cx;}
	double					gy(double z){return _my*z + _cy;}

	//Setters and getters _____________________________________________________

	void 					set_ID(double ID) {_ID = ID;}
	double					get_ID() {return _ID;}

	int						get_size(){return _nclusters;}

	void 					set_mx(double mx) {_mx = mx;}
	double					get_mx() {return _mx;}

	void 					set_cx(double cx) {_cx = cx;}
	double					get_cx() {return _cx;}

	void 					set_my(double my) {_my = my;}
	double					get_my() {return _my;}

	void 					set_cy(double cy) {_cy = cy;}
	double					get_cy() {return _cy;}

	void 					set_gTOA(double gTOA) {_gTOA = gTOA;}
	double					get_gTOA() {return _gTOA;}

	void 					set_gt(double gTOA) {_gTOA = gTOA;}
	double					get_gt() {return _gTOA;}

	void 					set_nclusters(double nclusters) {_nclusters = nclusters;}
	int						get_nclusters() {return _nclusters;}


	void 					set_clusters(std::vector<Ccluster*> cs){_clusters = cs;}
	std::vector<Ccluster*>& get_clusters() {return _clusters;}
	void					add_cluster(Ccluster * c){_clusters.push_back(c);}

	double 					get_impactAngle(){return pow(_mx*_mx + _my*_my, 0.5);}



};

#endif
