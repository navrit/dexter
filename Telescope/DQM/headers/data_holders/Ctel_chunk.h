//Ctel_chunk - a class to store a chunk of data collected during a telescope
//test beam. Analysis is done by passing Ctel_chunk to various other classes
//(e.g. a cluster maker, which appends the Ctel_chunk instance with the
//found clusters). This does mean there are sometimes variables unused, but does have
//the advantage of keeping data in one manageable place.

//Plotting is done in separate classes again, e.g. Cpixel_plots. This is to
//keep plotting code from data storage code, making code much more manageable 
//and shortening methods (more obejct-oriented). This also lowers dependencies
//between objects.

//Author: Dan Saunders
//Date created: 15/10/13
//Last modified: 22/01/14


#ifndef __CTEL_CHUNK_H_INCLUDED__
#define __CTEL_CHUNK_H_INCLUDED__

#include <vector>
#include <string>
#include "Cchip.h"
#include "Ctrack.h"
#include <iostream>
#include "Ccluster.h"
#include "../../src/Chandy.cpp"
#include "TFile.h"
#include "TH1F.h"
#include "TNtuple.h"
#include "../../CDQM_options.h"
#include <stdlib.h>
#include <fstream>


class Ctel_chunk{
private:
	//Telescope attributes.
	int 					_nchips;
	std::vector<Cchip*> 	_chips;


	//Tracks.
	int 					_ntracks;
	std::vector<Ctrack*> 	_tracks;
	std::string				_save_file_name;
	CDQM_options * 			_ops;


public:
	Chandy*					_handy;
	//uint64_t				_tzero;
	long long				_tzero;
	bool					_tzero_set;


	//Member functions ________________________________________________________
	Ctel_chunk(CDQM_options*); 
	~Ctel_chunk();
	int 					chipname_to_id(std::string);
	void					time_order_pixels_comb();
	void					time_order_clusters_comb();
	void 					rm_all_hot_pixels(float);
	void					fill_all_hit_maps();
	int						get_total_hits_clustered();
	int						get_total_pix_hits();
	void					truncate(int);
	void					save_alignments(std::string);
	void					save_blank_alignments(std::string);
	void					load_alignments(std::string);


	//Dumpers.
	void					dump_tel(bool, std::string, bool);
	void					dump_chips(std::string, bool);
	void					dump_clusters(bool, std::string, bool);
	void					dump_tracks(bool, std::string, bool);



	//Methods for chips --------------------------------
	void 					set_nchips(int nchips) {_nchips = nchips;}
	int 					get_nchips() {return _nchips;}

	void 					add_chip(Cchip* chip) {_chips.push_back(chip);}

	void 					set_chips(std::vector<Cchip*> & chips) {
							_chips = chips;}
	std::vector<Cchip*> &	get_chips() {return _chips;}

	Cchip* 					get_chip(int chipID) {return _chips[chipID];}



	//Methods for tracks -------------------------------

	void 					set_ntracks(int ntracks) {_ntracks = ntracks;}
	int 					get_ntracks() {return _ntracks;}

	void 					set_tracks(std::vector<Ctrack*> & tracks) {
							_tracks = tracks;}
	std::vector<Ctrack*> &	get_tracks() {return _tracks;}

	void					add_track(Ctrack * t){
								_tracks.push_back(t);
								_ntracks+=1;}

	void					set_save_file_name(std::string s){_save_file_name = s;}

	void					setup_pix_hits_by_glob_x();
};

#endif
