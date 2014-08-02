#ifndef __CCHIP_H_INCLUDED__
#define __CCHIP_H_INCLUDED__

#include <iostream>
#include <vector>
#include <string>
#include <math.h>
#include "stdlib.h"

#include "Cpix_hit.h"
#include "Ccluster.h"
#include "../../src/Chandy.cpp"
#include "../Ccorrel_line.h"

#include "TH2F.h"



class Cchip{
private:
	//Detector attributes.
	std::string 			_name;
	int 					_ID;


	//Position and orientation as arrays.
	double 					_gposn[4]; //4 alignment parameters here (time inc).
	double 					_orientation[3]; //around x, around y, around z.
	double 					_rotn;


	//Detectors will always have pixels and (optionally found) clusters.
	int						_npix_hits;
	std::vector<Cpix_hit*> 	_pix_hits;
	int 					_nclusters;
	std::vector<Ccluster*>	_clusters;

	std::vector<Ccluster*>	_clusters_by_glob_x;
	std::vector<Cpix_hit*> 	_pix_hits_by_glob_x;



	//Dimensions of the chip.
	double 					_pixel_width;
	double 					_pixel_height;
	double 					_chip_width; //both working in units of mm.
	double 					_chip_height;
	int						_size_pixs[2]; //number of pixels across and up.
	double					_size_mms[2]; //same, now in mms.
	int						_n_pixs_side;

	//ID finders.
	double					_tpixzero;
	double					_tpixstep;
	double					_tclustzero;
	double					_tcluststep;
	int						_num_pix_tsteps;
	//int						_num_clust_tsteps;



	int						_num_pix_xsteps;



	//Optionally filled.
	std::vector< std::vector< std::vector<int> > >* _hit_map;
	bool					_hit_map_filled;



public:
	int						_num_clust_xsteps;
	int						_num_clust_tsteps;
	int nthingy;
	std::vector<int> 		_loc_ts_to_pixIDs;
	std::vector<int> 		_loc_ts_to_clustIDs;

	std::vector<int> 		_glob_xs_to_pixIDs;
	std::vector<int> 		_glob_xs_to_clustIDs;

	double					_xpixzero;
	double					_xpixstep;
	double					_xclustzero;
	double					_xcluststep;


	//Function declarations.
	Cchip(CDQM_options* = NULL);
	~Cchip();
	CDQM_options * _ops;
	int 					get_min_pixel_ADC();
	int 					get_max_pixel_ADC();
	int 					get_min_cluster_ADC();
	int 					get_max_cluster_ADC();
	double 					get_average_cluster_ADC();
	double 					get_average_pixel_ADC();
	double 					get_pixel_cluster_ADC();
	double 					get_std_cluster_ADC();
	void 					set_std_and_average_cluster_ADC(double&, double&);
	void 					set_std_and_average_pixel_ADC(double&, double&);
	void					fill_cluster_sample(int, std::vector<Ccluster*> &);
	void					truncate(int);


	//For setting IDs to indexs.
	void					reset_cluster_IDs();
	void					reset_pixel_IDs();


	//For hot pixels.
	void					rm_hot_pixels(int);
	void					fill_hit_map();
	int						num_active_hits();


	//Spatial and temporal converters.
	void 					lposn_to_gposn(double*, double*);
	void 					gposn_to_lposn(double*, double*);

	void 					fill_loc_ts_to_pixIDs();
	void 					fill_loc_ts_to_clustIDs();

	void 					fill_glob_xs_to_pixIDs();
	void 					fill_glob_xs_to_clustIDs();

	int 					glob_t_to_pixID(double);
	int 					glob_t_to_clustID(double);
	int 					glob_x_to_pixID(double);
	int 					glob_x_to_clustID(double);

	void					time_order_pix_hits_comb();
	void					x_order_pix_hits_comb();
	void					time_order_clusts_comb();
	void					x_order_clusts_comb();

	void					setup_pix_hits_by_glob_x();
	void					setup_clusters_by_glob_x();

	//Misc.
	void 					print_details();



	//Setters and getters (and adders).
	//Chip stuff --------------------------------
	void 					set_name(std::string name) {_name = name;}
	std::string				get_name() {return _name;}

	void 					set_ID(int ID) {_ID = ID;}
	int 					get_ID() {return _ID;}

	int						get_n_pixs_side() {return _n_pixs_side;}

	void 					set_gposn(double gposn[4]) {
								_gposn[0] = gposn[0];
								_gposn[1] = gposn[1];
								_gposn[2] = gposn[2];
								_gposn[3] = gposn[3];
							}

	double					get_posn_dir(int i) {return _gposn[i];}
	void 					get_gposn(double gposn[4]) {
								gposn[0] = _gposn[0];
								gposn[1] = _gposn[1];
								gposn[2] = _gposn[2];
								gposn[3] = _gposn[3];
							}

	void 					set_orientation(double orientation[3]) {
								_orientation[0] = orientation[0];
								_orientation[1] = orientation[1];
								_orientation[2] = orientation[2];
							}

	void 					get_orientation(double orientation[3]) {
								orientation[0] = _orientation[0];
								orientation[1] = _orientation[1];
								orientation[2] = _orientation[2];
							}

	void					set_ox(double ox){_orientation[0] = ox;}				
	double					get_ox(){return _orientation[0];}

	void					set_oy(double oy){_orientation[1] = oy;}		
	double					get_oy(){return _orientation[1];}

	void					set_oz(double oz){_orientation[2] = oz;}		
	double					get_oz(){return _orientation[2];}

	void 					set_rotn(double rotn) {_rotn = rotn;}
	double 					get_rotn() {return _rotn;}

	double* 					get_size_mms() {return _size_mms;}

	int* 					get_size_pixs() {return _size_pixs;}


	std::vector< std::vector< std::vector<int> > >* get_hit_map(); 
							//...defined in src file.


	double					get_pix_dir(Cpix_hit*, int);

	double					get_gt(){return _gposn[3];}
	double					get_gx(){return _gposn[0];}
	double					get_gy(){return _gposn[1];}
	double					get_gz(){return _gposn[2];}



	//Pixel stuff -------------------------------
	void 					set_npix_hits(int n) {_npix_hits = n;}
	int 					get_npix_hits() {return _npix_hits;}

	void 					set_pix_hits(std::vector<Cpix_hit*> & pix_hits){
								_pix_hits = pix_hits;}
	std::vector<Cpix_hit*>&	get_pix_hits() {return _pix_hits;}
	std::vector<Cpix_hit*>&	get_pix_hits_by_glob_x() {return _pix_hits_by_glob_x;}

	void					add_pix_hit(Cpix_hit* pix_hit){
								_pix_hits.push_back(pix_hit);}

	void 					set_pixel_width(double pixel_width) {
								_pixel_width = pixel_width;}
	double 					get_pixel_width() {return _pixel_width;}

	void 					set_pixel_height(double pixel_height) {
								_pixel_height = pixel_height;}
	double 					get_pixel_height() {return _pixel_height;}


	void 					set_chip_width(double chip_width) {
								_chip_width = chip_width;}
	double 					get_chip_width() {return _chip_width;}

	void 					set_chip_height(double chip_height) {
								_chip_height = chip_height;}
	double 					get_chip_height() {return _chip_height;}

	double					get_z(){return _gposn[2];}



	//Cluster stuff -----------------------------
	void 					set_nclusters(int nclusters) {
								_nclusters = nclusters;}
	int						get_nclusters() {return _nclusters;}


	void 					set_clusters(std::vector<Ccluster*> & clusters){
								_clusters = clusters;}
	std::vector<Ccluster*>&	get_clusters() {return _clusters;}
	std::vector<Ccluster*>&	get_clusters_by_glob_x() {return _clusters_by_glob_x;}

	void					add_cluster(Ccluster* cluster){
								_clusters.push_back(cluster);
								_nclusters++;}

	void					print_gposn(){std::cout<<"gposn of chip "<<get_ID()<<":\t"<<
							get_gx()<<"\t"<<get_gy()<<"\t"<<get_gz()<<"\t"<<get_gt()<<std::endl;}


};

#endif
