#include "../../headers/data_holders/Cpix_hit.h"




//_____________________________________________________________________________

Cpix_hit::Cpix_hit(){
	//Set default values.
	_ID = 0;
	_row = 0;
	_column = 0;
	_ADC = 0;
	_TOA = 0.0;
	_valid = true;
	_clustered = false;
	_tag = 0;
}









//_____________________________________________________________________________


Cpix_hit::~Cpix_hit(){
}








//_____________________________________________________________________________

void Cpix_hit::PrintDetails(){
	std::cout<<"_ID\t"<<_ID<<std::endl;
	std::cout<<"_row\t"<<_row<<std::endl;
	std::cout<<"_column\t"<<_column<<std::endl;
	std::cout<<"_ADC\t"<<_ADC<<std::endl;
	std::cout<<"_TOA\t"<<_TOA<<std::endl;
	std::cout<<"_valid\t"<<_valid<<std::endl;
	std::cout<<"_clustered\t"<<_clustered<<std::endl;
	std::cout<<"_tag\t"<<_tag<<std::endl;
}








//_____________________________________________________________________________
