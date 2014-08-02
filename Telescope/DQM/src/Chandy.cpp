//Chandy - a simple class to hold some data, produce some simple plots
//and perform some simple generic functions.
#ifndef __CHANDY_CPP_INCLUDED__
#define __CHANDY_CPP_INCLUDED__


#include <vector>
#include <iostream>
#include <math.h>
#include "TAxis.h"
#include "TH2F.h"
#include <math.h>
#include <sstream>
#include <algorithm>

class Chandy{
public:
	double					h_f;
	std::vector<double>*		h_fs;

	int						h_i;
	std::vector<int>*		h_is;



	//member functions. Some are static as useful to be called from the outside
	//without having to create an instance of the class - funky.
	//-------------------------------------------------------------------------
	static void dash_line_break(){
		std::cout<<"\n\n";
		for (int i=0; i<78; i++) std::cout<<"_";
		std::cout<<"\n";
	}

	//-------------------------------------------------------------------------
	static std::string DFtS(double my_double){
		std::stringstream ss;
		ss<<my_double;
		return ss.str();
	}



	//-------------------------------------------------------------------------
	static void dash_line(){
		for (int i=0; i<78; i++) std::cout<<"_";
		std::cout<<"\n";
	}


	//-------------------------------------------------------------------------
	static void R_z(double r[3], double theta){
		double r_dash[3];

		r_dash[0] = - sin(theta) * r[1] + cos(theta)*r[0];
		r_dash[1] = sin(theta)*r[0] + cos(theta)*r[1];
		r_dash[2] = r[2];

		// std::cout<<"\nz: "<<theta<<std::endl;
		// std::cout<<r_dash[0]<<"\t"<<r[0]<<std::endl;
		// std::cout<<r_dash[1]<<"\t"<<r[1]<<std::endl;
		// std::cout<<r_dash[2]<<"\t"<<r[2]<<std::endl;

		r[0] = r_dash[0]; r[1] = r_dash[1]; r[2] = r_dash[2]; 
	}


	//-------------------------------------------------------------------------
	static void R_y(double r[3], double theta){
		double r_dash[3];
		
		r_dash[0] = cos(theta)*r[0] + sin(theta)*r[2];
		r_dash[1] = r[1];
		r_dash[2] = cos(theta)*r[2] - sin(theta)*r[0];

		// std::cout<<"\ny: "<<theta<<std::endl;
		// std::cout<<r_dash[0]<<"\t"<<r[0]<<std::endl;
		// std::cout<<r_dash[1]<<"\t"<<r[1]<<std::endl;
		// std::cout<<r_dash[2]<<"\t"<<r[2]<<std::endl;

		r[0] = r_dash[0]; r[1] = r_dash[1]; r[2] = r_dash[2]; 
	}


	//-------------------------------------------------------------------------
	static void R_x(double r[3], double theta){
		double r_dash[3];

		r_dash[0] = r[0];
		r_dash[1] = cos(theta)*r[1] - sin(theta)*r[2];
		r_dash[2] = sin(theta)*r[1] + cos(theta)*r[2];

		// std::cout<<"\nx: "<<theta<<std::endl;
		// std::cout<<r_dash[0]<<"\t"<<r[0]<<std::endl;
		// std::cout<<r_dash[1]<<"\t"<<r[1]<<std::endl;
		// std::cout<<r_dash[2]<<"\t"<<r[2]<<std::endl;

		r[0] = r_dash[0]; r[1] = r_dash[1]; r[2] = r_dash[2]; 
	}


	//-------------------------------------------------------------------------
	static std::string get_str_dir(int dir){
		//Converts 4vector indexs into English.

		if (dir == 0) return "x";
		else if (dir == 1) return "y";
		else if (dir == 2) return "z";
		else if (dir == 3) return "t";
		else std::cout<<"Unknowm dir given to Chandy::get_str_dir(int dir)."<<std::endl;
	}


	//-------------------------------------------------------------------------
	static TH2F* subtract_TH2Fs(TH2F* h1, TH2F* h2, double weight = 1.0, bool one_switch = false,
		std::string name = "h_subtracted"){
		//Method to subtract two TH2Fs that have the same bins, with the option
		//to weight the first over the other (like a re-norm).
		//Cheat, and just clone it to begin with - then reset the heights.
		TH2F *h = (TH2F*)h1->Clone(name.c_str());


		if (one_switch) h->SetMinimum(-1);
		//Loop over all bins.
		for (int ix = 0; ix < h->GetNbinsX(); ix++){
			for (int iy = 0; iy < h->GetNbinsY(); iy++){
				int binID = h->GetBin(ix ,iy);

				double x = h1->GetBinContent(binID) -
						   weight* h2->GetBinContent(binID);

				if (one_switch){if (x<0) x=0;}
				h->SetBinContent(binID, x);
			}
		}
		return h;
	}


	//-------------------------------------------------------------------------
	static TH1F* project_2dhist(TH2F* h2d, double theta, int nbins, std::string n = "my_hist"){
		//Method to return the projection of a 2d histogram (as a TH1F*) along
		//the direction defined by m (note m doesnt define the axis to be
		//projected onto - rather its perpendicular). Only need one variable.

		//Line is cenetered on the plot and tilted around.
		//Take the extremes. Let the xlow be top left extreme.
		double axis_length = get_projected_axis_length(h2d);
		double xlow = -0.5*axis_length;
		double xup = 0.5*axis_length;

		TH1F* h1d = new TH1F(n.c_str(), n.c_str(), nbins, xlow, xup);
		projection_fill(h2d, h1d, theta);
		return h1d;
	}


	//-------------------------------------------------------------------------
	static void projection_fill(TH2F* h2d, TH1F* h1d, double theta){
		//need to cycle over all bins, filling the TH1F with weights.
		TAxis* x_axis = h2d->GetXaxis();
		TAxis* y_axis = h2d->GetYaxis();

		int ix_bin = 0; int iy_bin = 0;

		for (double ix = x_axis->GetXmin();
					ix<x_axis->GetXmax();
					ix += x_axis->GetBinWidth(0)){
			iy_bin = 0;

			for (double iy = y_axis->GetXmin();
						iy<y_axis->GetXmax();
						iy += y_axis->GetBinWidth(0)){

				//Fist shift ix and iy into the projected codn system (centered
				//at the middle of the h2d).
				double shifted_ix = ix - (x_axis->GetXmin() +
					0.5*(x_axis->GetXmax() - x_axis->GetXmin()));

				double shifted_iy = iy - (y_axis->GetXmin() +
					0.5*(y_axis->GetXmax() - y_axis->GetXmin()));

				double x_dash = get_x_dash(shifted_ix, shifted_iy, theta);

				//finally get the weight.
				int x = h2d->GetBin(ix_bin, iy_bin);
				h1d->Fill(x_dash, h2d->GetBinContent(x));
				iy_bin++;
			}
			
			ix_bin++;
		}
	}


	//-------------------------------------------------------------------------
	static double get_x_dash(double x, double y, double theta){
		double x_dash = -sin(theta) * y + cos(theta) * x;
		return x_dash;
	}


	//-------------------------------------------------------------------------
	static double pythag(double x, double y){
		double z;
		z = pow(pow(x, 2) + pow(y, 2), 0.5);
		return z;
	}


	//-------------------------------------------------------------------------
	static double get_projected_axis_length(TH2F* h2d){
		TAxis* x_axis = h2d->GetXaxis();
		TAxis* y_axis = h2d->GetYaxis();

		double xlow = x_axis->GetXmin();
		double xup = x_axis->GetXmax();

		double ylow = y_axis->GetXmin();
		double yup = y_axis->GetXmax();

		return pythag(xup-xlow, yup-ylow);
	}


	//-------------------------------------------------------------------------
	static std::vector<std::string> split_line(std::string line, int spacing){

		//function splits a string by its spaces.
		std::vector<std::string> line_bits;
		//cycle through the line, looking for spaces. Never starts with a space.
		//bool hitNextNumber = false;
		for (unsigned int i=0; i<line.size(); i++){
			if (line[i] == ' ') {
				if (line[i-1] != ' ') {
					line_bits.push_back(line.substr(0, i));
					line = line.substr(i, line.size()); //skip the space.
					i=0;
				}
			}
		}
		line_bits.push_back(line); //the last one.

		return line_bits;
	}


	//-------------------------------------------------------------------------
	std::vector<double>* linear_scaler(	std::vector<double>* xs,
										double x_low, double x_up){
		double x_max = ptrvecf_max(xs);
		double x_min = ptrvecf_min(xs);

		std::cout<<x_max<<"\t"<<x_min<<std::endl;

		for (std::vector<double>::iterator xptr = xs->begin();
				xptr != xs->end(); ++xptr){
			(*xptr) = ((*xptr) - x_max)/(x_max - x_min);		//scaling down.
			(*xptr) = (*xptr) * (x_up - x_low) + x_low;			//scaling up.
		}

		return xs;
	}


	//-------------------------------------------------------------------------
	double ptrvecf_max(std::vector<double>* xs){
		//assume first is the lowest, then iterate.
		double x_max = xs->operator[](0);

		//iterate over xs.
		for (std::vector<double>::iterator xptr = xs->begin();
				xptr != xs->end(); ++xptr){
			if ((*xptr) > x_max) x_max = (*xptr);
		}
		return x_max;
	}


	//-------------------------------------------------------------------------
	double ptrvecf_min(std::vector<double>* xs){
		//assume first is the lowest, then iterate.
		double x_min = xs->operator[](0);
		for (std::vector<double>::iterator xptr = xs->begin();
				xptr != xs->end(); ++xptr){
			if ((*xptr) < x_min) x_min = (*xptr);
		}
		return x_min;
	}


	//-------------------------------------------------------------------------
	double project(double &x, double &y, double &theta, double &delX){
		//takes a 2d co-ordinate and returns the value projected down one
		//rotated and shifted axis. theta given in radians.

		double x_dash = cos(theta) * (x + y*tan(theta) + delX);
		return x_dash;
	}


	//-------------------------------------------------------------------------
	static bool vector_int_containt(std::vector<int> xs, int x){
		bool result = false;
		for (std::vector<int>::iterator it = xs.begin(); 
			 it != xs.end(); it++){
			if ((*it) == x) result = true;
		}
	return result;
	}


	//-------------------------------------------------------------------------
};








class Cterm_hist{
public:

	//Prints a simple (but awesome) histogram into the terminal. Each bin
	//corresponds to a line in the terminal.
	std::vector<double>* _xs;
	double _xup;
	double _xlow;
	int _nbins;
	std::vector<int> _ns;
	Chandy* _handy;
	double _binw; //bin width.
	int _nup;
	int _nlow;
	int _term_width;



	//member functions.
	//-------------------------------------------------------------------------




	//constructor.
	Cterm_hist(	std::vector<double>* xs, int nbins, double xlow = 0.2263,
				double xup = 0.2263, int nlow = -1, int nup = -1){
		std::cout<<"Constructor of a term_hist"<<std::endl;
		//initialize variables.
		_xs = xs;
		_nbins = nbins;
		_xlow = xlow;
		_xup = xup;

		if (xup == 0.2263) xup = *std::max_element(_xs->begin(), _xs->end());
		if (xlow == 0.2263) xlow = *std::min_element(_xs->begin(), _xs->end());
		for (int i=0; i<nbins; i++) _ns.push_back(0);

		_binw = (xup - xlow)/nbins;
		_nup = nup;
		_nlow = nlow;

		_term_width = 60;

		fill_ns();
		scale_ns();
		print_ns();
	}




	//-------------------------------------------------------------------------




	void fill_ns(){
		//cycles through xs, adding to the relevant element in ns.
		//start by finding the index of the element.
		for (	std::vector<double>::iterator xptr = _xs->begin();
				xptr != _xs->end(); ++xptr){

			//check not an extreme.
			if ((*xptr) > _xlow && (*xptr) < _xup){
				int in = (int)((*xptr) - _xlow)/_binw;
				_ns[in] += 1;
			}
		}
	}




	//-------------------------------------------------------------------------




	void print_ns(){

		//y axis and bin heights.
		std::cout<<"\n\n";
		for (int ibin = 0; ibin<_nbins; ibin++){
			std::cout<<"\n";
			std::cout.precision(3);
			std::cout<<_xlow+ibin*_binw;
			std::cout.precision(10);
			std::cout<<"\t\t|";
			for (int in = 0; in<_ns[ibin]; in++) std::cout<<"#";
		}

		//x axis.
		std::cout<<"\n\t\t|";
		for (int i = 0 ; i<_term_width; i++) std::cout<<"_";
		std::cout<<"\n\t\t"<<_nlow;
		for (int i = 0 ; i<_term_width-2; i++) std::cout<<" ";
		std::cout<<_nup<<"\n";
	}




	//-------------------------------------------------------------------------




	void scale_ns(){
		if (_nlow == -1) _nlow = *std::min_element(_ns.begin(), _ns.end());
		if (_nup == -1) _nup = *std::max_element(_ns.begin(), _ns.end());

		//linearly scale ns according to these lims.
		for (int i = 0; i<_nbins; i++){
			_ns[i] = (int)_term_width*(_ns[i] - _nlow)/(_nup - _nlow);
		}

	}




	//-------------------------------------------------------------------------

};



#endif
