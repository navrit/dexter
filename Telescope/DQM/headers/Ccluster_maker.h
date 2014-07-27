#ifndef __CCLUSTER_MAKER_H_INCLUDED__
#define __CCLUSTER_MAKER_H_INCLUDED__


#include <vector>
#include "data_holders/Cchip.h"
#include "data_holders/Ccluster.h"
#include "data_holders/Cpix_hit.h"
#include "data_holders/Ctel_chunk.h"
#include "../CDQM_options.h"
#include "string.h"


class Ccluster_maker{
private:
	Ctel_chunk*					_tel;
	std::vector<Cchip*>			_chips;
	int							_chip_loop_cut;
	float						_tlow;
	float						_tup;
	int							_num_pixs_not_clusted;
	std::string 				_save_file_name;
	float 						_COGweight;
	CDQM_options *				_ops;


public:
	//Member functions --------------------------------------------------------
	Ccluster_maker(CDQM_options*);
	~Ccluster_maker();
	void initialize();
	void execute(Ctel_chunk*);
	void finalize();

	void 						make_all_clusters();
	void						make_chip_clusters(Cchip*);
	void						fill_mini_event(Cpix_hit*, Cchip*, std::vector<Cpix_hit*> &);
	void						eval_mini_event(std::vector<Cpix_hit*> &, Cchip*);
	Cpix_hit*					get_first_seed_pix(std::vector<Cpix_hit*> &);
	void						add_edge_pixels(Ccluster*, std::vector<Cpix_hit*> &, Cpix_hit*);
	bool 						pixels_touch_side(Cpix_hit*, Cpix_hit *);
	bool 						pixels_touch_side_n_diag(Cpix_hit*, Cpix_hit *);
	void						test_loop(std::vector<Cpix_hit*>);



	//Setters and getters -----------------------------------------------------
	void 						set_save_file_name(std::string s){_save_file_name = s;}
	void						set_chips(std::vector<Cchip*> chips) {_chips = chips;}
	void						set_COGweight(float c) {_COGweight = c;}

	void						set_tlow(float t){_tlow = t;}
	float						get_tlow(){return _tlow;}

	void						set_tup(float t){_tlow = t;}
	float						get_tup(){return _tup;}

	void						set_chip_loop_cut(int n){_chip_loop_cut = n;}
	int							get_chip_loop_cut(){return _chip_loop_cut;}

};

#endif