#ifndef __CPS_TEL_GETTER_INCLUDED__
#define __CPS_TEL_GETTER_INCLUDED__

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



class CPS_tel_getter{
private:
	Ctel_chunk * 			_tel;
	CDQM_options * 			_ops;
	float					_tcut;
	unsigned int			_nPlanes;
	std::string				_input_file_name;
	std::vector<std::string> _input_file_names;


public:
	double tEventStep;
	int _eventNum;
	//Member functions.
	CPS_tel_getter(CDQM_options*);
	~CPS_tel_getter();
	void initialize();
	void execute(Ctel_chunk*);
	void finalize();

	void 					fill_chips();
	void 					fill_pixels();
	Cchip* 					make_chip(std::string, int);
	Cpix_hit* 				line_to_pixel(long long, int);
	void 					set_npixels();
	void 					reset_all_pixel_ids();
};


#endif
