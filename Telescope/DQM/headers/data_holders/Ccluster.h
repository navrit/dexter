//Ccluster.cpp - a class to store all information for a single cluster.

//Author: Dan Saunders
//Date created: 15/10/13
//Last modified: 13/01/13


#ifndef __CCLUSTER_H_INCLUDED__
#define __CCLUSTER_H_INCLUDED__


#include <vector>
#include <iostream>
#include "Cpix_hit.h"
#include "math.h"
#include "TH2F.h"
#include "TH1F.h"
#include "TFile.h"
#include <sstream>
#include "TRandom3.h"
#include "TGraph.h"
#include "../../CDQM_options.h"


class Ccluster{
private:
	//Position and arrival information - no need to actually store the z posn,
	//since its zero in the local co-ordinate system.
	int						_ID;
	int						_chipID;
	double					_lposn[3];
	double					_gposn[4];


	//Other cluster attributes.
	int 					_ADC;
	int 					_size;
	std::vector<Cpix_hit*>	_pix_hits;
	int						_tracked; //0 for not tracked, 1 for part of a track, 2 for untrackable.
	CDQM_options *			_ops;
	Cpix_hit * 				_earlisetHit;




public:
	int trackID;
	//Member functions ________________________________________________________
	Ccluster(CDQM_options*);
	void					print_details();
	void					set_by_pixels(double);
	void					set_cluster_position(double);
	void					fill_cog_posn(double);
	void					DanCorrect();
	void					TaylorCorrect1();
	int						Shape();
	void					GaussCorrect();
	double					chi2_of_posn_sharing(double, double);
	double					pred_left_frac(Cpix_hit*, double, double);
	double					pred_left_frac(double, double);
	double					pred_right_frac(double, double);


	//Setters and getters _____________________________________________________

	//Cluster attributes ____________________

	void 					set_ID(int ID) {_ID = ID;}
	int						get_ID() {return _ID;}

	void 					set_chipID(int chipID) {_chipID = chipID;}
	int						get_chipID() {return _chipID;}

	void 					set_lposn(double lposn[4]) {
								_lposn[0] = lposn[0];
						    	_lposn[1] = lposn[1]; 
							    _lposn[2] = lposn[3];
							}

	void					get_lposn(double lposn[4]) {
								lposn[0] = _lposn[0];
							    lposn[1] = _lposn[1]; 
							    lposn[2] = 0.0;
							    lposn[3] = _lposn[2];
							}

	void 					set_ADC(int ADC) {_ADC = ADC;}
	int						get_ADC() {return _ADC;}

	void 					set_ToT(int ToT) {_ADC = ToT;}
	int						get_ToT() {return _ADC;}

	void 					set_size(int size) {_size = size;}
	int						get_size() {return _size;}

	void 					set_TOA(int TOA) {_lposn[2] = TOA;}
	double					get_TOA() {return _lposn[2];}

	void 					set_column(int column) {_lposn[0] = column;}
	double					get_column() {return _lposn[0];}

	void 					set_row(int row) {_lposn[1] = row;}
	double					get_row() {return _lposn[1];}

	void 					set_gposn(double gposn[4]) {
								_gposn[0] = gposn[0];
						    	_gposn[1] = gposn[1]; 
							    _gposn[2] = gposn[2];
							    _gposn[3] = gposn[3];
							}

	void					get_gposn(double gposn[4]) {
								gposn[0] = _gposn[0];
							    gposn[1] = _gposn[1]; 
							    gposn[2] = _gposn[2]; 
							    gposn[3] = _gposn[3];
							}

    double					get_gx(){return _gposn[0];}
    double					get_gy(){return _gposn[1];}
    double					get_gz(){return _gposn[2];}
    double					get_gt(){return _gposn[3];}

    void 					set_tracked(int tracked) {_tracked = tracked;}
	int						get_tracked() {return _tracked;}

	Cpix_hit*				earliestHit(){return _earlisetHit;}


	//Pixel attributes ____________________

	void 					add_pixel(Cpix_hit * pix_hit) {
								_pix_hits.push_back(pix_hit);}

	std::vector<Cpix_hit*> & get_pix_hits() {return _pix_hits;}

	Cpix_hit*					get_seed_pix(); //in source.


};

#endif
