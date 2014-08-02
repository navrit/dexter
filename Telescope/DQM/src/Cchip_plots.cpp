#include "../headers/Cchip_plots.h"

//-----------------------------------------------------------------------------

Cchip_plots::Cchip_plots(CDQM_options * ops){
	std::cout<<"Constructor of Cchip_plots"<<std::endl;

	_save_file_name = "Default_save_name";
	_chip_loop_cut = 10;
	_tel = NULL;
	_ops = ops;
}






//-----------------------------------------------------------------------------

void Cchip_plots::initialize(){
	_save_file_name = _ops->save_file_name;
	set_directories();
}


//-----------------------------------------------------------------------------

void Cchip_plots::execute(Ctel_chunk * tel){
	_tel = tel;
	plot_all();
}

//-----------------------------------------------------------------------------

void Cchip_plots::finalize(){
}


//-----------------------------------------------------------------------------

void Cchip_plots::plot_all(){
	plot_cluster_t_to_ID();
	plot_cluster_x_to_ID();
	plot_pix_t_to_ID();
	plot_pix_x_to_ID();

	save_figs();
}






//-----------------------------------------------------------------------------

void Cchip_plots::plot_cluster_x_to_ID(){
	double xlow, xup, xstep;

	for (std::vector<Cchip*>::iterator ichip = _tel->get_chips().begin();
		ichip != _tel->get_chips().end(); ++ichip){
		if ((*ichip)->get_nclusters()!=0){

			int num_dots = (*ichip)->_glob_xs_to_clustIDs.size() * 100;
			std::vector<Ccluster*> clusters = (*ichip)->get_clusters_by_glob_x();

			double xs[num_dots];
			double ys[num_dots];

			xlow = clusters[0]->get_gx();
			xup = clusters.back()->get_gx();
			xstep = (xup-xlow)/(double)num_dots;

			for (int i=0; i<num_dots; i++){
				xs[i] = xlow+i*xstep;
				ys[i] = (*ichip)->glob_x_to_clustID(xs[i]);
			}

			TGraph * g = new TGraph(num_dots, xs, ys);
			_clust_x_to_IDs.push_back(g);


			std::stringstream s; s<<(*ichip)->get_ID();
			std::string name = "chip_" + s.str();
			g->SetName(name.c_str());
							
		}

		if ((*ichip)->get_ID() == _chip_loop_cut) break;
	}
}






//-----------------------------------------------------------------------------

void Cchip_plots::plot_pix_x_to_ID(){
	double xlow, xup, xstep;

	for (std::vector<Cchip*>::iterator ichip = _tel->get_chips().begin();
		ichip != _tel->get_chips().end(); ++ichip){
		if ((*ichip)->get_npix_hits()!=0){

			//Setup arrays to hold the points on the plot.
			int num_dots = (*ichip)->_glob_xs_to_pixIDs.size() * 100;
			std::vector<Cpix_hit*> pix_hits = (*ichip)->get_pix_hits_by_glob_x();
			double xs[num_dots];
			double ys[num_dots];


			xlow = (*ichip)->get_pix_dir(pix_hits[0], 0);
			xup = (*ichip)->get_pix_dir(pix_hits[pix_hits.size()-1], 0);
			xstep = (xup-xlow)/(double)num_dots;


			//Now cycle over this range of xs, using the relation functions in Cchip.
			for (int i=0; i<num_dots; i++){
				xs[i] = xlow+i*xstep;
				ys[i] = (*ichip)->glob_x_to_pixID(xs[i]);
			}


			//Keeps for saves.
			TGraph * g = new TGraph(num_dots, xs, ys);
			_pix_x_to_IDs.push_back(g);



			//Name and cut.
			std::stringstream s; s<<(*ichip)->get_ID();
			std::string name = "chip_" + s.str();
			g->SetName(name.c_str());
		}
		if ((*ichip)->get_ID() == _chip_loop_cut) break;

	}
}






//-----------------------------------------------------------------------------

void Cchip_plots::plot_cluster_t_to_ID(){
	double tlow, tup, tstep;

	for (std::vector<Cchip*>::iterator ichip = _tel->get_chips().begin();
		ichip != _tel->get_chips().end(); ++ichip){
		if ((*ichip)->get_nclusters()!=0){

			int num_dots = (*ichip)->_loc_ts_to_clustIDs.size() * 100;
			std::vector<Ccluster*> clusters = (*ichip)->get_clusters();

			double xs[num_dots];
			double ys[num_dots];

			tlow = clusters[0]->get_gt();
			tup = clusters.back()->get_gt();
			tstep = (tup-tlow)/(double)num_dots;

			for (int i=0; i<num_dots; i++){
				xs[i] = tlow+i*tstep;
				ys[i] = (*ichip)->glob_t_to_clustID(xs[i]);
			}

			TGraph * g = new TGraph(num_dots, xs, ys);
			_clust_t_to_IDs.push_back(g);

			std::stringstream s; s<<(*ichip)->get_ID();
			std::string name = "chip_" + s.str();
			g->SetName(name.c_str());
		}
		if ((*ichip)->get_ID() == _chip_loop_cut) break;
	}
}






//-----------------------------------------------------------------------------

void Cchip_plots::plot_pix_t_to_ID(){
	double tlow, tup, tstep;

	for (std::vector<Cchip*>::iterator ichip = _tel->get_chips().begin();
		ichip != _tel->get_chips().end(); ++ichip){
		if ((*ichip)->get_npix_hits()!=0){

			int num_dots = (*ichip)->_glob_xs_to_pixIDs.size() * 100;
			std::vector<Cpix_hit*> pix_hits = (*ichip)->get_pix_hits();

			double xs[num_dots];
			double ys[num_dots];

			tlow = pix_hits[0]->get_TOA() - (*ichip)->get_gt();
			tup = pix_hits[pix_hits.size()-1]->get_TOA() - (*ichip)->get_gt();
			tstep = (tup-tlow)/(double)num_dots;

			for (int i=0; i<num_dots; i++){
				xs[i] = tlow+i*tstep;
				ys[i] = (*ichip)->glob_t_to_pixID(xs[i]);
			}

			TGraph * g = new TGraph(num_dots, xs, ys);
			_pix_t_to_IDs.push_back(g);

			std::stringstream s; s<<(*ichip)->get_ID();
			std::string name = "chip_" + s.str();
			g->SetName(name.c_str());
		}
		if ((*ichip)->get_ID() == _chip_loop_cut) break;
	}
}






//-----------------------------------------------------------------------------

void Cchip_plots::save_figs(){
	std::cout<<"Hi Dan!"<<std::endl;
	TFile* save_file = new TFile(_save_file_name.c_str(),"update");
	std::string temp_name = "Chip_plots/";
	std::string name;

	save_file->cd();


	//Single plots first (in instance directory).
	save_file->cd("Chip_plots/");

	name = temp_name + "Pix_t_to_IDs";
	save_file->cd(name.c_str());
	for (int iplot = 0; iplot<_pix_t_to_IDs.size(); iplot++){
		_pix_t_to_IDs[iplot]->Write("", TObject::kOverwrite);
	}



	name = temp_name + "Pix_x_to_IDs";
	save_file->cd(name.c_str());
	for (int iplot = 0; iplot<_pix_x_to_IDs.size(); iplot++){
		_pix_x_to_IDs[iplot]->Write("", TObject::kOverwrite);
	}



	name = temp_name + "Clust_t_to_IDs";
	save_file->cd(name.c_str());
	for (int iplot = 0; iplot<_clust_t_to_IDs.size(); iplot++){
		_clust_t_to_IDs[iplot]->Write("", TObject::kOverwrite);
	}



	name = temp_name + "Clust_x_to_IDs";
	save_file->cd(name.c_str());
	for (int iplot = 0; iplot<_clust_x_to_IDs.size(); iplot++){
		_clust_x_to_IDs[iplot]->Write("", TObject::kOverwrite);
	}


	save_file->Close();
	std::cout<<"- Saved chip plots -"<<std::endl;
}








//-----------------------------------------------------------------------------

void Cchip_plots::set_directories(){
	//Make a directory to save everything in this instance.
	TFile* save_file = new TFile(_save_file_name.c_str(),"update");

	save_file->cd();
	std::string name = "Chip_plots";
	TDirectory* instance_direc = save_file->mkdir(name.c_str());
	instance_direc->cd();


	//Make the subdirectories.
	instance_direc->mkdir("Pix_t_to_IDs");
	instance_direc->mkdir("Pix_x_to_IDs");
	instance_direc->mkdir("Clust_t_to_IDs");
	instance_direc->mkdir("Clust_x_to_IDs");

	save_file->Close();
}








//-----------------------------------------------------------------------------

Cchip_plots::~Cchip_plots(){
	//Make a directory to save everything in this instance.
	for (int iplot = 0; iplot<_pix_t_to_IDs.size(); iplot++)
		delete _pix_t_to_IDs[iplot];

	for (int iplot = 0; iplot<_pix_x_to_IDs.size(); iplot++)
		delete _pix_x_to_IDs[iplot];

	for (int iplot = 0; iplot<_clust_t_to_IDs.size(); iplot++)
		delete _clust_t_to_IDs[iplot];

	for (int iplot = 0; iplot<_clust_x_to_IDs.size(); iplot++)
		delete _clust_x_to_IDs[iplot];

}








//-----------------------------------------------------------------------------
