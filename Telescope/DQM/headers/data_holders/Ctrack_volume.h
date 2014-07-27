//Author: Dan Saunders
//Date created: 15/10/13
//Last modified: 6/11/13


#ifndef __CTRACK_VOLUME_INCLUDED__
#define __CTRACK_VOLUME_INCLUDED__

#include <iostream>
#include "Ccluster.h"
#include <vector>
#include "string.h"
#include "Ctrack.h"


class Ctrack_volume{
private:
	//Track parameters.
	std::string				_shape;
	std::vector< std::vector<Ccluster*>	> _clusters; //weird, but handy.
	float 					_cyl_r;
	float					_tcut;
	std::vector<Ctrack*>	_tracks;
	int						_nchips;
	Ccluster * 				_seed_clust;
	float					_theta;
	int _minNClusters;


public:
	Ctrack_volume(int, int); //constructor (arguement is nchips).
	void					consider_cluster(Ccluster*);
	bool					cluster_inside(Ccluster*);
	bool					cluster_inside_cylinder(Ccluster*);
	bool					cluster_inside_diabolo(Ccluster*);
	float					diabolo_r(float);
	void					fit_tracks(int);

	//Setters and getters _____________________________________________________

	std::vector<Ctrack*> &	get_tracks(){return _tracks;}

	void 					set_cyl_r(float r) {_cyl_r = r;}

	void 					set_tcut(float t) {_tcut = t;}

	float					get_cyl_r() {return _cyl_r;}

	void					set_nchips(int n) {_nchips = n;}


	Ccluster*				get_seed_clust() {return _seed_clust;}

	void					add_cluster(Ccluster * c){_clusters[c->get_chipID()].push_back(c);}

	void					add_seed_cluster(Ccluster * c){
								_seed_clust = c;
								_clusters[c->get_chipID()].push_back(c);}

	void					set_shape(std::string s){_shape = s;}
	std::string &			get_shape(){return _shape;}

	float					get_delr(Ccluster*);

	void					set_theta(float t){_theta = t;}

	void					pop_far_clusters(std::vector<Ccluster*> &);


};

#endif
