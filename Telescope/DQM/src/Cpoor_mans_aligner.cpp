#include "../headers/Cpoor_mans_aligner.h"



//Author: Dan Saunders
//Date created: 27/11/13
//Date created: 27/11/13




//-----------------------------------------------------------------------------

Cpoor_mans_aligner::Cpoor_mans_aligner(Ctel_chunk * tel, 
	int ref_chip, int chip_loop_cut){
	Chandy::dash_line_break();
	std::cout<<"Constructor of Cpoor_mans_aligner."<<std::endl;

	_tel = tel;
	_ref_chip = ref_chip;
	_chip_loop_cut = chip_loop_cut;

	
	//Default/initial values.
	_ref_xbar = 0.0;
	_ref_ybar = 0.0;
	find_ref_av_positions();
}








//-----------------------------------------------------------------------------

void Cpoor_mans_aligner::find_av_positions(Cchip* chip, float &xbar, float &ybar){
	xbar = 0.0; ybar = 0.0; //reset current values.
	float lposn[4], gposn[4];

	std::vector<Cpix_hit*>::iterator ipix;
	for (ipix = chip->get_pix_hits().begin(); ipix != chip->get_pix_hits().end(); ipix++){
		(*ipix)->get_lposn(lposn);
		chip->lposn_to_gposn(lposn, gposn);

		xbar += gposn[0];
		ybar += gposn[1];
	}


	//Average.
	xbar /= (float) chip->get_npix_hits();
	ybar /= (float) chip->get_npix_hits();
}








//-----------------------------------------------------------------------------

void Cpoor_mans_aligner::find_ref_av_positions(){
	find_av_positions(_tel->get_chip(_ref_chip), _ref_xbar, _ref_ybar);
}








//-----------------------------------------------------------------------------

void Cpoor_mans_aligner::align_all(){
	//Cycle over all chips.
	for (std::vector<Cchip*>::iterator ichip = _tel->get_chips().begin();
		 ichip != _tel->get_chips().end(); ++ichip){
		align_chip((*ichip));
	}
}








//-----------------------------------------------------------------------------

void Cpoor_mans_aligner::align_chip(Cchip* chip){
	float xbar = 0.0; float ybar = 0.0;
	find_av_positions(chip, xbar, ybar);

	//set the x and y of the chip.
	float old_posn[4], new_posn[4];
	chip->get_gposn(old_posn);
	new_posn[0] = old_posn[0] + (_ref_xbar - xbar);
	new_posn[1] = old_posn[1] + (_ref_ybar - ybar);
	new_posn[2] = old_posn[2]; new_posn[3] = old_posn[3];
	chip->set_gposn(new_posn);

	std::cout<<chip->get_name()<<" shifted by:\t\t"<<(_ref_xbar - xbar)<<"\t"<<
		(_ref_ybar - ybar)<<std::endl;
}








//-----------------------------------------------------------------------------