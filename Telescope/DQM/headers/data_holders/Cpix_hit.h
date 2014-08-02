//Cpixel - a class to store all information for a single hit on a pixel.

//Author: Dan Saunders
//Date created: 15/10/13
//Last modified: 13/01/13


#ifndef __CPIX_HIT_H_INCLUDED__
#define __CPIX_HIT_H_INCLUDED__


#include <iostream>
#include <stdlib.h>

class Cpix_hit{
private:
	int 					_ID; //Unique to a chip only.
	int 					_row;
	int 					_column;
	int 					_ADC;
	double 					_TOA;
	bool 					_valid; //Whether to use hit in analysis.
	bool					_clustered;
	int 					_tag;
	int 					_chipID;



public:
	//Member functions ________________________________________________________
	Cpix_hit();
	~Cpix_hit();
	void PrintDetails();



	//Setters and getters _____________________________________________________
	void 					set_chipID(int i){_chipID = i;}
	int 					get_chipID(){return _chipID;}

	void 					set_ID(double ID) {_ID = ID;}
	int						get_ID() {return _ID;}

	void 					set_row(int row) {_row = row;}
	int						get_row() {return _row;}

	void 					set_column(int column) {_column = column;}
	int						get_column() {return _column;}

	void 					set_tag(int tag) {_tag = tag;}
	int						get_tag() {return _tag;}

	void 					set_ADC(double ADC) {_ADC = ADC;}
	int						get_ADC() {return _ADC;}

	void 					set_ToT(double ToT) {_ADC = ToT;}
	int						get_ToT() {return _ADC;}

	void 					set_TOA(double TOA) {_TOA = TOA;}
	double					get_TOA() {return _TOA;}

	void					set_valid(bool valid){_valid = valid;}
	bool					get_valid() {return _valid;}

	void					set_clustered(bool clustered){_clustered = clustered;}
	bool					get_clustered() {return _clustered;}

	void					get_lposn(double lposn[4]) {
								lposn[0] = double(_column);
								lposn[1] = double(_row);
								lposn[2] = 0.0;
								lposn[3] = _TOA;
							}						 

};

#endif
