#include "../headers/Cpixel_plots.h"
#include <sstream>


//-----------------------------------------------------------------------------

 Cpixel_plots::Cpixel_plots(CDQM_options * ops){
	std::cout<<"Constructor of Cpixel_plots"<<std::endl;


	//Default values.
	_ops = ops;
}





//-----------------------------------------------------------------------------
void Cpixel_plots::initialize(){
	std::cout<<"Init of Cpixel_plots"<<std::endl;
	_save_file_name = _ops->save_file_name;
	_z_distribution_initialized = false;
	_chip_loop_cut = _ops->chip_cut;


	_ADC_low = 0.0;
	_ADC_up = 200;
	_ADC_nbins = _ops->pix_ToT_nbins;


	_t_low = 0.0;
	_t_up = 1000000;
	_z_low = -100;
	_z_up = 1000;

	_z_low -= 0.1*(_z_up - _z_low);
	_z_up += 0.1*(_z_up - _z_low);
	_t_low -= 0.1*(_t_up - _t_low);
	_t_up += 0.1*(_t_up - _t_low);


	_z_nbins = 1000;
	_hitmap_nbins = _ops->pix_hitmap_nbins;



	init_ADC_dists();
	init_t_dists();
	init_z_dists();
	init_occupancies();
	init_hitmaps();
}






//-----------------------------------------------------------------------------

void Cpixel_plots::execute(Ctel_chunk * tel){
	_tel = tel;

	appto_ADC_dists();
	appto_t_dists();
	appto_z_dist();
	appto_occupancies();
	appto_hitmaps();

	make_pixel_t_orders();
	make_pixel_x_orders();
}





//-----------------------------------------------------------------------------

void Cpixel_plots::finalize(){
	set_directories();
	save_figs();
}





//-----------------------------------------------------------------------------

void Cpixel_plots::init_hitmaps(){
	//Creates the hitmap 2d histograms. Work in units of pixels and local 
	//co-ordinate systems.

	//Cycle over the chips.
	for (int ichip = 0; ichip<_ops->nchips; ++ichip){

		//Construct the empty histogram.
		int npixs = 256;
		std::stringstream ss_chipid; ss_chipid << ichip;
		std::string name = "Chip_" + ss_chipid.str();
		std::string title = "Pix_Hitmap" + ss_chipid.str() + ";x (/mm); y(/mm)";
		TH2F* h = new TH2F(name.c_str(), title.c_str(), _hitmap_nbins, 0, npixs,
													   _hitmap_nbins, 0, npixs);
		_hitmaps.push_back(h);

		if (ichip == _chip_loop_cut) break;
	}
}








//-----------------------------------------------------------------------------

void Cpixel_plots::init_ADC_dists(){
	//Creates the landau histograms. Each landau to be scaled individually.

	//Cycle over the chips.
	for (int ichip = 0; ichip<_ops->nchips; ++ichip){

		//Construct the empty histogram.
		std::stringstream ss_chipid; ss_chipid << ichip;
		std::string name = "Chip_" + ss_chipid.str();
		std::string title = "Pix_ToT_Dist" + ss_chipid.str() + ";ToT;N";
		TH1F* h = new TH1F(name.c_str(), title.c_str(), _ADC_nbins, _ADC_low, _ADC_up);
		_ADC_dists.push_back(h);

		if (ichip == _chip_loop_cut) break;
	}
}








//-----------------------------------------------------------------------------

void Cpixel_plots::appto_hitmaps(){

	//Cycle over the chips.
	std::vector<Cchip*>::iterator ichip;
	for (ichip = _tel->get_chips().begin();
		 ichip != _tel->get_chips().end(); ++ichip){


		//Fill (readymade) histograms.
		std::vector<Cpix_hit*>::iterator ipix;
		for (ipix = (*ichip)->get_pix_hits().begin();
			 ipix != (*ichip)->get_pix_hits().end(); ipix++){

			_hitmaps[(*ichip)->get_ID()]->Fill((*ipix)->get_column(), (*ipix)->get_row());
		}

		if ((*ichip)->get_ID() == _chip_loop_cut) break;
	}
}








//-----------------------------------------------------------------------------

void Cpixel_plots::appto_ADC_dists(){

	//Cycle over the chips.
	std::vector<Cchip*>::iterator ichip;
	for (ichip = _tel->get_chips().begin();
		 ichip != _tel->get_chips().end(); ++ichip){


		//Fill (readymade) histograms.
		std::vector<Cpix_hit*>::iterator ipix;
		for (ipix = (*ichip)->get_pix_hits().begin();
			 ipix != (*ichip)->get_pix_hits().end(); ipix++){

			_ADC_dists[(*ichip)->get_ID()]->Fill((*ipix)->get_ADC());
		}

		if ((*ichip)->get_ID() == _chip_loop_cut) break;
	}
}








//-----------------------------------------------------------------------------

void Cpixel_plots::init_t_dists(){

	//Cycle over the chips.
	for (int i=0; i<_ops->nchips; i++){
		std::stringstream ss;
		ss<<i;
		//Construct the empty histogram.
		std::string name = "Pix_TOA_Dist" + ss.str() + ";t (/DAQ time unit); N";
		TH1F* h = new TH1F(name.c_str(), name.c_str(), 100, _t_low, _t_up);
		_t_dists.push_back(h);

		if (i == _chip_loop_cut) break;
	}
}








//-----------------------------------------------------------------------------

void Cpixel_plots::appto_t_dists(){

	//Cycle over the chips.
	std::vector<Cchip*>::iterator ichip;
	for (ichip = _tel->get_chips().begin();
		 ichip != _tel->get_chips().end(); ++ichip){


		//Fill (readymade) histograms.
		std::vector<Cpix_hit*>::iterator ipix;
		for (ipix = (*ichip)->get_pix_hits().begin();
			 ipix != (*ichip)->get_pix_hits().end(); ipix++){

			_t_dists[(*ichip)->get_ID()]->Fill((*ipix)->get_TOA());
		}
		if ((*ichip)->get_ID() == _chip_loop_cut) break;
	}
}









//-----------------------------------------------------------------------------

void Cpixel_plots::init_z_dists(){
	//Construct the empty histogram.

	const char* title = "Pix_Z_Dist; Z (/mm); N";
	const char* name = "Pix_Z_Dist";
	_z_distribution = new TH1F(name, title, _z_nbins, _z_low, _z_up);
	_z_distribution_initialized = true;
}








//-----------------------------------------------------------------------------

void Cpixel_plots::appto_z_dist(){

	//Cycle over the chips.
	std::vector<Cchip*>::iterator ichip;
	for (ichip = _tel->get_chips().begin();
		 ichip != _tel->get_chips().end(); ++ichip){


		//Fill (readymade) histograms.
		std::vector<Cpix_hit*>::iterator ipix;
		for (ipix = (*ichip)->get_pix_hits().begin();
			 ipix != (*ichip)->get_pix_hits().end(); ipix++){

			_z_distribution->Fill((*ichip)->get_z());
		}
	}
}








//-----------------------------------------------------------------------------

void Cpixel_plots::init_occupancies(){

	//Cycle over the chips.
	for (int i=0; i<_ops->nchips; i++){
		std::stringstream ss;
		ss<<i;
		//Construct the empty histogram.
		std::string name = "Chip_" + ss.str();
		TH1I* h = new TH1I(name.c_str(), name.c_str(), 10, _occupancy_low, _occupancy_up);
		_occupancies.push_back(h);

		if (i == _chip_loop_cut) break;
	}
}








//-----------------------------------------------------------------------------

void Cpixel_plots::appto_occupancies(){

	//Cycle over the chips.
	std::vector<Cchip*>::iterator ichip;
	for (ichip = _tel->get_chips().begin();
		 ichip != _tel->get_chips().end(); ++ichip){

		std::vector< std::vector< std::vector<int> > >* hit_map;
		hit_map = (*ichip)->get_hit_map();


		//Cycle over the map.
		for (int i = 0; i<hit_map->size(); i++){
			for (int j = 0; j<(*hit_map)[0].size(); j++){
				_occupancies[(*ichip)->get_ID()]->Fill((*hit_map)[i][j].size());
			}
		}
		if ((*ichip)->get_ID() == _chip_loop_cut) break;
	}
}








//-----------------------------------------------------------------------------

void Cpixel_plots::make_pixel_t_orders(){
	_t_orders.clear();

	//Cycle over the chips.
	std::vector<Cchip*>::iterator ichip;
	for (ichip = _tel->get_chips().begin();
		 ichip != _tel->get_chips().end(); ++ichip){

		std::vector<Cpix_hit*> pix_hits = (*ichip)->get_pix_hits();
		int N = pix_hits.size();

		//Get xs and ys.
		double xs[N];
		double ys[N];
		for (int ipix = 0; ipix < N; ipix++){
			xs[ipix] = (double)ipix;
			ys[ipix] = pix_hits[ipix]->get_TOA();
		}


		//Construct graph.
		TGraph* g = new TGraph(N, xs, ys);
		std::string name = 	(*ichip)->get_name() + "pix_hit t ordering";
		g->SetTitle(name.c_str());
		_t_orders.push_back(g);

		if ((*ichip)->get_ID() == _chip_loop_cut) break;
	}
}








//-----------------------------------------------------------------------------

void Cpixel_plots::make_pixel_x_orders(){
	_x_orders.clear();

	//Cycle over the chips.
	std::vector<Cchip*>::iterator ichip;
	for (ichip = _tel->get_chips().begin();
		 ichip != _tel->get_chips().end(); ++ichip){

		std::vector<Cpix_hit*> pix_hits = (*ichip)->get_pix_hits_by_glob_x();
		int N = pix_hits.size();

		//Get xs and ys.
		double xs[N];
		double ys[N];
		for (int ipix = 0; ipix < N; ipix++){
			ys[ipix] = (double)ipix;
			xs[ipix] = (*ichip)->get_pix_dir(pix_hits[ipix], 0);
		}


		//Construct graph.
		TGraph* g = new TGraph(N, xs, ys);
		std::string name = 	(*ichip)->get_name() + "pix_hit x ordering";
		g->SetTitle(name.c_str());
		_x_orders.push_back(g);

		if ((*ichip)->get_ID() == _chip_loop_cut) break;
	}
}








//-----------------------------------------------------------------------------

void Cpixel_plots::save_figs(){
	std::cout<<"Saving pixel plots..."<<std::endl;
	TFile* save_file = new TFile(_save_file_name.c_str(),"update");


	std::string temp_name = "Pixel_plots/";
	std::string name;
	save_file->cd();


	//Single plots first (in instance directory).
	save_file->cd("Pixel_plots");

	if (_z_distribution_initialized) _z_distribution->Write("", TObject::kOverwrite);

	name = temp_name + "ToT_dists";
	save_file->cd(name.c_str());
	for (int iplot = 0; iplot<_ADC_dists.size(); iplot++){
		_ADC_dists[iplot]->Write("", TObject::kWriteDelete);
	}


	name = temp_name + "t_dists";
	save_file->cd(name.c_str());
	for (int iplot = 0; iplot<_t_dists.size(); iplot++){
		_t_dists[iplot]->Write("", TObject::kWriteDelete);
	}


	name = temp_name + "occupancies";
	save_file->cd(name.c_str());
	for (int iplot = 0; iplot<_occupancies.size(); iplot++){
		_occupancies[iplot]->Write("", TObject::kWriteDelete);
	}


	name = temp_name + "t_loc_orders";
	save_file->cd(name.c_str());
	for (int iplot = 0; iplot<_t_orders.size(); iplot++){
		std::stringstream s; s<<iplot;
		std::string name = "t_order_chip_" + s.str();
		_t_orders[iplot]->Write(name.c_str(), TObject::kWriteDelete);
	}


	name = temp_name + "x_glob_orders";
	save_file->cd(name.c_str());
	for (int iplot = 0; iplot<_x_orders.size(); iplot++){
		std::stringstream s; s<<iplot;
		std::string name = "x_order_chip_" + s.str();
		_x_orders[iplot]->Write(name.c_str(), TObject::kWriteDelete);
	}


	name = temp_name + "hitmaps";
	save_file->cd(name.c_str());
	for (int iplot = 0; iplot<_hitmaps.size(); iplot++){
		_hitmaps[iplot]->Write("", TObject::kWriteDelete);
	}


	save_file->Close();
	std::cout<<"- Saved pix plts -\n";
}








//-----------------------------------------------------------------------------

void Cpixel_plots::set_directories(){
	TFile* save_file = new TFile(_save_file_name.c_str(),"update");
	std::string temp_name = "Pixel_plots/";
	//Make a directory to save everything in this instance.
	save_file->cd();
	std::string name = "Pixel_plots";
	TDirectory* instance_direc = save_file->mkdir(name.c_str());
	instance_direc->cd();


	//Make the subdirectories.
	TDirectory* ADC_direc = instance_direc->mkdir("ToT_dists");
	TDirectory* t_dists_direc = instance_direc->mkdir("t_dists");
	TDirectory* Occupancies_direc = instance_direc->mkdir("occupancies");
	TDirectory* t_order_direc = instance_direc->mkdir("t_loc_orders");
	TDirectory* x_order_direc = instance_direc->mkdir("x_glob_orders");
	TDirectory* hitmap_direc = instance_direc->mkdir("hitmaps");

	save_file->cd();
	save_file->Close();
}








//-----------------------------------------------------------------------------

Cpixel_plots::~Cpixel_plots(){
	for (int iplot = 0; iplot<_ADC_dists.size(); iplot++) 
		delete _ADC_dists[iplot];

	if (_z_distribution_initialized) delete _z_distribution;

	for (int iplot = 0; iplot<_t_dists.size(); iplot++) 
		delete _t_dists[iplot];

	for (int iplot = 0; iplot<_occupancies.size(); iplot++) 
		delete _occupancies[iplot];

	for (int iplot = 0; iplot<_t_orders.size(); iplot++) 
		delete _t_orders[iplot];

	for (int iplot = 0; iplot<_x_orders.size(); iplot++) 
		delete _x_orders[iplot];

	for (int iplot = 0; iplot<_hitmaps.size(); iplot++)
		delete _hitmaps[iplot];
}








//-----------------------------------------------------------------------------

