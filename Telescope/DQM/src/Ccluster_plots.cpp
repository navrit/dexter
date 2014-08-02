#include "../headers/Ccluster_plots.h"
#include <sstream>


//-----------------------------------------------------------------------------

Ccluster_plots::Ccluster_plots(CDQM_options * ops){
	std::cout<<"Constructor of Ccluster_plots"<<std::endl;
	_ops = ops;
	_tel = NULL;
	_save_file_name = "Default_file_name";
	_chip_loop_cut = 10;
	_ref_chip = 5;
	_SampleNum = 0;

	_ToT_low = 0.0;
	_ToT_up = 500.0;
	_ToT_nbins = 500;

	_t_low = -1.0e9;
	_t_up = 3.0e10;
	_tdist_nbins = 1000;

	_size_low = -0.5;
	_size_up = 14.5;
	_size_nbins = 15;

	_z_distributions_initialized = false;
	_z_low = -10;
	_z_up = 410;
	_z_nbins = 500;

	_hitmap_nbins = 256;
	_ncluster_samples = 16;
	_root_ncluster_samples = 4;
	_sample_spacing = 12;
}



//-----------------------------------------------------------------------------

void Ccluster_plots::initialize(){
	std::cout<<"Initializer of Ccluster_plots"<<std::endl;
	_SampleNum = 0;

	//Default values.
	_save_file_name = _ops->save_file_name;
	_z_distributions_initialized = false;
	_chip_loop_cut = _ops->chip_cut;


	//Default (but not auto) cuts.
	_hitmap_nbins = _ops->clust_hitmap_nbins;
	_ToT_nbins = _ops->clust_ToT_nbins;


	init_ToT_dists();
	init_t_dists();
	init_z_dists();
	init_hitmaps();
	init_size_dists();
	init_pixDeltaTs();
	//init_sharers();
}





//-----------------------------------------------------------------------------

void Ccluster_plots::execute(Ctel_chunk * tel){
	// Should have one telescope per use of this function.
	_tel = tel;

	// Appendable plots.
	appto_ToT_dists();
	appto_t_dists();
	appto_size_dists();
	appto_z_dists();
	appto_hitmaps();
	appto_pixDeltaTs();
	
	//appto_sharers();

	// Replaces plots.
	make_cluster_t_orders();
	make_cluster_x_orders();
	if (_SampleNum == 0) {
		make_cluster_samples();
		appto_sampleHitmaps();
	}
	//save_figs();
	_SampleNum++;
}



//-----------------------------------------------------------------------------

void Ccluster_plots::finalize(){
	set_directories();
	save_figs();
}




//-----------------------------------------------------------------------------

void Ccluster_plots::init_pixDeltaTs(){
	std::stringstream ssChip;
	std::string name = "Chip";
	std::string name2;
	std::string title1 = "PixDeltaTs; delta t (26/16 ns); N";
	std::string title2 = "PixToT_vs_Pix_minus_CulsterTOA; ToT; delta t (26/16 ns);";
	for (int ichip = 0; ichip<_ops->nchips; ++ichip){
		ssChip<<ichip;
		name2 = name + ssChip.str();
		_pixDeltaTs.push_back(new TH1F(name2.c_str(), title1.c_str(), 300, -30, 120));
		_pixDeltaTsVsCharge.push_back(new TH2F(name2.c_str(), title2.c_str(), 300, 0, 150, 300, -30, 120));
	}

	name2 = name + "ChipAll";
	_pixDeltaTs.push_back(new TH1F(name2.c_str(), title1.c_str(), 300, -30, 120));
	_pixDeltaTsVsCharge.push_back(new TH2F(name2.c_str(), title2.c_str(), 300, 0, 150, 300, -30, 120));
}







//-----------------------------------------------------------------------------

void Ccluster_plots::init_hitmaps(){
	//Creates the hitmap 2d histograms. Work in units of pixels and local 
	//co-ordinate systems.

	//Cycle over the chips.
	for (int ichip = 0; ichip<_ops->nchips; ++ichip){

		//Construct the empty histogram.
		int nbins = 400;
		std::stringstream ss_chipid; 
		ss_chipid << ichip;
		std::string name = "Chip_" + ss_chipid.str();
		std::string title = "Clusts_Hitmaps_Local" + ss_chipid.str() + "; x (/pixels); y (/pixels)";
		TH2F* h = new TH2F(name.c_str(), title.c_str(), _hitmap_nbins, 0, 256,
													   _hitmap_nbins, 0, 256);
		_hitmapsLocal.push_back(h);

		title = "Clusts_Hitmaps_Global" + ss_chipid.str() + "; x (/mm); y (/mm)";
		h = new TH2F(name.c_str(), title.c_str(), _hitmap_nbins, -3, 17.08,
													   _hitmap_nbins, -3, 17.08);

		_hitmapsGlobal.push_back(h);
		TH1F* hp;
		title = "Clusts_Global_XDist" + ss_chipid.str() + "; x (/mm); N";
		hp = new TH1F(name.c_str(), title.c_str(), _hitmap_nbins, -3, 17.08);

		_hitmapsGlobalXDist.push_back(hp);

		title = "Clusts_Global_YDist" + ss_chipid.str() + "; y (/mm); N";
		hp = new TH1F(name.c_str(), title.c_str(), _hitmap_nbins, -3, 17.08);

		_hitmapsGlobalYDist.push_back(hp);

		// Sample hitmaps.
		name = "ClusteringTimeWindowPixelHitmap" + ss_chipid.str() + "; x(/mm); y (/mm)";
		TH2F* h1 = new TH2F(name.c_str(), title.c_str(), _hitmap_nbins, 0, nbins,
													   _hitmap_nbins, 0, nbins);
		_sampleHitmaps.push_back(h1);

		if (ichip == _chip_loop_cut) break;
	}
}





//-----------------------------------------------------------------------------

void Ccluster_plots::init_ToT_dists(){
	//Creates the landau histograms. Each landau to be scaled individually.

	//Cycle over the chips.
	for (int ichip = 0; ichip<_ops->nchips; ++ichip){

		//Construct the empty histogram.
		std::stringstream ss_chipid; 
		ss_chipid << ichip;
		std::string name = "Chip_" + ss_chipid.str();
		std::string title = "Clusts_ToT_Dist" + ss_chipid.str() + "; ToT; N";
		TH1F* h = new TH1F(name.c_str(), title.c_str(), _ToT_nbins, _ToT_low, _ToT_up);
		_ToT_dists.push_back(h);

		title = "Size1Clusts_ToT_Dist" + ss_chipid.str() + "; ToT; N";
		h = new TH1F(name.c_str(), title.c_str(), _ToT_nbins, _ToT_low, _ToT_up);
		_ToT_Size1_dists.push_back(h);

		title = "Size2Clusts_ToT_Dist" + ss_chipid.str() + "; ToT; N";
		h = new TH1F(name.c_str(), title.c_str(), _ToT_nbins, _ToT_low, _ToT_up);
		_ToT_Size2_dists.push_back(h);

		title = "Size3Clusts_ToT_Dist" + ss_chipid.str() + "; ToT; N";
		h = new TH1F(name.c_str(), title.c_str(), _ToT_nbins, _ToT_low, _ToT_up);
		_ToT_Size3_dists.push_back(h);

		title = "Size4Clusts_ToT_Dist" + ss_chipid.str() + "; ToT; N";
		h = new TH1F(name.c_str(), title.c_str(), _ToT_nbins, _ToT_low, _ToT_up);
		_ToT_Size4_dists.push_back(h);
		

		if (ichip == _chip_loop_cut) break;
	}
}








//-----------------------------------------------------------------------------

void Ccluster_plots::init_size_dists(){
	//Creates the landau histograms. Each landau to be scaled individually.

	//Cycle over the chips.
	for (int ichip = 0; ichip<_ops->nchips; ++ichip){

		//Construct the empty histogram.
		std::stringstream ss_chipid; 
		ss_chipid << ichip;
		std::string name = "Chip_" + ss_chipid.str();
		std::string title = "Clusts_Size_Dist" + ss_chipid.str() + "; size (int); N";
		TH1F* h = new TH1F(name.c_str(), title.c_str(), _size_nbins, _size_low, _size_up);
		_size_dists.push_back(h);

		if (ichip == _chip_loop_cut) break;
	}
}








//-----------------------------------------------------------------------------

void Ccluster_plots::appto_hitmaps(){

	//Cycle over the chips.
	std::vector<Cchip*>::iterator ichip;
	for (ichip = _tel->get_chips().begin();
		 ichip != _tel->get_chips().end(); ++ichip){


		//Fill (readymade) histograms.
		std::vector<Ccluster*>::iterator iclust;
		for (iclust = (*ichip)->get_clusters().begin();
			 iclust != (*ichip)->get_clusters().end(); iclust++) {

			_hitmapsLocal[(*ichip)->get_ID()]->Fill((*iclust)->get_column(), (*iclust)->get_row());
			_hitmapsGlobal[(*ichip)->get_ID()]->Fill((*iclust)->get_gx(), (*iclust)->get_gy());
			_hitmapsGlobalXDist[(*ichip)->get_ID()]->Fill((*iclust)->get_gx());
			_hitmapsGlobalYDist[(*ichip)->get_ID()]->Fill((*iclust)->get_gy());
		}

		if ((*ichip)->get_ID() == _chip_loop_cut) break;
	}
}



//-----------------------------------------------------------------------------

void Ccluster_plots::appto_sampleHitmaps(){

	//Cycle over the chips.
	std::vector<Cchip*>::iterator ichip;
	for (ichip = _tel->get_chips().begin();
		 ichip != _tel->get_chips().end(); ++ichip){

		if ((*ichip)->get_nclusters() == 0) continue;

		//Fill (readymade) histograms.
		std::vector<Cpix_hit*>::iterator ipix;
		double tstart = (*ichip)->get_pix_hits()[0]->get_TOA();
		double tend = tstart + _ops->clust_maker_tup;
		for (ipix = (*ichip)->get_pix_hits().begin();
			 ipix != (*ichip)->get_pix_hits().end(); ipix++) {
			_sampleHitmaps[(*ichip)->get_ID()]->Fill((*ipix)->get_column(), (*ipix)->get_row());

			if ((*ipix)->get_TOA() > tend) break;
		}

		if ((*ichip)->get_ID() == _chip_loop_cut) break;
	}
}









//-----------------------------------------------------------------------------

void Ccluster_plots::appto_ToT_dists(){

	//Cycle over the chips.
	std::vector<Cchip*>::iterator ichip;
	for (ichip = _tel->get_chips().begin();
		 ichip != _tel->get_chips().end(); ++ichip){


		//Fill (readymade) histograms.
		std::vector<Ccluster*>::iterator iclust;
		for (iclust = (*ichip)->get_clusters().begin();
			 iclust != (*ichip)->get_clusters().end(); iclust++){
				_ToT_dists[(*ichip)->get_ID()]->Fill((*iclust)->get_ToT());
				if ((*iclust)->get_size()==1) _ToT_Size1_dists[(*ichip)->get_ID()]->Fill((*iclust)->get_ToT());
				if ((*iclust)->get_size()==2) _ToT_Size2_dists[(*ichip)->get_ID()]->Fill((*iclust)->get_ToT());
				if ((*iclust)->get_size()==3) _ToT_Size3_dists[(*ichip)->get_ID()]->Fill((*iclust)->get_ToT());
				if ((*iclust)->get_size()==4) _ToT_Size4_dists[(*ichip)->get_ID()]->Fill((*iclust)->get_ToT());
		}

		if ((*ichip)->get_ID() == _chip_loop_cut) break;
	}
}






//-----------------------------------------------------------------------------

void Ccluster_plots::appto_size_dists(){

	//Cycle over the chips.
	std::vector<Cchip*>::iterator ichip;
	for (ichip = _tel->get_chips().begin();
		 ichip != _tel->get_chips().end(); ++ichip){


		//Fill (readymade) histograms.
		std::vector<Ccluster*>::iterator iclust;
		for (iclust = (*ichip)->get_clusters().begin();
			 iclust != (*ichip)->get_clusters().end(); iclust++){

			_size_dists[(*ichip)->get_ID()]->Fill((*iclust)->get_size());
		}

		if ((*ichip)->get_ID() == _chip_loop_cut) break;
	}
}








//-----------------------------------------------------------------------------

void Ccluster_plots::appto_pixDeltaTs(){
	int chip;
	//Cycle over the chips.
	std::vector<Cchip*>::iterator ichip;
	for (ichip = _tel->get_chips().begin();
		 ichip != _tel->get_chips().end(); ++ichip){

		chip = (*ichip)->get_ID();
		//Fill (readymade) histograms.
		std::vector<Ccluster*>::iterator iclust;
		for (iclust = (*ichip)->get_clusters().begin();
			 iclust != (*ichip)->get_clusters().end(); iclust++){

			std::vector<Cpix_hit*>::iterator ipix;
			for (ipix = (*iclust)->get_pix_hits().begin();
				 ipix != (*iclust)->get_pix_hits().end(); ipix++){

				double deltaT = (*ipix)->get_TOA() - (*iclust)->get_TOA();
				if ((*iclust)->earliestHit() != (*ipix)) {
					_pixDeltaTs[chip]->Fill(deltaT);
					_pixDeltaTsVsCharge[chip]->Fill((*ipix)->get_ADC(), deltaT);

					_pixDeltaTs[_pixDeltaTs.size()-1]->Fill(deltaT);
					_pixDeltaTsVsCharge[_pixDeltaTsVsCharge.size()-1]->Fill((*ipix)->get_ADC(), deltaT);
				}
			}

		}

		if ((*ichip)->get_ID() == _chip_loop_cut) break;
	}
}








//-----------------------------------------------------------------------------

void Ccluster_plots::init_t_dists(){

	//Cycle over the chips.
	for (int ichip = 0; ichip<_ops->nchips; ichip++){

		//Construct the empty histogram.
		std::stringstream ss_chipid; 
		ss_chipid << ichip;
		std::string name = "Chip_" + ss_chipid.str();
		TH1F* h = new TH1F(name.c_str(), name.c_str(), _tdist_nbins, _t_low, _t_up);
		_t_dists.push_back(h);

		if (ichip == _chip_loop_cut) break;
	}
}




//-----------------------------------------------------------------------------

void Ccluster_plots::init_sharers(){

	//Cycle over the chips.
	for (int ichip = 0; ichip<_ops->nchips; ichip++){

		std::stringstream ss_chipid; 
		ss_chipid << ichip;
		std::string name = "Chip_" + ss_chipid.str();
		
		TH1F* h = new TH1F(name.c_str(), name.c_str(), 30, 0.0, 0.1);
		_DCorrectChis.push_back(h);

		TH1F* h1 = new TH1F(name.c_str(), name.c_str(), 30, 0, 1);
		_PredictedLeftShare.push_back(h1);

		TH1F* h2 = new TH1F(name.c_str(), name.c_str(), 30, 0, 1);
		_PredictedRightShare.push_back(h2);

		TH1F* h3 = new TH1F(name.c_str(), name.c_str(), 30, 0, 1);
		_ActualRightShare.push_back(h3);

		TH1F* h4 = new TH1F(name.c_str(), name.c_str(), 30, 0, 1);
		_ActualLeftShare.push_back(h4);

		TH1F* h5 = new TH1F(name.c_str(), name.c_str(), 30, -0.5, 0.5);
		_DiffLeftShare.push_back(h5);

		TH1F* h6 = new TH1F(name.c_str(), name.c_str(), 30, -0.5, 0.5);
		_DiffRightShare.push_back(h6);


		if (ichip == _chip_loop_cut) break;
	}
}





//-----------------------------------------------------------------------------

void Ccluster_plots::appto_t_dists(){

	//Cycle over the chips.
	std::vector<Cchip*>::iterator ichip;
	for (ichip = _tel->get_chips().begin();
		 ichip != _tel->get_chips().end(); ++ichip){


		//Fill (readymade) histograms.
		std::vector<Ccluster*>::iterator iclust;
		for (iclust = (*ichip)->get_clusters().begin();
			 iclust != (*ichip)->get_clusters().end(); iclust++){

			_t_dists[(*ichip)->get_ID()]->Fill((*iclust)->get_TOA());
		}

		if ((*ichip)->get_ID() == _chip_loop_cut) break;
	}
}






//-----------------------------------------------------------------------------

void Ccluster_plots::appto_sharers(){

	//Cycle over the chips.
	std::vector<Cchip*>::iterator ichip;
	for (ichip = _tel->get_chips().begin();
		 ichip != _tel->get_chips().end(); ++ichip){


		//Fill (readymade) histograms.
		std::vector<Ccluster*>::iterator iclust;
		for (iclust = (*ichip)->get_clusters().begin();
			 iclust != (*ichip)->get_clusters().end(); iclust++){

			_DCorrectChis[(*ichip)->get_ID()]->Fill((*iclust)->chi2_of_posn_sharing((*iclust)->get_row(), (*iclust)->get_column()));

			if ((*iclust)->Shape() == 1){
				Cpix_hit * pix_left;
				Cpix_hit * pix_right;
				if ((*iclust)->get_pix_hits()[0]->get_column() > (*iclust)->get_pix_hits()[1]->get_column()){
					pix_left = (*iclust)->get_pix_hits()[1];
					pix_right = (*iclust)->get_pix_hits()[0];
				}

				else {
					pix_left = (*iclust)->get_pix_hits()[0];
					pix_right = (*iclust)->get_pix_hits()[1];
				}


				
				_PredictedLeftShare[(*ichip)->get_ID()]->Fill((*iclust)->pred_left_frac((*iclust)->get_row(), (*iclust)->get_column()));
				_PredictedRightShare[(*ichip)->get_ID()]->Fill((*iclust)->pred_right_frac((*iclust)->get_row(), (*iclust)->get_column()));
				_ActualRightShare[(*ichip)->get_ID()]->Fill(pix_right->get_ToT()/(double)(*iclust)->get_ToT());
				_ActualLeftShare[(*ichip)->get_ID()]->Fill(pix_left->get_ToT()/(double)(*iclust)->get_ToT());

				_DiffLeftShare[(*ichip)->get_ID()]->Fill(pix_left->get_ToT()/(double)(*iclust)->get_ToT() - (*iclust)->pred_left_frac((*iclust)->get_row(), (*iclust)->get_column()));
				_DiffRightShare[(*ichip)->get_ID()]->Fill(pix_right->get_ToT()/(double)(*iclust)->get_ToT() - (*iclust)->pred_right_frac((*iclust)->get_row(), (*iclust)->get_column()));
			}
		}

		if ((*ichip)->get_ID() == _chip_loop_cut) break;
	}
}










//-----------------------------------------------------------------------------

void Ccluster_plots::init_z_dists(){
	//Construct the empty histogram.
	_z_distributions_initialized = true;

	const char* title = "Cluster_Z_Dist; z (/mm); N";
	const char* name = "Clust_Z_Dist";
	_z_distribution = new TH1F(name, title, _z_nbins, _z_low, _z_up);
		

	title = "Cluster_ZvsX_Dist; X (/mm); Z (/mm)";
	name = "Cluster_ZvsX_Dist";
	_zVsX_distribution = new TH2F(name, title, 4000, _z_low, _z_up, 400, -3.0, 17.08);


	title = "Cluster_ZvsY_Dist; Y (/mm); Z (/mm)";
	name = "Cluster_ZvsY_Dist";
	_zVsY_distribution = new TH2F(name, title, 4000, _z_low, _z_up, 400, -3.0, 17.08);	
}








//-----------------------------------------------------------------------------

void Ccluster_plots::appto_z_dists(){

	//Cycle over the chips.
	std::vector<Cchip*>::iterator ichip;
	for (ichip = _tel->get_chips().begin();
		 ichip != _tel->get_chips().end(); ++ichip){

		std::vector<Ccluster*>::iterator iclust;
		for (iclust = (*ichip)->get_clusters().begin();
			 iclust != (*ichip)->get_clusters().end(); iclust++){

			_z_distribution->Fill((*ichip)->get_z());
			_zVsX_distribution->Fill((*iclust)->get_gz(), (*iclust)->get_gx());
			_zVsY_distribution->Fill((*iclust)->get_gz(), (*iclust)->get_gy());
		}
	}
}








//-----------------------------------------------------------------------------

void Ccluster_plots::make_cluster_samples(){
	_cluster_samples.clear(); //replacing.

	//Cycle over the chips.
	std::vector<Cchip*>::iterator ichip;
	for (ichip = _tel->get_chips().begin(); ichip != _tel->get_chips().end(); ++ichip){

		//Make the plot.
		std::stringstream ss; 
		ss<<(*ichip)->get_ID();
		std::string name = "Chip_" + ss.str();
		int nbins = _sample_spacing * _root_ncluster_samples;
		TH2F * h = new TH2F(name.c_str(), name.c_str(), nbins, 0, nbins, nbins, 0, nbins);


		std::vector<Ccluster*> cluster_sample;
		(*ichip)->fill_cluster_sample(_ncluster_samples, cluster_sample);
		//Pass to another function to fill the plot with these clusters.
		fill_cluster_sample_h(h, cluster_sample);


		//All done.
		_cluster_samples.push_back(h);

		if ((*ichip)->get_ID() == _chip_loop_cut) break;
	}
}








//-----------------------------------------------------------------------------

void Ccluster_plots::fill_cluster_sample_h(TH2F * h, std::vector<Ccluster*> & clusters){
	//Iterate over clusters.
	int n = _root_ncluster_samples;
	int i=0;
	for (std::vector<Ccluster*>::iterator iclust = clusters.begin();
		 iclust != clusters.end(); iclust++){

		int seed_x = (*iclust)->get_seed_pix()->get_column();
		int seed_y = (*iclust)->get_seed_pix()->get_row();
		
		int x = (i%n) * _sample_spacing;
		int y = floor((i/n)) * _sample_spacing;

		//Iterate over hits.
		std::vector<Cpix_hit*>::iterator ipix;
		for (ipix = (*iclust)->get_pix_hits().begin();
			 ipix != (*iclust)->get_pix_hits().end(); ipix++){

			//Shift position by seed.
			h->Fill( ((*ipix)->get_column()-seed_x + x),
					 ((*ipix)->get_row()-seed_y + y),
					 (*ipix)->get_ToT());
		}

		i++;
	}
}








//-----------------------------------------------------------------------------

void Ccluster_plots::make_cluster_x_orders(){
	_x_orders.clear(); //replacing.


	//Cycle over the chips.
	std::vector<Cchip*>::iterator ichip;
	for (ichip = _tel->get_chips().begin();
		 ichip != _tel->get_chips().end(); ++ichip){

		if ((*ichip)->get_nclusters() == 0) continue;
		std::vector<Ccluster*> clusters = (*ichip)->get_clusters_by_glob_x();
		int N = clusters.size();


		//Get xs and ys.
		double xs[N];
		double ys[N];
		for (int iclust = 0; iclust < N; iclust++){
			ys[iclust] = (double)iclust;
			xs[iclust] = clusters[iclust]->get_gx();
		}


		//Construct graph.
		TGraph* g = new TGraph(N, xs, ys);
		std::string name = 	(*ichip)->get_name() + "cluster x ordering";
		g->SetTitle(name.c_str());
		_x_orders.push_back(g);

		if ((*ichip)->get_ID() == _chip_loop_cut) break;
	}
}








//-----------------------------------------------------------------------------

void Ccluster_plots::make_cluster_t_orders(){
	_t_orders.clear(); //replacing.


	//Cycle over the chips.
	std::vector<Cchip*>::iterator ichip;
	for (ichip = _tel->get_chips().begin();
		 ichip != _tel->get_chips().end(); ++ichip){

		if ((*ichip)->get_nclusters() == 0) continue;
		std::vector<Ccluster*> clusters = (*ichip)->get_clusters();
		int N = clusters.size();


		//Get xs and ys.
		double xs[N];
		double ys[N];
		for (int iclust = 0; iclust < N; iclust++){
			xs[iclust] = (double)iclust;
			ys[iclust] = clusters[iclust]->get_TOA();
		}


		//Construct graph.
		TGraph* g = new TGraph(N, ys, xs);
		std::string name = 	(*ichip)->get_name() + "cluster t ordering";
		g->SetTitle(name.c_str());
		_t_orders.push_back(g);

		if ((*ichip)->get_ID() == _chip_loop_cut) break;
	}
}








//-----------------------------------------------------------------------------

void Ccluster_plots::save_figs(){
	TFile* save_file = new TFile(_save_file_name.c_str(),"update");


	std::string temp_name = "Cluster_plots/";
	std::string name;
	save_file->cd();


	//Single plots first (in instance directory).
	save_file->cd("Cluster_plots");

	if (_z_distributions_initialized) {
		_z_distribution->Write("", TObject::kOverwrite);
		_zVsX_distribution->Write("", TObject::kOverwrite);
		_zVsY_distribution->Write("", TObject::kOverwrite);
	}

	name = temp_name + "ToT_dists/ToT_AllSizes";
	save_file->cd(name.c_str());
	for (unsigned int iplot = 0; iplot<_ToT_dists.size(); iplot++){
		_ToT_dists[iplot]->Write("", TObject::kWriteDelete);
	}

	name = temp_name + "ToT_dists/ToT_Size1";
	save_file->cd(name.c_str());
	for (unsigned int iplot = 0; iplot<_ToT_Size1_dists.size(); iplot++){
		_ToT_Size1_dists[iplot]->Write("", TObject::kWriteDelete);
	}

	name = temp_name + "ToT_dists/ToT_Size2";
	save_file->cd(name.c_str());
	for (unsigned int iplot = 0; iplot<_ToT_Size2_dists.size(); iplot++){
		_ToT_Size2_dists[iplot]->Write("", TObject::kWriteDelete);
	}

	name = temp_name + "ToT_dists/ToT_Size3";
	save_file->cd(name.c_str());
	for (unsigned int iplot = 0; iplot<_ToT_Size3_dists.size(); iplot++){
		_ToT_Size3_dists[iplot]->Write("", TObject::kWriteDelete);
	}

	name = temp_name + "ToT_dists/ToT_Size4";
	save_file->cd(name.c_str());
	for (unsigned int iplot = 0; iplot<_ToT_Size4_dists.size(); iplot++){
		_ToT_Size4_dists[iplot]->Write("", TObject::kWriteDelete);
	}


	name = temp_name + "t_dists";
	save_file->cd(name.c_str());
	for (unsigned int iplot = 0; iplot<_t_dists.size(); iplot++){
		_t_dists[iplot]->Write("", TObject::kWriteDelete);
	}


	name = temp_name + "size_dists";
	save_file->cd(name.c_str());
	for (unsigned int iplot = 0; iplot<_size_dists.size(); iplot++){
		_size_dists[iplot]->Write("", TObject::kWriteDelete);
	}


	name = temp_name + "t_orders";
	save_file->cd(name.c_str());
	for (unsigned int iplot = 0; iplot<_t_orders.size(); iplot++){
		std::stringstream s; s<<iplot;
		std::string name = "t_order_chip_" + s.str();
		_t_orders[iplot]->Write(name.c_str(), TObject::kWriteDelete);
	}


	name = temp_name + "x_orders";
	save_file->cd(name.c_str());
	for (unsigned int iplot = 0; iplot<_x_orders.size(); iplot++){
		std::stringstream s; s<<iplot;
		std::string name = "x_order_chip_" + s.str();
		_x_orders[iplot]->Write(name.c_str(), TObject::kWriteDelete);
	}


	name = temp_name + "Global_hitmaps";
	save_file->cd(name.c_str());
	for (unsigned int iplot = 0; iplot<_hitmapsGlobal.size(); iplot++){
		_hitmapsGlobal[iplot]->Write("", TObject::kWriteDelete);
	}

	name = temp_name + "Local_hitmaps";
	save_file->cd(name.c_str());
	for (unsigned int iplot = 0; iplot<_hitmapsLocal.size(); iplot++){
		_hitmapsLocal[iplot]->Write("", TObject::kWriteDelete);
	}

	name = temp_name + "clusteringTimeWindow_sample_hitmaps";
	save_file->cd(name.c_str());
	for (unsigned int iplot = 0; iplot<_hitmapsLocal.size(); iplot++){
		_sampleHitmaps[iplot]->Write("", TObject::kWriteDelete);
	}


	name = temp_name + "Global_YDist";
	save_file->cd(name.c_str());
	for (unsigned int iplot = 0; iplot<_hitmapsGlobalYDist.size(); iplot++){
		_hitmapsGlobalYDist[iplot]->Write("", TObject::kWriteDelete);
	}


	name = temp_name + "Global_XDist";
	save_file->cd(name.c_str());
	for (unsigned int iplot = 0; iplot<_hitmapsGlobalXDist.size(); iplot++){
		_hitmapsGlobalXDist[iplot]->Write("", TObject::kWriteDelete);
	}

	name = temp_name + "cluster_samples";
	save_file->cd(name.c_str());
	for (unsigned int iplot = 0; iplot<_cluster_samples.size(); iplot++){
		_cluster_samples[iplot]->Write("", TObject::kWriteDelete);
	}

	name = temp_name + "pixDeltaTs";
	save_file->cd(name.c_str());
	for (unsigned int iplot = 0; iplot<_pixDeltaTs.size(); iplot++){
		_pixDeltaTs[iplot]->Write("", TObject::kWriteDelete);
	}

	name = temp_name + "pixDeltaTsVsCharge";
	save_file->cd(name.c_str());
	for (unsigned int iplot = 0; iplot<_pixDeltaTsVsCharge.size(); iplot++){
		_pixDeltaTsVsCharge[iplot]->Write("", TObject::kWriteDelete);
	}


	// name = temp_name + "DCorrectChis";
	// save_file->cd(name.c_str());
	// for (int iplot = 0; iplot<_DCorrectChis.size(); iplot++){
	// 	_DCorrectChis[iplot]->Write("", TObject::kWriteDelete);
	// }

	// name = temp_name + "PredictedLeftShare";
	// save_file->cd(name.c_str());
	// for (int iplot = 0; iplot<_PredictedLeftShare.size(); iplot++){
	// 	_PredictedLeftShare[iplot]->Write("", TObject::kWriteDelete);
	// }

	// name = temp_name + "PredictedRightShare";
	// save_file->cd(name.c_str());
	// for (int iplot = 0; iplot<_PredictedRightShare.size(); iplot++){
	// 	_PredictedRightShare[iplot]->Write("", TObject::kWriteDelete);
	// }

	// name = temp_name + "ActualRightShare";
	// save_file->cd(name.c_str());
	// for (int iplot = 0; iplot<_ActualRightShare.size(); iplot++){
	// 	_ActualRightShare[iplot]->Write("", TObject::kWriteDelete);
	// }

	// name = temp_name + "ActualLeftShare";
	// save_file->cd(name.c_str());
	// for (int iplot = 0; iplot<_ActualLeftShare.size(); iplot++){
	// 	_ActualLeftShare[iplot]->Write("", TObject::kWriteDelete);
	// }

	// name = temp_name + "DiffLeftShare";
	// save_file->cd(name.c_str());
	// for (int iplot = 0; iplot<_DiffLeftShare.size(); iplot++){
	// 	_DiffLeftShare[iplot]->Write("", TObject::kWriteDelete);
	// }

	// name = temp_name + "DiffRightShare";
	// save_file->cd(name.c_str());
	// for (int iplot = 0; iplot<_DiffRightShare.size(); iplot++){
	// 	_DiffRightShare[iplot]->Write("", TObject::kWriteDelete);
	// }

	// name = temp_name + "x_differences";
	// save_file->cd(name.c_str());
	// for (int iplot = 0; iplot<_x_differences.size(); iplot++){
	// 	_x_differences[iplot]->Write("", TObject::kWriteDelete);
	// }

	// name = temp_name + "y_differences";
	// save_file->cd(name.c_str());
	// for (int iplot = 0; iplot<_y_differences.size(); iplot++){
	// 	_y_differences[iplot]->Write("", TObject::kWriteDelete);
	// }


	save_file->Close();
	std::cout<<"- Saved cluster plts -\n";
}








//-----------------------------------------------------------------------------

void Ccluster_plots::set_directories(){
	TFile* save_file = new TFile(_save_file_name.c_str(),"update");

	//Make a directory to save everything in this instance.
	save_file->cd();
	std::string name = "Cluster_plots";
	TDirectory* instance_direc = save_file->mkdir(name.c_str());
	instance_direc->cd();


	//Make the subdirectories.
	instance_direc->mkdir("ToT_dists/ToT_AllSizes");
	instance_direc->mkdir("ToT_dists/ToT_Size1");
	instance_direc->mkdir("ToT_dists/ToT_Size2");
	instance_direc->mkdir("ToT_dists/ToT_Size3");
	instance_direc->mkdir("ToT_dists/ToT_Size4");
	instance_direc->mkdir("t_dists");
	instance_direc->mkdir("size_dists");
	instance_direc->mkdir("cluster_samples");
	instance_direc->mkdir("t_orders");
	instance_direc->mkdir("x_orders");
	instance_direc->mkdir("Global_hitmaps");
	instance_direc->mkdir("Global_XDist");
	instance_direc->mkdir("Global_YDist");
	instance_direc->mkdir("Local_hitmaps");
	instance_direc->mkdir("clusteringTimeWindow_sample_hitmaps");
	instance_direc->mkdir("pixDeltaTs");
	instance_direc->mkdir("pixDeltaTsVsCharge");

	// instance_direc->mkdir("DCorrectChis");
	// instance_direc->mkdir("PredictedLeftShare");
	// instance_direc->mkdir("ActualLeftShare");
	// instance_direc->mkdir("PredictedRightShare");
	// instance_direc->mkdir("ActualRightShare");

	// instance_direc->mkdir("DiffLeftShare");
	// instance_direc->mkdir("DiffRightShare");


	save_file->cd();
	save_file->Close();
}








//-----------------------------------------------------------------------------

Ccluster_plots::~Ccluster_plots(){
	for (unsigned int iplot = 0; iplot<_ToT_dists.size(); iplot++)
		if (_ToT_dists[iplot] != NULL) delete _ToT_dists[iplot];

	for (unsigned int iplot = 0; iplot<_ToT_Size1_dists.size(); iplot++)
		if (_ToT_Size1_dists[iplot] != NULL) delete _ToT_Size1_dists[iplot];

	for (unsigned int iplot = 0; iplot<_ToT_Size2_dists.size(); iplot++)
		if (_ToT_Size2_dists[iplot] != NULL) delete _ToT_Size2_dists[iplot];

	for (unsigned int iplot = 0; iplot<_ToT_Size3_dists.size(); iplot++)
		if (_ToT_Size3_dists[iplot] != NULL) delete _ToT_Size3_dists[iplot];

	for (unsigned int iplot = 0; iplot<_ToT_Size4_dists.size(); iplot++)
		if (_ToT_Size4_dists[iplot] != NULL) delete _ToT_Size4_dists[iplot];

	for (unsigned int iplot = 0; iplot<_size_dists.size(); iplot++)
		if (_size_dists[iplot] != NULL) delete _size_dists[iplot];

	if (_z_distributions_initialized) {
		if (_z_distribution != NULL) delete _z_distribution;
		if (_zVsX_distribution != NULL) delete _zVsX_distribution;
		if (_zVsY_distribution != NULL) delete _zVsY_distribution;
	}

	for (unsigned int iplot = 0; iplot<_t_dists.size(); iplot++)
		if (_t_dists[iplot] != NULL) delete _t_dists[iplot];

	for (unsigned int iplot = 0; iplot<_t_orders.size(); iplot++)
		if (_t_orders[iplot] != NULL) delete _t_orders[iplot];

	for (unsigned int iplot = 0; iplot<_x_orders.size(); iplot++)
		if (_x_orders[iplot] != NULL) delete _x_orders[iplot];

	for (unsigned int iplot = 0; iplot<_hitmapsGlobal.size(); iplot++)
		if (_hitmapsGlobal[iplot] != NULL) delete _hitmapsGlobal[iplot];

	for (unsigned int iplot = 0; iplot<_hitmapsLocal.size(); iplot++)
		if (_hitmapsLocal[iplot] != NULL) delete _hitmapsLocal[iplot];

	for (unsigned int iplot = 0; iplot<_hitmapsGlobalXDist.size(); iplot++)
		if (_hitmapsGlobalXDist[iplot] != NULL) delete _hitmapsGlobalXDist[iplot];

	for (unsigned int iplot = 0; iplot<_hitmapsGlobalYDist.size(); iplot++)
		if (_hitmapsGlobalYDist[iplot] != NULL) delete _hitmapsGlobalYDist[iplot];

	for (unsigned int iplot = 0; iplot<_sampleHitmaps.size(); iplot++)
		if (_sampleHitmaps[iplot] != NULL) delete _sampleHitmaps[iplot];

	for (unsigned int iplot = 0; iplot<_pixDeltaTsVsCharge.size(); iplot++)
		if (_pixDeltaTsVsCharge[iplot] != NULL) delete _pixDeltaTsVsCharge[iplot];

	for (unsigned int iplot = 0; iplot<_pixDeltaTs.size(); iplot++)
		if (_pixDeltaTs[iplot] != NULL) delete _pixDeltaTs[iplot];

}








//-----------------------------------------------------------------------------

