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
	float 					_mx;
	float 					_cx;
	float 					_my;
	float					_cy;
	double 					_gTOA;
	int						_nclusters;

	//Handy to know which clusters form this track.
	std::vector<Ccluster*>	_clusters;


public:
	Ctrack(); //constructor.
	void					set_by_clusters(std::vector<int>, bool, int);
	int						get_clustID_for_chipID(int);
	float					gx(float z){return _mx*z + _cx;}
	float					gy(float z){return _my*z + _cy;}

	//Setters and getters _____________________________________________________

	void 					set_ID(float ID) {_ID = ID;}
	float					get_ID() {return _ID;}

	int						get_size(){return _nclusters;}

	void 					set_mx(float mx) {_mx = mx;}
	float					get_mx() {return _mx;}

	void 					set_cx(float cx) {_cx = cx;}
	float					get_cx() {return _cx;}

	void 					set_my(float my) {_my = my;}
	float					get_my() {return _my;}

	void 					set_cy(float cy) {_cy = cy;}
	float					get_cy() {return _cy;}

	void 					set_gTOA(float gTOA) {_gTOA = gTOA;}
	float					get_gTOA() {return _gTOA;}

	void 					set_gt(float gTOA) {_gTOA = gTOA;}
	float					get_gt() {return _gTOA;}

	void 					set_nclusters(float nclusters) {_nclusters = nclusters;}
	int						get_nclusters() {return _nclusters;}


	void 					set_clusters(std::vector<Ccluster*> cs){_clusters = cs;}
	std::vector<Ccluster*>& get_clusters() {return _clusters;}
	void					add_cluster(Ccluster * c){_clusters.push_back(c);}



};

#endif
