#ifndef __Coldtelgetter_INCLUDED__
#define __Coldtelgetter_INCLUDED__

//This class contains a set of functions for reading in the clusters of old
//data (no time stamps)

//Author: Dan Saunders
//Date created: 15/10/13
//Last modified: 15/10/13

#include <iostream>
#include <vector>
#include <string>
#include <stdlib.h>
#include <fstream>
#include "data_holders/Ctel_chunk.h"
#include "data_holders/Cchip.h"
#include "data_holders/Ccluster.h"
#include "data_holders/Cpix_hit.h"
#include "../src/Chandy.cpp"
#include "../CDQM_options.h"



class Cold_tel_getter{
private:
	Ctel_chunk * 			_tel;
	CDQM_options * 			_ops;
	double					_tcut;


public:
	//Member functions.
	Cold_tel_getter(CDQM_options*);
	~Cold_tel_getter();
	void initialize();
	void execute(Ctel_chunk*);
	void finalize();

	void 					fill_chips();
	void 					fill_pixels();
	void					fill_MC_pixels();
	Cchip* 					make_chip(std::string, int);
	Cpix_hit* 				line_to_pixel(std::string, int&);
	void 					set_npixels();
	void 					reset_all_pixel_ids();
	double					make_TOA(double, int); //adds prescribed errors.
};


#endif
