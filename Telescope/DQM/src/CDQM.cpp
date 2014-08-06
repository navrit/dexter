#include "../headers/CDQM.h"
#include <time.h> 
#include <random>
#include <algorithm>


//_____________________________________________________________________________

CDQM::CDQM(CDQM_options* ops) {
	Chandy::dash_line_break();
	std::cout<<"================ Constructing DQM ==============="<<std::endl;
	_t1 = clock();
	_ops = ops;
	make_new_save_file();
	_ops->save_ops();

	_tel = NULL;
	_sampleNum = 0;



	// Construct algorithms.
	for (std::vector<int>::iterator it = _ops->algorithms.begin() ; it != _ops->algorithms.end(); ++it) {

		if ((*it) == 0) _old_tel_getter = new Cold_tel_getter(_ops);
		if ((*it) == 14) _PS_tel_getter = new CPS_tel_getter(_ops);

		if ((*it) == 1) _pixel_plots = new Cpixel_plots(_ops);
		if ((*it) == 2) _x_pix_correlations = new Ccorrelation_plots(_ops, "pixs", 0);
		if ((*it) == 3) _y_pix_correlations = new Ccorrelation_plots(_ops, "pixs", 1);
		if ((*it) == 4) _t_pix_correlations = new Ccorrelation_plots(_ops, "pixs", 3);

		if ((*it) == 5) _cluster_maker = new Ccluster_maker(_ops);
		if ((*it) == 6) _cluster_plots = new Ccluster_plots(_ops);
		if ((*it) == 7) _x_clust_correlations = new Ccorrelation_plots(_ops, "clusts", 0);
		if ((*it) == 8) _y_clust_correlations = new Ccorrelation_plots(_ops, "clusts", 1);
		if ((*it) == 9) _t_clust_correlations = new Ccorrelation_plots(_ops, "clusts", 3);

		if ((*it) == 10) _chip_plots = new Cchip_plots(_ops);

		if ((*it) == 11) _track_maker = new Ctrack_maker(_ops);

		if ((*it) == 12) _track_plots = new Ctrack_plots(_ops);
		if ((*it) == 13) _cog_fitter = new CCOG_fitter(_ops);

	}

	Chandy::dash_line_break();
} 



//_________________________________________________________________1____________

void CDQM::initialize(){
	std::cout<<"\n================ Initializing DQM ==============="<<std::endl;
	// Initialize algorithms.
	for (std::vector<int>::iterator it = _ops->algorithms.begin() ; it != _ops->algorithms.end(); ++it) {
		//std::cout<<"Algorithm number:\t"<<(*it)<<std::endl;
		if ((*it) == 0) _old_tel_getter->initialize();
		if ((*it) == 14) _PS_tel_getter->initialize();

		if ((*it) == 1) _pixel_plots->initialize();
		if ((*it) == 2) _x_pix_correlations->initialize();
		if ((*it) == 3) _y_pix_correlations->initialize();
		if ((*it) == 4) _t_pix_correlations->initialize();

		if ((*it) == 5) _cluster_maker->initialize();
		if ((*it) == 6) _cluster_plots->initialize();
		if ((*it) == 7) _x_clust_correlations->initialize();
		if ((*it) == 8) _y_clust_correlations->initialize();
		if ((*it) == 9) _t_clust_correlations->initialize();

		// if ((*it) == 10) _chip_plots->initialize();

		if ((*it) == 11) _track_maker->initialize();
		if ((*it) == 12) _track_plots->initialize();
		if ((*it) == 13) _cog_fitter->initialize();
		if ((*it) == 15) _prepareLongScalePlots();
	}

	Chandy::dash_line_break();
}




//_____________________________________________________________________________
void CDQM::executeEventLoop(double hr_time){
	unsigned int i;
	for (i=0; i<_ops->nEvents; i++) {
		std::cout<<"*******************************************************"<<std::endl;
		std::cout<<"************** Executing event number: "<<i<<" **************"<<std::endl;
		std::cout<<"*******************************************************"<<std::endl;
		execute(hr_time);

		if (_tel->isLastChunk) break;
	}
}


//_____________________________________________________________________________

void CDQM::execute(double hr_time){
	std::cout<<"\n================ Executing DQM ==============="<<std::endl;
	_sampleNum++;
	// Execute algorithms.
	if (_tel != NULL) delete _tel;
	_tel = new Ctel_chunk(_ops);
	bool loaded_alignment = false;
	for (std::vector<int>::iterator it = _ops->algorithms.begin() ; it != _ops->algorithms.end(); ++it) {

		if ((*it) == 0) _old_tel_getter->execute(_tel);
		if ((*it) == 14) _PS_tel_getter->execute(_tel);
		if (_ops->load_previous_alignment && !loaded_alignment) {
			_tel->load_alignments(_ops->alignment_save_file_name);
			loaded_alignment = true;
		}
		

		if ((*it) == 1) _pixel_plots->execute(_tel);
		if ((*it) == 2) _x_pix_correlations->execute(_tel);
		if ((*it) == 3) _y_pix_correlations->execute(_tel);
		if ((*it) == 4) _t_pix_correlations->execute(_tel);



		if ((*it) == 5) _cluster_maker->execute(_tel);
		if ((*it) == 6) _cluster_plots->execute(_tel);
		
		if ((*it) == 7) _x_clust_correlations->execute(_tel);
		if ((*it) == 8) _y_clust_correlations->execute(_tel);
		if ((*it) == 9) _t_clust_correlations->execute(_tel);


		// if ((*it) == 10) _chip_plots->execute(_tel);
		if ((*it) == 11) _track_maker->execute(_tel);
		if ((*it) == 12) _track_plots->execute(_tel);
		if ((*it) == 13) _cog_fitter->execute(_tel);
		if ((*it) == 15) _appendLongScalePlots(hr_time);


	}

	Chandy::dash_line_break();
}





//_____________________________________________________________________________

void CDQM::finalize(){
	std::cout<<"\n================ Finalizing DQM ==============="<<std::endl;
	bool pix_alignment_done = false;
	bool clust_alignment_done = false;

	for (std::vector<int>::iterator it = _ops->algorithms.begin() ; it != _ops->algorithms.end(); ++it) {
		if ((*it) == 0) _old_tel_getter->finalize();
		if ((*it) == 14) _PS_tel_getter->finalize();

		if ((*it) == 1) _pixel_plots->finalize();
		if ((*it) == 2) _x_pix_correlations->finalize();
		if ((*it) == 3) _y_pix_correlations->finalize();
		if ((*it) == 4) _t_pix_correlations->finalize();

		if (_ops->align_by_pix_differences && !pix_alignment_done) {
			pix_alignment_done = true;
			if (_ops->algorithms_contains(2)) _x_pix_correlations->align_by_pix_differences();
			if (_ops->algorithms_contains(3)) _y_pix_correlations->align_by_pix_differences();
			if (_ops->algorithms_contains(4)) _t_pix_correlations->align_by_pix_differences();
			_tel->save_alignments(_ops->alignment_save_file_name);
		}

		if ((*it) == 5) _cluster_maker->finalize();

		if ((*it) == 6) _cluster_plots->finalize();
		if ((*it) == 7) _x_clust_correlations->finalize();

		if ((*it) == 8) _y_clust_correlations->finalize();
		if ((*it) == 9) _t_clust_correlations->finalize();

		if (_ops->align_by_clust_differences && !clust_alignment_done) {
			clust_alignment_done = true;
			if (_ops->algorithms_contains(7)) _x_clust_correlations->align_by_clust_differences();
			if (_ops->algorithms_contains(8)) _y_clust_correlations->align_by_clust_differences();
			if (_ops->algorithms_contains(9)) _t_clust_correlations->align_by_clust_differences();
			_tel->save_alignments(_ops->alignment_save_file_name);
		}

		if ((*it) == 10) _chip_plots->finalize();

		if ((*it) == 11) _track_maker->finalize();
		if ((*it) == 12) _track_plots->finalize();
		if ((*it) == 13) _cog_fitter->finalize();
	}


	//_tel->dump_chips("ChipData.root", false);
	Chandy::dash_line_break();
	//delete _tel;
}



//_____________________________________________________________________________

void CDQM::make_new_save_file(){
	TFile* save_file = new TFile(_ops->save_file_name.c_str(),"recreate");
	save_file->Close();
}



//_____________________________________________________________________________

void CDQM::_prepareLongScalePlots() {
	if (_ops->replaceLongScalesFile) {
		std::cout<<"Recreating LongScaleDQM File."<<std::endl;
		TFile* save_file = new TFile(_ops->save_file_nameLongScale.c_str(),"RECREATE");
		std::string titleX = "xResiduals_vs_TestbeamSampleTime; time (/hours); delta_x (/mm)";
		std::string titleY = "yResiduals_vs_TestbeamSampleTime; time (/hours); delta_y (/mm)";
		std::string titleT = "tResiduals_vs_TestbeamSampleTime; time (/hours); delta_t (/DAQ time units);";
		std::string name = "Chip_";

		TDirectory* xDirec = save_file->mkdir("xResidualsVsTime");
		TDirectory* yDirec = save_file->mkdir("yResidualsVsTime");
		TDirectory* tDirec = save_file->mkdir("tResidualsVsTime");

		for (int i=0; i<_ops->nchips; i++) {
			std::stringstream ss;
			ss<<i;
			xDirec->cd();
			TGraph * hx = new TGraph(0);
			hx->SetName((name+ss.str()).c_str());
			hx->SetTitle((ss.str()+titleX).c_str());
			hx->Write();

		}


		for (int i=0; i<_ops->nchips; i++) {
			std::stringstream ss;
			ss<<i;
			yDirec->cd();
			TGraph * hx = new TGraph(0);
			hx->SetName((name+ss.str()).c_str());
			hx->SetTitle((ss.str()+titleY).c_str());
			hx->Write();

		}


		for (int i=0; i<_ops->nchips; i++) {
			std::stringstream ss;
			ss<<i;
			tDirec->cd();
			TGraph * hx = new TGraph(0);
			hx->SetName((name+ss.str()).c_str());
			hx->SetTitle((ss.str()+titleT).c_str());
			hx->Write();
		}

		save_file->Close();
	}
	else std::cout<<"Not recreating LongScaleDQM File."<<std::endl;

}



//_____________________________________________________________________________

void CDQM::_appendLongScalePlots(double hr_time) {
	std::cout<<"_appendLongScalePlots"<<std::endl;
	std::string xDirecName = "xResidualsVsTime";
	std::string yDirecName = "yResidualsVsTime";
	std::string tDirecName = "tResidualsVsTime";
	//std::default_random_engine generator;
    //std::normal_distribution<double> distribution(0.0, 0.5);

	TFile* save_file = new TFile(_ops->save_file_nameLongScale.c_str(), "UPDATE");
	for (unsigned int i=0; i<_ops->nchips; i++) {
		//generator.seed(_sampleNum);
		//double rx = distribution(generator);
		//double rx = distribution(generator);
		//double ry = distribution(generator);
		double rx = 0;
		double ry = 0; 
		double rt = 0;


		std::stringstream ss; 
		ss<<i;
		std::string direc = xDirecName + "/Chip_" + ss.str();

		TGraph * gx = (TGraph*) save_file->Get(direc.c_str());
		//gx->SetPoint(gx->GetN(), hr_time, _track_plots->_xresiduals[i]->GetMean());
		gx->SetPoint(gx->GetN(), hr_time, rx);
		save_file->cd(xDirecName.c_str());
		gx->Write("", TObject::kOverwrite);

		direc = yDirecName + "/Chip_" + ss.str();
		gx = (TGraph*) save_file->Get(direc.c_str());
		//gx->SetPoint(gx->GetN(), hr_time, _track_plots->_yresiduals[i]->GetMean());
		gx->SetPoint(gx->GetN(), hr_time, ry);
		save_file->cd(yDirecName.c_str());
		gx->Write("", TObject::kOverwrite);

		direc = tDirecName + "/Chip_" + ss.str();
		gx = (TGraph*) save_file->Get(direc.c_str());
		//gx->SetPoint(gx->GetN(), hr_time, _track_plots->_tresiduals[i]->GetMean());
		gx->SetPoint(gx->GetN(), hr_time, rt);
		save_file->cd(tDirecName.c_str());
		gx->Write("", TObject::kOverwrite);

		delete gx;
	}
	save_file->Close();
}



//_____________________________________________________________________________

//CDQM::~CDQM(){
//	std::cout<<"\n================ Destructing DQM ==============="<<std::endl;
//	std::cout<<__LINE__<<std::endl;
//	delete _old_tel_getter;
//	delete _PS_tel_getter;
//	std::cout<<__LINE__<<std::endl;
//	delete _pixel_plots;
//	delete _x_pix_correlations;
//	delete _y_pix_correlations;
//	delete _t_pix_correlations;
//	std::cout<<__LINE__<<std::endl;
//	delete _cluster_maker;
//	std::cout<<__LINE__<<std::endl;
//	delete _cluster_plots;
//	std::cout<<__LINE__<<std::endl;
//	delete _x_clust_correlations;
//	std::cout<<__LINE__<<std::endl;
//	delete _y_clust_correlations;
//	std::cout<<__LINE__<<std::endl;
//	if (_t_clust_correlations != NULL) delete _t_clust_correlations;
//	std::cout<<__LINE__<<std::endl;
//	// if ((*it) == 10) _chip_plots;
//
//	delete _track_maker;
//	std::cout<<__LINE__<<std::endl;
//	delete _track_plots;
//	std::cout<<__LINE__<<std::endl;
//	delete _cog_fitter;
//}



//_____________________________________________________________________________

