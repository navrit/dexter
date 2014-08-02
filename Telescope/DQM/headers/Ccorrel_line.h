#ifndef __CCORREL_LINE_H__
#define __CCORREL_LINE_H__

#include "math.h"


class Ccorrel_line{
private:
	double 			_theta;
	double			_c_shift;
	double			_del_theta;
	double			_del_c_shift;
	double 			_SN;



public:
	void			set_SN(double SN){_SN = SN;}
	double			get_SN(){return _SN;}

	void			set_theta(double theta){_theta = theta;}
	double			get_theta(){return _theta;}

	void			set_del_theta(double del_theta){_del_theta = del_theta;}
	double			get_del_theta(){return _del_theta;}

	void			set_c_shift(double c_shift){_c_shift = c_shift;}
	double			get_c_shift(){return _c_shift;}

	void			set_del_c_shift(double del_c){_del_c_shift = del_c;}
	double			get_del_c_shift(){return _del_c_shift;}

	double			y(double x){return tan(get_theta())*x + get_c_shift();}
};

#endif
