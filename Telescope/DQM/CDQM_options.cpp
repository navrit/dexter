#include "CDQM_options.h"



//_____________________________________________________________________________

CDQM_options::CDQM_options(int r){
	tEventStep = 300e12;
	nEvents = 1;
	nPixHitCut = 150000;
	eventTrackN = 1200;
	PSNumFix = 1;
	runNumber = r;
	std::stringstream ssRunNumber;
	ssRunNumber<<runNumber;
	save_file_name = "DQM_files/DQM_OnlinePlots_PSBulkRun" + ssRunNumber.str()+ ".root";
	save_file_nameLongScale = "LongScale" + save_file_name;
	replaceLongScalesFile = true;
	alignment_save_file_name = "test_data/Alignment1274.dat";

	// int arr_algorithms[] = {0, 1, 5, 6, 7, 8, 9, 11, 12};
	//int arr_algorithms[] = {14, 1, 2, 3, 5, 6, 7, 8, 9, 10, 11, 12};
	//int arr_algorithms[] = {14, 1, 5, 6, 7, 8, 10, 11, 12};
	//int arr_algorithms[] = {14, 1, 5, 6, 7, 8, 11, 12};
	//int arr_algorithms[] = {14, 1, 5, 6, 7, 8}; // Default.
	int arr_algorithms[] = {14, 1, 5, 6, 7, 8, 11, 12}; // Basic track.
	for (int i=0; i<sizeof(arr_algorithms)/sizeof(int); i++) algorithms.push_back(arr_algorithms[i]);

	//Prior actions____________________________________________________________
	chip_cut = 10; //produce plots until reaching this cut - useful for tests.
	ref_chips.push_back(3); //used during correlation plots for comparing two chips.
	ref_chips.push_back(5); //extra reference chip for tracking.

	// Noise pixels.
//	_MaskedPixelsX.push_back(139);
//	_MaskedPixelsY.push_back(162);
//	_MaskedPixelsChipID.push_back(0);
//
//	_MaskedPixelsX.push_back(185);
//	_MaskedPixelsY.push_back(172);
//	_MaskedPixelsChipID.push_back(0);
//
	_MaskedPixelsX.push_back(79);
	_MaskedPixelsY.push_back(88);
	_MaskedPixelsChipID.push_back(3);

	_MaskedPixelsX.push_back(77);
	_MaskedPixelsY.push_back(92);
	_MaskedPixelsChipID.push_back(2);
//
//	_MaskedPixelsX.push_back(77);
//	_MaskedPixelsY.push_back(92);
//	_MaskedPixelsChipID.push_back(5);
//
//	_MaskedPixelsX.push_back(132);
//	_MaskedPixelsY.push_back(54);
//	_MaskedPixelsChipID.push_back(5);
//
//	_MaskedPixelsX.push_back(155);
//	_MaskedPixelsY.push_back(40);
//	_MaskedPixelsChipID.push_back(7);
//
//	_MaskedPixelsX.push_back(156);
//	_MaskedPixelsY.push_back(40);
//	_MaskedPixelsChipID.push_back(7);
//
//	_MaskedPixelsX.push_back(7);
//	_MaskedPixelsY.push_back(35);
//	_MaskedPixelsChipID.push_back(7);
//
//	_MaskedPixelsX.push_back(5);
//	_MaskedPixelsY.push_back(219);
//	_MaskedPixelsChipID.push_back(7);
//
//	_MaskedPixelsX.push_back(6);
//	_MaskedPixelsY.push_back(217);
//	_MaskedPixelsChipID.push_back(7);
//
//	_MaskedPixelsX.push_back(85);
//	_MaskedPixelsY.push_back(120);
//	_MaskedPixelsChipID.push_back(7);

	_MaskedPixelsX.push_back(69);
	_MaskedPixelsY.push_back(150);
	_MaskedPixelsChipID.push_back(4);



	truncate = false;
	tcut = 20;
	nchips = 8; // also set by alignment file.


	//Options to do methods prior to filling the DQM plots (so act on a single 
	//chunk of data). pixel_run specifies old data.
	remove_hot_pixels = false;
	remove_frequency = 2;
	load_previous_alignment = true;
	also_order_pixs_x = true; //V unsafe not to.


	//Pixel hits ______________________________________________________________
	//Parameters for plots made by Cpixel_plots (with on-off switch).
	pix_ToT_nbins = 100;
	pix_hitmap_nbins = 256;

	align_by_pix_differences = false;
	pix_correl_xynbins = 256;
	pix_correl_tnbins = 256;

	pix_xlow = -20; 
	pix_xup = 20;
	pix_ylow = -20;
	pix_yup = 20;
	pix_tlow = -200;
	pix_tup = 200;
	pix_tbglow = -2;
	pix_tbgup = 0;


	//Clusters ________________________________________________________________
	clust_maker_tlow = 0;
	clust_maker_tup = 100;
	COGweight = 1.3;
	DanCorrectPosn = false;
	
	clust_ToT_nbins = 100;
	clust_hitmap_nbins = 256;

	align_by_clust_differences = false;
	clust_correl_xynbins = 400;
	clust_correl_tnbins = 100;

	clust_xlow = -4;
	clust_xup = 4;
	clust_ylow = -4;
	clust_yup = 4;
	clust_tlow = -50;
	clust_tup = 50;
	clust_tbglow = -2;
	clust_tbgup = 0;


	//Tracking ________________________________________________________________
	track_delt = 80;
	track_vol_shape = 1;
	track_vol_rx = 0.5;
	track_vol_ry = 0.5;
	minNClusterPerTrack = 6;
	track_vol_thetax = 0.03;
	track_vol_thetay = 0.02;
	track_hitmap_nbins = 256;
	track_tdist_nbins = 100;
	track_resolution_nbins = 100;


	//Dumping _________________________________________________________________
	dump_tel = false;
	dump_check = false;


	//Py plots ________________________________________________________________
	include_py_plots = false;
	Pytel_drawer_tlower = 30.5;
	Pytel_drawer_tupper = 30.7;
}







//_____________________________________________________________________________
	
void CDQM_options::print_ops(){
	//Method to print out the current DQM options.
	// std::cout<<"\n\nChip options _____________________________\n"<<std::endl;
	// std::cout<<"chip_cut:\t\t"<<chip_cut<<std::endl;
	// std::cout<<"ref_chip:\t\t"<<ref_chip<<std::endl;
	// std::cout<<"pixel_run:\t\t"<<pixel_run<<std::endl;
	// std::cout<<"remove_hot_pixels:\t"<<remove_hot_pixels<<std::endl;
	// std::cout<<"poor_man_align:\t\t"<<poor_man_align<<std::endl;

	// std::cout<<"\npixel_plots:\t\t"<<pixel_plots<<std::endl;
	// std::cout<<"ADC_nbins:\t\t"<<pix_ADC_nbins<<std::endl;
	// std::cout<<"ADCdist_upper:\t\t"<<ADCdist_upper<<std::endl;
	// std::cout<<"ADCdist_lower:\t\t"<<ADCdist_lower<<std::endl;
	// std::cout<<"zdist_nbins:\t\t"<<zdist_nbins<<std::endl;
	// std::cout<<"hitmap_nbins:\t\t"<<zdist_nbins<<std::endl;

	// std::cout<<"\ncorrel_plots\t\t"<<correl_plots<<std::endl;
	// std::cout<<"correl_nbins\t\t"<<correl_nbins<<std::endl;
	// std::cout<<"xlow\t\t\t"<<pix_xlow<<std::endl;
	// std::cout<<"xup\t\t\t"<<pix_xup<<std::endl;
	// std::cout<<"ylow\t\t\t"<<pix_ylow<<std::endl;
	// std::cout<<"yup\t\t\t"<<pix_yup<<std::endl;
	// std::cout<<"tlow\t\t\t"<<pix_tlow<<std::endl;
	// std::cout<<"tup\t\t\t"<<pix_tup<<std::endl;
	// std::cout<<"tbglow\t\t\t"<<pix_tbglow<<std::endl;
	// std::cout<<"tbgup\t\t\t"<<pix_tbgup<<std::endl;

	// std::cout<<"\ncorrelation_align\t\t"<<correlation_align<<std::endl;
	// std::cout<<"n_proj_bins\t\t"<<n_proj_bins<<std::endl;
	
	// std::cout<<"End chip options _________________________\n\n"<<std::endl;
}








//_____________________________________________________________________________

void CDQM_options::save_ops(){
	int nops = 100;
	TH1F * h_ops = new TH1F("DQM_options", "DQM_options", nops, 0, nops);
	int n=1;

	h_ops->SetBinContent(h_ops->GetBin(n), nchips); n++;
	h_ops->SetBinContent(h_ops->GetBin(n), tcut); n++;
	h_ops->SetBinContent(h_ops->GetBin(n), chip_cut); n++;
	h_ops->SetBinContent(h_ops->GetBin(n), ref_chips[0]); n++;
	h_ops->SetBinContent(h_ops->GetBin(n), ref_chips[1]); n++;


	h_ops->SetBinContent(h_ops->GetBin(n), remove_hot_pixels); n++;
	h_ops->SetBinContent(h_ops->GetBin(n), remove_frequency); n++;
	h_ops->SetBinContent(h_ops->GetBin(n), load_previous_alignment); n++;

	h_ops->SetBinContent(h_ops->GetBin(n), pix_ToT_nbins); n++;
	h_ops->SetBinContent(h_ops->GetBin(n), pix_hitmap_nbins); n++;

	h_ops->SetBinContent(h_ops->GetBin(n), align_by_pix_differences); n++;
	h_ops->SetBinContent(h_ops->GetBin(n), pix_correl_xynbins); n++;
	h_ops->SetBinContent(h_ops->GetBin(n), pix_correl_tnbins); n++;

	h_ops->SetBinContent(h_ops->GetBin(n), pix_xlow); n++;
	h_ops->SetBinContent(h_ops->GetBin(n), pix_xup); n++;
	h_ops->SetBinContent(h_ops->GetBin(n), pix_ylow); n++;
	h_ops->SetBinContent(h_ops->GetBin(n), pix_yup); n++;
	h_ops->SetBinContent(h_ops->GetBin(n), pix_tlow); n++;
	h_ops->SetBinContent(h_ops->GetBin(n), pix_tup); n++;
	h_ops->SetBinContent(h_ops->GetBin(n), pix_tbglow); n++;
	h_ops->SetBinContent(h_ops->GetBin(n), pix_tbgup); n++;


	h_ops->SetBinContent(h_ops->GetBin(n), clust_maker_tlow); n++;
	h_ops->SetBinContent(h_ops->GetBin(n), clust_maker_tup); n++;

	h_ops->SetBinContent(h_ops->GetBin(n), clust_ToT_nbins); n++;
	h_ops->SetBinContent(h_ops->GetBin(n), clust_hitmap_nbins); n++;
	h_ops->SetBinContent(h_ops->GetBin(n), nsample_clusters); n++;

	h_ops->SetBinContent(h_ops->GetBin(n), align_by_clust_differences); n++;
	h_ops->SetBinContent(h_ops->GetBin(n), clust_correl_xynbins); n++;
	h_ops->SetBinContent(h_ops->GetBin(n), clust_correl_tnbins); n++;

	h_ops->SetBinContent(h_ops->GetBin(n), clust_xlow); n++;
	h_ops->SetBinContent(h_ops->GetBin(n), clust_xup); n++;
	h_ops->SetBinContent(h_ops->GetBin(n), clust_ylow); n++;
	h_ops->SetBinContent(h_ops->GetBin(n), clust_yup); n++;
	h_ops->SetBinContent(h_ops->GetBin(n), clust_tlow); n++;
	h_ops->SetBinContent(h_ops->GetBin(n), clust_tup); n++;
	h_ops->SetBinContent(h_ops->GetBin(n), clust_tbglow); n++;
	h_ops->SetBinContent(h_ops->GetBin(n), clust_tbgup); n++;

//
//	h_ops->SetBinContent(h_ops->GetBin(n), track_vol_r); n++;
//	h_ops->SetBinContent(h_ops->GetBin(n), track_vol_theta); n++;
	h_ops->SetBinContent(h_ops->GetBin(n), track_delt); n++;
	h_ops->SetBinContent(h_ops->GetBin(n), track_hitmap_nbins); n++;
	h_ops->SetBinContent(h_ops->GetBin(n), track_tdist_nbins); n++;


	h_ops->SetBinContent(h_ops->GetBin(n), dump_tel); n++;
	h_ops->SetBinContent(h_ops->GetBin(n), dump_check); n++;


	h_ops->SetBinContent(h_ops->GetBin(n), include_py_plots); n++;
	h_ops->SetBinContent(h_ops->GetBin(n), Pytel_drawer_tlower); n++;
	h_ops->SetBinContent(h_ops->GetBin(n), Pytel_drawer_tupper); n++;


	std::cout<<"Num saved options: "<<n<<std::endl;
	TFile* save_file = new TFile(save_file_name.c_str(),"update");
	h_ops->Write();
	save_file->Close();
	delete h_ops;
}








//_____________________________________________________________________________

void CDQM_options::load_ops(){
	TFile* save_file = new TFile(save_file_name.c_str());
	TH1F * h_ops = (TH1F*)save_file->Get("DQM_options");
	ref_chips.clear();

	int n=1;

	nchips = h_ops->GetBinContent(h_ops->GetBin(n)); n++;
	tcut = h_ops->GetBinContent(h_ops->GetBin(n)); n++;
	chip_cut = h_ops->GetBinContent(h_ops->GetBin(n)); n++;
	ref_chips.push_back(h_ops->GetBinContent(h_ops->GetBin(n))); n++;
	ref_chips.push_back(h_ops->GetBinContent(h_ops->GetBin(n))); n++;


	remove_hot_pixels = h_ops->GetBinContent(h_ops->GetBin(n)); n++;
	remove_frequency = h_ops->GetBinContent(h_ops->GetBin(n)); n++;
	load_previous_alignment = h_ops->GetBinContent(h_ops->GetBin(n)); n++;


	pix_ToT_nbins = h_ops->GetBinContent(h_ops->GetBin(n)); n++;
	pix_hitmap_nbins = h_ops->GetBinContent(h_ops->GetBin(n)); n++;


	align_by_pix_differences = h_ops->GetBinContent(h_ops->GetBin(n)); n++;
	pix_correl_xynbins = h_ops->GetBinContent(h_ops->GetBin(n)); n++;
	pix_correl_tnbins = h_ops->GetBinContent(h_ops->GetBin(n)); n++;


	pix_xlow = h_ops->GetBinContent(h_ops->GetBin(n)); n++;
	pix_xup = h_ops->GetBinContent(h_ops->GetBin(n)); n++;
	pix_ylow = h_ops->GetBinContent(h_ops->GetBin(n)); n++;
	pix_yup = h_ops->GetBinContent(h_ops->GetBin(n)); n++;
	pix_tlow = h_ops->GetBinContent(h_ops->GetBin(n)); n++;
	pix_tup = h_ops->GetBinContent(h_ops->GetBin(n)); n++;
	pix_tbglow = h_ops->GetBinContent(h_ops->GetBin(n)); n++;
	pix_tbgup = h_ops->GetBinContent(h_ops->GetBin(n)); n++;


	clust_maker_tlow = h_ops->GetBinContent(h_ops->GetBin(n)); n++;
	clust_maker_tup = h_ops->GetBinContent(h_ops->GetBin(n)); n++;



	clust_ToT_nbins = h_ops->GetBinContent(h_ops->GetBin(n)); n++;
	clust_hitmap_nbins = h_ops->GetBinContent(h_ops->GetBin(n)); n++;
	nsample_clusters = h_ops->GetBinContent(h_ops->GetBin(n)); n++;


	align_by_clust_differences = h_ops->GetBinContent(h_ops->GetBin(n)); n++;
	clust_correl_xynbins = h_ops->GetBinContent(h_ops->GetBin(n)); n++;
	clust_correl_tnbins = h_ops->GetBinContent(h_ops->GetBin(n)); n++;



	clust_xlow = h_ops->GetBinContent(h_ops->GetBin(n)); n++;
	clust_xup = h_ops->GetBinContent(h_ops->GetBin(n)); n++;
	clust_ylow = h_ops->GetBinContent(h_ops->GetBin(n)); n++;
	clust_yup = h_ops->GetBinContent(h_ops->GetBin(n)); n++;
	clust_tlow = h_ops->GetBinContent(h_ops->GetBin(n)); n++;
	clust_tup = h_ops->GetBinContent(h_ops->GetBin(n)); n++;
	clust_tbglow = h_ops->GetBinContent(h_ops->GetBin(n)); n++;
	clust_tbgup = h_ops->GetBinContent(h_ops->GetBin(n)); n++;
//
//
//	track_vol_r = h_ops->GetBinContent(h_ops->GetBin(n)); n++;
//	track_vol_theta = h_ops->GetBinContent(h_ops->GetBin(n)); n++;
	track_delt = h_ops->GetBinContent(h_ops->GetBin(n)); n++;
	track_hitmap_nbins = h_ops->GetBinContent(h_ops->GetBin(n)); n++;
	track_tdist_nbins = h_ops->GetBinContent(h_ops->GetBin(n)); n++;


	dump_tel = h_ops->GetBinContent(h_ops->GetBin(n)); n++;
	dump_check = h_ops->GetBinContent(h_ops->GetBin(n)); n++;


	include_py_plots = h_ops->GetBinContent(h_ops->GetBin(n)); n++;
	Pytel_drawer_tlower = h_ops->GetBinContent(h_ops->GetBin(n)); n++;
	Pytel_drawer_tupper = h_ops->GetBinContent(h_ops->GetBin(n)); n++;
}	



//_____________________________________________________________________________
bool CDQM_options::algorithms_contains(int x){
	bool result = false;
	for (std::vector<int>::iterator it = algorithms.begin();
		 it != algorithms.end(); it++){
		if ((*it) == x) result = true;
	}

	return result;
}




//_____________________________________________________________________________
bool CDQM_options::maskedPixel(int x, int y){
	bool result = false;
	for (std::vector<int>::iterator itx = _MaskedPixelsX.begin();
		 itx != _MaskedPixelsX.end(); itx++){
		if ((*itx) == x) {
			for (std::vector<int>::iterator ity = _MaskedPixelsY.begin();
		 		 ity != _MaskedPixelsY.end(); ity++){
				if ((*ity) == y) result = true;
			}
		}
	}


	bool excludeZeroZeroQuartile = false;
	if (excludeZeroZeroQuartile) {
		if (x < 64 && y<64) result = true;
	}

	return result;
}


//_____________________________________________________________________________
