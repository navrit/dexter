#include "../headers/Cpoor_mans_resolution.h"
#include <sstream>


//-----------------------------------------------------------------------------

 Cpoor_mans_resolution::Cpoor_mans_resolution(Ctel_chunk* tel, 
 	std::string save_file_name, int chip_loop_cut){

 	Chandy::dash_line_break();
	std::cout<<"Constructor of Cpoor_mans_resolution"<<std::endl;

	//Default values.
	_save_file_name = save_file_name;
	_tel = tel;
	_chip_loop_cut = chip_loop_cut;
}







//-----------------------------------------------------------------------------

 void Cpoor_mans_resolution::find_resolutions(){
 	//Outputs a TH1F of all the resolutions.
}







//-----------------------------------------------------------------------------

 void Cpoor_mans_resolution::fill_seq_variances(){

}







//-----------------------------------------------------------------------------

 Cpoor_mans_resolution::~Cpoor_mans_resolution(){
 	std::cout<<"Deleting Cpoor_mans_resolution."<<std::endl;
}








//-----------------------------------------------------------------------------
