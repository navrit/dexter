#include "../../headers/data_holders/Ccluster.h"

//=============================================================================

class DGauss{
public:
	double m_x0;
	double m_y0;
	double m_sigma; //symmetric.
	double m_norm;
	DGauss(double, double);

	double integral(double, double, double, double);
	double integrand(double, double);
};


DGauss::DGauss(double x0, double y0){
	m_x0 = x0;
	m_y0 = y0;
	m_norm = 1;
	m_sigma = 0.36;
}


double DGauss::integral(double x1, double x2, double y1, double y2){
	// TRandom3 RandGentor;
 //  	RandGentor.SetSeed(); 

	// double integral1 = 0.0;
	// int ndots = 400;
	// double delx = (x2-x1);
	// double dely = (y2-y1);
	// for (int i = 0; i<ndots; i++){
	// 	double x = RandGentor.Rndm() * delx + x1;
	// 	double y = RandGentor.Rndm() * dely + y1;
	// 	integral1 += delx*dely*integrand(x, y)/(double)ndots;
	// }
	// //integral1 = delx*dely*integral1;

	double integral0 = 0.0;
	int nbinsx = 30;
	int nbinsy = 30;

	double xstep = (x2-x1)/(double)nbinsx;
	double ystep = (y2-y1)/(double)nbinsy;

	for (int i=0; i<nbinsx; i++){
		for (int j=0; j<nbinsy; j++){
			double x = x1+i*xstep;
			double y = y1+j*ystep;

			integral0+=xstep*ystep*integrand(x, y);
		}
	}


	//std::cout<<"\t"<<integral0<<std::endl;
	return integral0;
}


double DGauss::integrand(double x, double y){
	double integrand = 0.0;
	double delx = x - m_x0;
	double dely = y - m_y0;

	double dum = delx*delx + dely*dely;
	//std::cout<<m_sigma<<"\t"<<2*pow(m_sigma,2)<<std::endl;
	double dum2 = 2*m_sigma*m_sigma;
	integrand = m_norm*exp(-(dum/dum2));
	return integrand;
}


//=============================================================================

//_____________________________________________________________________________

Ccluster::Ccluster(CDQM_options * ops){
	_ops = ops;
	_ID = 0;
	_chipID = 0;
	_lposn[0] = 0.0; _lposn[1] = 0.0; _lposn[2] = 0.0; //row, column, t.
	_ADC = 0;
	_size = 0;
	_tracked = 0;

	_gposn[0] = 0.0; _gposn[1] = 0.0; _gposn[2] = 0.0; _gposn[3] = 0.0;
}






//_____________________________________________________________________________

void Ccluster::print_details(){
	std::cout<<"\nCluster details:"<<std::endl;
	std::cout<< "ID" <<"\t"<< _ID <<std::endl;
	std::cout<< "ChipID" <<"\t"<< _chipID <<std::endl;
	std::cout<< "posn:" <<"\t"<< _lposn[0] <<" "<< _lposn[1] <<" "<<_lposn[2]<<std::endl;
	std::cout<< "ADC:" <<"\t"<< _ADC <<std::endl;
	std::cout<< "size:" <<"\t"<< _size <<std::endl;
}







//_____________________________________________________________________________

void Ccluster::set_by_pixels(double COGweight){
	//Sets this clusters attributes according to the set of pixels.

	this->set_ADC(0);
	for (std::vector<Cpix_hit*>::iterator ipix = get_pix_hits().begin(); 
		ipix != get_pix_hits().end(); ipix++){
		set_ADC(get_ADC() + (*ipix)->get_ADC());
	}

	set_cluster_position(COGweight);
	set_size(get_pix_hits().size());


	//Total ADC.
	this->set_ADC(0);
	for (std::vector<Cpix_hit*>::iterator ipix = get_pix_hits().begin(); 
		ipix != get_pix_hits().end(); ipix++){
		set_ADC(get_ADC() + (*ipix)->get_ADC());
	}

	int maxADC = 0;
	int maxADCpixID = 0;
	int pixID = 0;

	_earlisetHit =  get_pix_hits()[0];
	for (std::vector<Cpix_hit*>::iterator ipix = get_pix_hits().begin(); 
		ipix != get_pix_hits().end(); ipix++){
		if ((*ipix)->get_TOA() < _earlisetHit->get_TOA()) _earlisetHit = (*ipix);
	}

	for (std::vector<Cpix_hit*>::iterator ipix = get_pix_hits().begin(); 
		ipix != get_pix_hits().end(); ipix++){
		if ((*ipix)->get_ADC() > maxADC) {
			maxADC = (*ipix)->get_ADC();
			maxADCpixID = pixID;
		}
		pixID++;
	}
	_lposn[2] = get_pix_hits()[maxADCpixID]->get_TOA();
}







//_____________________________________________________________________________

void Ccluster::set_cluster_position(double COGweight){
	//Sets this clusters attributes according to the set of pixels.
	fill_cog_posn(COGweight);
	// if (_ops->DanCorrectPosn) {
	// 	if (get_size()==2 || get_size()==3) {
	// 		//std::cout<<get_row()<<"\t"<<get_column()<<std::endl;
	// 		DanCorrect();
	// 		//std::cout<<get_row()<<"\t"<<get_column()<<std::endl;
	// 	}
	// }
}







//_____________________________________________________________________________

void Ccluster::DanCorrect(){
	// Start by filling the COG posn, since that gets us close to the minimum.
	//std::cout<<this->Shape()<<std::endl;
	//std::cout<<"Hi Dan!"<<std::endl;
	//if (this->Shape() == 1) std::cout<<"God one returned!"<<std::endl;
	if (get_chipID()==2) GaussCorrect();
}



//_____________________________________________________________________________

void Ccluster::GaussCorrect(){
	// Start by filling the COG posn, since that gets us close to the minimum.
	// No row correct.

	int n = 40;
	double h = 0.01;
	double gamma = 0.04;

	double xs[n];
	double ys[n];
	double chis[n];
	double ns[n];

	for (int i=0; i<n; i++){
		double c_h = get_column() - h;
		double r_h = get_row() - h;

		double chi_current = chi2_of_posn_sharing(get_row(), get_column()); 

		double derivx = (chi2_of_posn_sharing(get_row(), c_h) - chi_current)/h;
		double derivy = (chi2_of_posn_sharing(r_h, get_column()) - chi_current)/h;

		_lposn[0] = _lposn[0] + gamma*derivx;
		_lposn[1] = _lposn[1] + gamma*derivy;


		ns[i] = i;
		chis[i] = chi_current;
		xs[i] = _lposn[0];
		ys[i] = _lposn[1];
	}


	if (get_ID()<50){
		TGraph chi_vs_n(n, ns, chis);
		chi_vs_n.SetTitle("chi_vs_n");
		TGraph xs_vs_ys(n, xs, ys);
		xs_vs_ys.SetTitle("xs_vs_ys");
		TFile* save_file = new TFile(_ops->save_file_name.c_str(),"update");

		chi_vs_n.Write();
		xs_vs_ys.Write();

		save_file->Close();
	}
}






//_____________________________________________________________________________

int Ccluster::Shape(){
	// Start by filling the COG posn, since that gets us close to the minimum.
	int shape = -1;
	if (this->get_size()==1) shape = 0;
	else if (this->get_size()==2){
		if ((_pix_hits[0]->get_row() - _pix_hits[1]->get_row()) == 0) shape = 1;
		else shape = 2;
	}
	//std::cout<<"Considering returning"<<"\t"<<shape<<std::endl;
	return shape;
}







//_____________________________________________________________________________

void Ccluster::fill_cog_posn(double COGweight){
	double alpha = COGweight; //nonlinear weighting on signal.
	double tot_weighted_ADC = 0.0;

	for (std::vector<Cpix_hit*>::iterator ipix = get_pix_hits().begin(); 
		ipix != get_pix_hits().end(); ipix++){
		_lposn[0] += (*ipix)->get_column() * pow((*ipix)->get_ADC(), alpha);
		_lposn[1] += (*ipix)->get_row() * pow((*ipix)->get_ADC(), alpha);

		tot_weighted_ADC += pow((*ipix)->get_ADC(), alpha);
	}

	_lposn[0] /= (double)tot_weighted_ADC;
	_lposn[1] /= (double)tot_weighted_ADC;
}







//_____________________________________________________________________________

Cpix_hit * Ccluster::get_seed_pix(){
	//Assume the first.
	Cpix_hit * seed_pix = get_pix_hits()[0];
	for (std::vector<Cpix_hit*>::iterator ipix = get_pix_hits().begin(); 
		ipix != get_pix_hits().end(); ipix++){

		if ((*ipix)->get_ADC() > seed_pix->get_ADC()) seed_pix = (*ipix);
	}

	return seed_pix;
}







//_____________________________________________________________________________

double Ccluster::chi2_of_posn_sharing(double r, double c){
	double chi = 0.0;
	DGauss *  my_gauss = new DGauss(c, r);
	std::vector<double> fracs;
	double tot = 0.0;

	std::vector<Cpix_hit*>::iterator ipix;
	for (ipix = get_pix_hits().begin() ; ipix != get_pix_hits().end(); ++ipix) {
		double frac_num = my_gauss->integral((*ipix)->get_column()-0.5, (*ipix)->get_column()+0.5,
											 (*ipix)->get_row()-0.5, (*ipix)->get_row()+0.5);

		fracs.push_back(frac_num);
		tot += frac_num;
	}


	int i=0;
	for (ipix = get_pix_hits().begin() ; ipix != get_pix_hits().end(); ++ipix) {
		double pull = (*ipix)->get_ADC()/(double)get_ADC() - fracs[i]/tot;
		chi += pow(pull, 2);
		i++;
	}

	return chi;
}



//_____________________________________________________________________________
//_____________________________________________________________________________



double Ccluster::pred_left_frac(double r, double c) {
	DGauss my_gauss(c, r);
	Cpix_hit * pix_left;
	Cpix_hit * pix_right;

	// Working in local co-ordinate systems.
	if (this->get_pix_hits()[0]->get_column() > this->get_pix_hits()[1]->get_column()){
		pix_left = this->get_pix_hits()[1];
		pix_right = this->get_pix_hits()[0];
	}

	else {
		pix_left = this->get_pix_hits()[0];
		pix_right = this->get_pix_hits()[1];
	}

	double denom = my_gauss.integral(pix_left->get_column()-0.5,
									 pix_right->get_column()+0.5,
									 pix_left->get_row()-0.5,
									 pix_left->get_row()+0.5);


	double num = my_gauss.integral(pix_left->get_column()-0.5,
									 pix_left->get_column()+0.5,
									 pix_left->get_row()-0.5,
									 pix_left->get_row()+0.5);


	return num/denom;
}



double Ccluster::pred_right_frac(double r, double c) {
	DGauss my_gauss(c, r);
	Cpix_hit * pix_left;
	Cpix_hit * pix_right;

	// Working in local co-ordinate systems.
	if (this->get_pix_hits()[0]->get_column() > this->get_pix_hits()[1]->get_column()){
		pix_left = this->get_pix_hits()[1];
		pix_right = this->get_pix_hits()[0];
	}

	else {
		pix_left = this->get_pix_hits()[0];
		pix_right = this->get_pix_hits()[1];
	}

	double denom = my_gauss.integral(pix_left->get_column()-0.5,
									 pix_right->get_column()+0.5,
									 pix_left->get_row()-0.5,
									 pix_left->get_row()+0.5);

	
	double num = my_gauss.integral(pix_right->get_column()-0.5,
									 pix_right->get_column()+0.5,
									 pix_right->get_row()-0.5,
									 pix_right->get_row()+0.5);
	return num/denom;
}