#ifndef __CCORREL_LINE_H__
#define __CCORREL_LINE_H__

#include "math.h"


class Ccorrel_line{
private:
	float 			_theta;
	float			_c_shift;
	float			_del_theta;
	float			_del_c_shift;
	float 			_SN;



public:
	void			set_SN(float SN){_SN = SN;}
	float			get_SN(){return _SN;}

	void			set_theta(float theta){_theta = theta;}
	float			get_theta(){return _theta;}

	void			set_del_theta(float del_theta){_del_theta = del_theta;}
	float			get_del_theta(){return _del_theta;}

	void			set_c_shift(float c_shift){_c_shift = c_shift;}
	float			get_c_shift(){return _c_shift;}

	void			set_del_c_shift(float del_c){_del_c_shift = del_c;}
	float			get_del_c_shift(){return _del_c_shift;}

	float			y(double x){return tan(get_theta())*x + get_c_shift();}
};

#endif
