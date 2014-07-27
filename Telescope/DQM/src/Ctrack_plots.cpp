#include "../headers/Ctrack_plots.h"
#include <sstream>


//-----------------------------------------------------------------------------

 Ctrack_plots::Ctrack_plots(CDQM_options * ops){
	std::cout<<"Constructor of Ctrack_plots"<<std::endl;
	_ops = ops;

	_ADC_low = 0.0;
	_ADC_high = 20000.0;

	_tcut = 0.3;
}








//-----------------------------------------------------------------------------

void Ctrack_plots::initialize(){
	_save_file_name = _ops->save_file_name;
	_ref_chips = _ops->ref_chips;

	_nhitmap_bins = 256;
	_tdist_nbins = 100;

	_delr = 0.07;
	_delt = 200;
	_t_dist = NULL;

	//_ADC_low = 0.0;
	//_ADC_high = 200;


	init_hitmaps();
	init_residuals();
	init_track_dists();
}







//-----------------------------------------------------------------------------

void Ctrack_plots::finalize(){
	//show_residuals();

	set_directories();
	save_figs();
}



//-----------------------------------------------------------------------------

void Ctrack_plots::show_residuals(){
	float r, delr;
	std::cout<<"ChipID\t\txResolutionByRMS(um)\t\tyResolutionByRMS(um)"<<std::endl;
	for (int i=0; i<_xresiduals.size(); i++) {
		std::cout<<i<<"\t\t\t"<<1000*_xresiduals1[i]->GetRMS()<<"\t\t"<<1000*_yresiduals[i]->GetRMS()<<std::endl;
	}
	
	std::cout<<"\ncf ChipID\t\txResolutionByGaus(um)\t\tErrxResolutionByGaus(um)"<<std::endl;
	for (int i=0; i<_xresiduals.size(); i++) {
		get_GaussResidual(0, i, r, delr);
		std::cout<<i<<"\t\t\t"<<1000*r<<"\t\t"<<1000*delr<<std::endl;
	}

	std::cout<<"\ncf ChipID\t\tyResolutionByGaus(um)\t\tErryResolutionByGaus(um)"<<std::endl;
	for (int i=0; i<_xresiduals.size(); i++) {
		get_GaussResidual(1, i, r, delr);
		std::cout<<i<<"\t\t\t"<<1000*r<<"\t\t"<<1000*delr<<std::endl;
	}
}




//-----------------------------------------------------------------------------

void Ctrack_plots::get_GaussResidual(int dir, int ichip, float & r, float & delr){
	TF1 * fit;

	if (dir == 0) {
		_xresiduals[ichip]->Fit("gaus", "Q");
		fit = _xresiduals[ichip]->GetFunction("gaus");
	}

	else {
		_yresiduals[ichip]->Fit("gaus", "Q");
		fit = _yresiduals[ichip]->GetFunction("gaus");
	}

	r = (float)fit->GetParameter(2);
	delr = (float)fit->GetParError(2);

	delete fit;
}





//-----------------------------------------------------------------------------

void Ctrack_plots::execute(Ctel_chunk * tel){
	//Should have one telescope per use of this function (so _tel should be 
	//replaced).
	_tel = tel;
	//Appendable plots.
	appto_track_dists();
	appto_TrackCluster_plots();
	//for (int i=0; i<_ops->nchips; i++) appto_residuals(i);
	appto_residuals(0);

	//Replaces plots.
	//make_t_dist();
}








//-----------------------------------------------------------------------------

void Ctrack_plots::init_track_dists(){
	_mx_dist = new TH1F("mx_dist", "mx_Dist; mx; N", 100, -0.05, 0.05);
	_my_dist = new TH1F("my_dist", "my_Dist; my; N", 100, -0.05, 0.05);
	_cx_dist = new TH1F("cx_dist", "cx_Dist; cx (/mm); N", 100, -1, 15);
	_cy_dist = new TH1F("cy_dist", "cy_Dist; cy (/mm); N", 100, -1, 15);
	_size_dist = new TH1F("size_dist", "size_dist; size (int); N", _ops->nchips + 2, 0, _ops->nchips + 2);
}






//-----------------------------------------------------------------------------

void Ctrack_plots::init_residuals(){
	float d = 0.2;
	for (int i=0; i<_ops->nchips; i++){
		
		std::stringstream ss; 
		ss<<i;
		std::string xtitle = "xResidual_Dist" + ss.str() + "; delta x(/mm); N";
		std::string ytitle = "yResidual_Dist" + ss.str() + "; delta y(/mm); N";
		std::string ttitle = "tResidual_Dist" + ss.str() + "; delta t(/DAQ t units); N";
		std::string name = "Chip_" + ss.str(); 	

		_tresiduals.push_back(new TH1F(name.c_str(), ttitle.c_str(), _ops->track_resolution_nbins, -30, 30));
		_xresiduals.push_back(new TH1F(name.c_str(), xtitle.c_str(), _ops->track_resolution_nbins, -d, d));
		_yresiduals.push_back(new TH1F(name.c_str(), ytitle.c_str(), _ops->track_resolution_nbins, -d, d));
		_xresiduals0.push_back(new TH1F(name.c_str(), xtitle.c_str(), 100, -10, 10));
		_xresiduals1.push_back(new TH1F(name.c_str(), xtitle.c_str(), 100, -10, 10));
	}
}






//-----------------------------------------------------------------------------

void Ctrack_plots::init_hitmaps(){
	//According to z=0 (ideally reference chip[0]);
	_track_hitmap = new TH2F("track_hitmap", "track_hitmap",
		_nhitmap_bins, 0.0, 15, _nhitmap_bins, 0.0, 15);
	for (unsigned int i=0; i<_ops->nchips; i++) {
		std::stringstream ss;
		ss<<i;
		std::string title = "TrackedClustersGlobalHitmap" + ss.str() + "; x(/mm); y(/mm)";
		std::string name = "Chip_" + ss.str();
		_trackedClusters.push_back(new TH2F(name.c_str(), title.c_str(), 256, 
			0.0, 14.08, 256, 0.0, 14.08));
		title = "NonTrackedClustersGlobalHitmap" + ss.str() + "; x(/mm); y(/mm)";
		_nonTrackedClusters.push_back(new TH2F(name.c_str(), title.c_str(), 256, 
			0.0, 14.08, 256, 0.0, 14.08));
	}
}






//-----------------------------------------------------------------------------

void Ctrack_plots::appto_track_dists(){
	//Cycle over the tracks.
	std::vector<Ctrack*>::iterator it;
	for (it = _tel->get_tracks().begin(); it != _tel->get_tracks().end(); ++it){

		_track_hitmap->Fill((*it)->get_cx(), (*it)->get_cy());
		_mx_dist->Fill((*it)->get_mx());
		_my_dist->Fill((*it)->get_my());

		_cx_dist->Fill((*it)->get_cx());
		_cy_dist->Fill((*it)->get_cy());

		_size_dist->Fill((float)(*it)->get_size());
	}
}


//-----------------------------------------------------------------------------

void Ctrack_plots::appto_TrackCluster_plots(){
	for (unsigned int i=0; i<_tel->get_nchips(); i++) {
		std::vector<Ccluster*>::iterator iclust;
		for (iclust = _tel->get_chip(i)->get_clusters().begin(); 
			 iclust != _tel->get_chip(i)->get_clusters().end(); iclust++) {
			if ((*iclust)->get_tracked()) _trackedClusters[i]->Fill((*iclust)->get_gx(), (*iclust)->get_gy());
			else _nonTrackedClusters[i]->Fill((*iclust)->get_gx(), (*iclust)->get_gy());
		}
	}
}




//-----------------------------------------------------------------------------

void Ctrack_plots::appto_residuals(int ichip){
	double xresidual, yresidual, tresidual;
	std::vector<Ctrack*>::iterator it;
	for (it = _tel->get_tracks().begin(); it != _tel->get_tracks().end(); ++it){
		std::vector<Ccluster*> clusters = (*it)->get_clusters();
		std::vector<Ccluster*>::iterator ic;
		for (ic = clusters.begin(); ic != clusters.end(); ic++) {
			xresidual = (*it)->gx((*ic)->get_gz()) - (*ic)->get_gx();
			yresidual = (*it)->gy((*ic)->get_gz()) - (*ic)->get_gy();
			tresidual = (*it)->get_gTOA() - (*ic)->get_gt();

			int ichip = (*ic)->get_chipID();
			_xresiduals[ichip]->Fill(xresidual);
			_yresiduals[ichip]->Fill(yresidual);
			_tresiduals[ichip]->Fill(tresidual);
		}
	}


//	//Cycle over the tracks.
//	if (_tel->get_chip(ichip)->get_nclusters() != 0) {
//		std::vector<Ccluster*> clusters = _tel->get_chip(ichip)->get_clusters();
//		std::vector<Ctrack*>::iterator it;
//		for (it = _tel->get_tracks().begin(); it != _tel->get_tracks().end(); ++it){
//
//			// Refit track.
//			std::vector<int> excludes;
//			excludes.push_back(ichip);
//			(*it)->set_by_clusters(excludes, true, (*it)->get_ID());
//
//
//			// Find nearby clusters.
//			std::vector<float> xresiduals;
//			std::vector<float> yresiduals;
//			std::vector<float> tresiduals;
//
//
//			float zchip = _tel->get_chip(ichip)->get_gz();
//			//float xtrack = (*it)->gx(zchip);
//
//			int iclust = _tel->get_chip(ichip)->glob_t_to_clustID((*it)->get_gt() - _delt);
//			//int iclust = 0;
//			for (; iclust < clusters.size(); iclust++){
//				float xresidual = (*it)->gx(clusters[iclust]->get_gz()) - clusters[iclust]->get_gx();
//				float yresidual = (*it)->gy(clusters[iclust]->get_gz()) - clusters[iclust]->get_gy();
//				float tresidual = (*it)->get_gTOA() - clusters[iclust]->get_gt();
//
//				if (fabs(xresidual) < _delr && fabs(yresidual) < _delr && fabs(tresidual) < _delt) {
//					_xresiduals[ichip]->Fill(xresidual);
//					_yresiduals[ichip]->Fill(yresidual);
//					_tresiduals[ichip]->Fill(tresidual);
//				}
//
//				if (clusters[iclust]->get_gt() > (*it)->get_gt() + _delt) break;
//			}
//
//			// Undo excluding fit.
//			std::vector<int> non_excludes;
//			(*it)->set_by_clusters(non_excludes, false, (*it)->get_ID());
//		}
//	}
}





//-----------------------------------------------------------------------------

void Ctrack_plots::make_t_dist(){
	//This is a replacement.
	if (_t_dist!=NULL) delete _t_dist;

	_t_dist = new TH1F("track_t_dist", "track_t_dist", _tel->get_tracks()[0]->get_gt(), 
		_tel->get_tracks().back()->get_gt(), _tdist_nbins);

	std::vector<Ctrack*>::iterator it;
	for (it = _tel->get_tracks().begin(); it != _tel->get_tracks().end(); ++it){
		_t_dist->Fill((*it)->get_gt());
	}

}





//-----------------------------------------------------------------------------

void Ctrack_plots::save_figs(){
	TFile* save_file = new TFile(_save_file_name.c_str(),"update");
	std::string temp_name = "Track_plots/";
	save_file->cd(temp_name.c_str());

	if (_track_hitmap!=NULL) _track_hitmap->Write("", TObject::kWriteDelete);
	if (_t_dist!=NULL) _t_dist->Write("", TObject::kWriteDelete);
	if (_mx_dist!=NULL) _mx_dist->Write("", TObject::kWriteDelete);
	if (_my_dist!=NULL) _my_dist->Write("", TObject::kWriteDelete);
	if (_cx_dist!=NULL) _cx_dist->Write("", TObject::kWriteDelete);
	if (_cy_dist!=NULL) _cy_dist->Write("", TObject::kWriteDelete);
	if (_size_dist!=NULL) _size_dist->Write("", TObject::kWriteDelete);

	std::string name;
	name = temp_name + "xResiduals";
	save_file->cd(name.c_str());
	for (int iplot = 0; iplot<_xresiduals.size(); iplot++){
		_xresiduals[iplot]->Write("", TObject::kOverwrite);
	}


	name = temp_name + "xResiduals0";
	save_file->cd(name.c_str());
	for (int iplot = 0; iplot<_xresiduals0.size(); iplot++){
		_xresiduals0[iplot]->Write("", TObject::kOverwrite);
	}


	name = temp_name + "xResiduals1";
	save_file->cd(name.c_str());
	for (int iplot = 0; iplot<_xresiduals1.size(); iplot++){
		_xresiduals1[iplot]->Write("", TObject::kOverwrite);
	}


	name = temp_name + "yResiduals";
	save_file->cd(name.c_str());
	for (int iplot = 0; iplot<_yresiduals.size(); iplot++){
		_yresiduals[iplot]->Write("", TObject::kOverwrite);
	}

	name = temp_name + "tResiduals";
	save_file->cd(name.c_str());
	for (int iplot = 0; iplot<_tresiduals.size(); iplot++){
		_tresiduals[iplot]->Write("", TObject::kOverwrite);
	}

	name = temp_name + "TrackedClusters";
	save_file->cd(name.c_str());
	for (int iplot = 0; iplot<_trackedClusters.size(); iplot++){
		_trackedClusters[iplot]->Write("", TObject::kOverwrite);
	}

	name = temp_name + "NonTrackedClusters";
	save_file->cd(name.c_str());
	for (int iplot = 0; iplot<_nonTrackedClusters.size(); iplot++){
		_nonTrackedClusters[iplot]->Write("", TObject::kOverwrite);
	}

	save_file->Close();
	std::cout<<"- Saved track plts -\n";
}








//-----------------------------------------------------------------------------

void Ctrack_plots::set_directories(){
	TFile* save_file = new TFile(_save_file_name.c_str(),"update");

	//Make a directory to save everything in this instance.
	save_file->cd();
	std::string name = "Track_plots";
	TDirectory* instance_direc = save_file->mkdir(name.c_str());
	instance_direc->cd();


	instance_direc->mkdir("xResiduals");
	instance_direc->mkdir("xResiduals0");
	instance_direc->mkdir("xResiduals1");
	instance_direc->mkdir("yResiduals");
	instance_direc->mkdir("tResiduals");
	instance_direc->mkdir("TrackedClusters");
	instance_direc->mkdir("NonTrackedClusters");

	save_file->cd();
	save_file->Close();
}








//-----------------------------------------------------------------------------

Ctrack_plots::~Ctrack_plots(){
//	for (int i=0; i<_xresiduals.size(); i++){
//		delete _xresiduals[i];
//		delete _xresiduals0[i];
//		delete _xresiduals1[i];
//		delete _yresiduals[i];
//		delete _tresiduals[i];
//		_trackedClusters[i];
//		_nonTrackedClusters[i];
//	}
//
//	delete _track_hitmap;
//	delete _t_dist;
//	delete _size_dist;
//	delete _mx_dist;
//	delete _my_dist;
//	delete _cx_dist;
//	delete _cy_dist;
}








//-----------------------------------------------------------------------------

