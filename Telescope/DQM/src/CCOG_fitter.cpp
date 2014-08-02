// The aim of this class is to plot the resolution of each chip as a function
// of non-linear COG power.

#include "../headers/CCOG_fitter.h"
#include <sstream>


//-----------------------------------------------------------------------------

CCOG_fitter::CCOG_fitter(CDQM_options * ops){
	std::cout<<"Constructor of CCOG_fitter."<<std::endl;
	_ops = ops;
	_SampleNum = 0;
}






//-----------------------------------------------------------------------------

void CCOG_fitter::initialize(){
	//Default values.
	_SampleNum = 0;
	_save_file_name = _ops->save_file_name;
	_chip_loop_cut = _ops->chip_cut;

	_alpha_low = 1.3;
	_alpha_high = 2.6;
	_nalphas = 20;

	_alpha_step = (_alpha_high - _alpha_low)/(double)_nalphas;

	set_directories();
}


//-----------------------------------------------------------------------------

void CCOG_fitter::execute(Ctel_chunk * tel){
	_tel = tel;
	if (_SampleNum == 0) plot_method();
}


//-----------------------------------------------------------------------------

void CCOG_fitter::plot_method(){
	fill_plot_data();
	for (int i=0; i<_ops->nchips; i++){
		double xs[_nalphas], ys[_nalphas], alphas[_nalphas], del_xs[_nalphas], del_ys[_nalphas], del_alphas[_nalphas];

		for (int j=0; j<_nalphas; j++){
			xs[j] = _xrs[j][i];
			ys[j] = _yrs[j][i];

			del_xs[j] = _del_xrs[j][i];
			del_ys[j] = _del_yrs[j][i];

			del_alphas[j] = 0.0;
			alphas[j] = _alpha_low + j*_alpha_step;

			//std::cout<<xs[j]<<"\t"<<ys[j]<<"\t"<<del_xs[j]<<"\t"<<del_ys[j]<<"\t"<<del_alphas[j]<<std::endl;
		}

		// TGraphErrors * gx = new TGraphErrors(_nalphas, xs, alphas, del_xs, del_alphas);
		// TGraphErrors * gy = new TGraphErrors(_nalphas, ys, alphas, del_ys, del_alphas);

		TGraph * gx = new TGraph(_nalphas, alphas, xs);
		TGraph * gy = new TGraph(_nalphas, alphas, ys);


		for (int j=0; j<_nalphas; j++)
			std::cout<<xs[j]<<"\t"<<alphas[j]<<std::endl;

		std::stringstream ss;  ss<<i;
		std::string name = "chip_" + ss.str();

		gx->SetName(name.c_str());
		gy->SetName(name.c_str());
		_gxs.push_back(gx);
		_gys.push_back(gy);
	}

	save_figs();
}




//-----------------------------------------------------------------------------

void CCOG_fitter::fill_plot_data(){
	for (int i=0; i<_nalphas; i++){
		double alpha = _alpha_low + i*_alpha_step;
		set_cluster_positions(alpha);

		std::vector<double> xrs; // Residuals by chip, for this alpha value.
		std::vector<double> del_xrs;
		std::vector<double> yrs;
		std::vector<double> del_yrs; 

		Ctrack_plots track_plots(_ops);
		track_plots.initialize();
		track_plots.execute(_tel);

		double r, delr;
		for (int i=0; i<_ops->nchips; i++){
			track_plots.get_GaussResidual(0, i, r, delr);
			xrs.push_back(r);
			del_xrs.push_back(delr);
			//std::cout<<"Dan!"<<r<<"\t\t"<<delr<<"\t\t"<<alpha<<std::endl;

			track_plots.get_GaussResidual(1, i, r, delr);
			yrs.push_back(r);
			del_yrs.push_back(delr);

			//std::cout<<"Dan!"<<r<<"\t\t"<<delr<<std::endl;
		}

		_xrs.push_back(xrs);
		_del_xrs.push_back(del_xrs);
		_yrs.push_back(yrs);
		_del_yrs.push_back(del_yrs);
	}

	set_cluster_positions(_ops->COGweight);
}




//-----------------------------------------------------------------------------

void CCOG_fitter::set_cluster_positions(double alpha){
	for (int i=0; i<_ops->nchips; i++){
		std::vector<Ccluster*>::iterator iclust;
		for (iclust = _tel->get_chip(i)->get_clusters().begin() ; 
			 iclust != _tel->get_chip(i)->get_clusters().end(); ++iclust) {

			//std::cout<<(*iclust)->get_row()<<"\t"<<get_column()<<std::endl;
			bool OldDanCorrect = _ops->DanCorrectPosn;
			_ops->DanCorrectPosn = false;
			(*iclust)->set_cluster_position(alpha);
			_ops->DanCorrectPosn = OldDanCorrect;

			double temp_lposn[4], temp_gposn[4];
			(*iclust)->get_lposn(temp_lposn);
			_tel->get_chip(i)->lposn_to_gposn(temp_lposn, temp_gposn);
			(*iclust)->set_gposn(temp_gposn);
		}
	}
}




//-----------------------------------------------------------------------------

void CCOG_fitter::finalize(){
}





//-----------------------------------------------------------------------------

CCOG_fitter::~CCOG_fitter(){
 	std::cout<<"\n\nDeleting CCOG_fitter."<<std::endl;
}





//-----------------------------------------------------------------------------

void CCOG_fitter::set_directories(){
	TFile* save_file = new TFile(_save_file_name.c_str(),"update");

	//Make a directory to save everything in this instance.
	save_file->cd();
	std::string name = "CCOG_fitter";
	TDirectory* instance_direc = save_file->mkdir(name.c_str());
	instance_direc->cd();


	instance_direc->mkdir("xResiduals_vs_alpha");
	instance_direc->mkdir("yResiduals_vs_alpha");

	save_file->cd();
	save_file->Close();
}





//-----------------------------------------------------------------------------

void CCOG_fitter::save_figs(){
	TFile* save_file = new TFile(_save_file_name.c_str(),"update");
	std::string temp_name = "CCOG_fitter/";
	save_file->cd(temp_name.c_str());


	std::string name;
	name = temp_name + "xResiduals_vs_alpha";
	save_file->cd(name.c_str());
	for (int iplot = 0; iplot<_gxs.size(); iplot++){
		_gxs[iplot]->Write("", TObject::kOverwrite);
	}

	name = temp_name + "yResiduals_vs_alpha";
	save_file->cd(name.c_str());
	for (int iplot = 0; iplot<_gys.size(); iplot++){
		_gys[iplot]->Write("", TObject::kOverwrite);
	}

	save_file->Close();
	std::cout<<"- Saved COG_fitter plts -\n";
}









//-----------------------------------------------------------------------------
