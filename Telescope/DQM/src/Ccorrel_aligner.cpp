#include "../headers/Ccorrel_aligner.h"


//Author: Dan Saunders
//Date created: 27/11/13
//Date created: 27/11/13




//-----------------------------------------------------------------------------

Ccorrel_aligner::Ccorrel_aligner(Ctel_chunk * tel, std::string save_file_name, 
	Ccorrelation_plots * x_correlation_plots, Ccorrelation_plots * y_correlation_plots,
	int ref_chip, int chip_loop_cut, int n_proj_bins){
	Chandy::dash_line_break();
	std::cout<<"Constructor of Ccorrel_aligner."<<std::endl;

	_tel = tel;
	_ref_chip = ref_chip;
	_chip_loop_cut = chip_loop_cut;
	_save_file_name = save_file_name;
	_x_correlation_plots = x_correlation_plots;
	_y_correlation_plots = y_correlation_plots;
	_n_proj_bins = n_proj_bins;
}








//-----------------------------------------------------------------------------

void Ccorrel_aligner::align_all(){
	// align_direction(0); //xs.
	// align_direction(1); //ys. Note order performed should be kept consistent.
	align_ts();
}








//-----------------------------------------------------------------------------

// void Ccorrel_aligner::align_ts(){
// 	//Fills the t_diffs. Cycle over chips.
// 	for (int ichip=0; ichip<_correls[0].size(); ichip++){
// 		find_line(ichip, 0);
// 		find_line(ichip, 1);



// 		//Cycle over both sets of pixels indexs.
// 		std::vector<Cpixel*> pixs = *(_tel->get_chip(ichip)->get_pixels());
// 		std::vector<Cpixel*> refpixs = *(_tel->get_chip(_ref_chip)->get_pixels());
// 		for (int ipix=0; ipix < pixs.size(); ipix++){
// 			if (pixs[ipix]->get_onoff() == false) continue;
// 			else {


// 				//Start the loop over pixels two at the farthest index back for 
// 				//the t window - saves time. Weary of the one thingy. Approx.
// 				int irpix_start = _tel->get_chip(_ref_chip)->t_to_pixID(pixs[ipix]->get_TOA());
// 				for (int irpix=irpix_start; irpix<refpixs.size(); irpix++){


// 					//Ask if too far ahead in time.
// 					if (unsuitable_t(pixs[ipix], refpixs[irpix])) break;
// 					if (refpixs[irpix]->get_onoff() == false) continue;
// 					else{


// 						//At this stage, both pixels are switched on, and 
// 						//near the tbounds. Send to another function to check
// 						//bound and fill.
// 						ts_fill(ichip, pixs[ipix], refpixs[irpix]);
// 					}
// 				}
// 			}
// 		}
// 	}
// }








// //-----------------------------------------------------------------------------

// void Ccorrel_aligner::ts_fill(int ichip, Cpixel* pix, Cpixel* refpix){
// 	float pix_x = (float) pix->get_column();
// 	float refpix_x_pred = _lines[0][ichip]->y(pix_x); //zero index since want x

// 	//std::cout<<_lines[0][ichip]->get_c_shift()<<"\t\t"<<pix_x<<"\t"<<refpix_x_pred<<"\t"<<refpix->get_column();

// 	//Ask if near the correlation lines.
// 	if (fabs(refpix_x_pred - refpix->get_column()) < _correl_bound){
// 		//std::cout<<"\tYay!";

// 		//Do same in y.
// 		float refpix_y_pred = _lines[1][ichip]->y((float) pix->get_row());
// 		if (fabs(refpix_y_pred - refpix->get_row()) < _correl_bound){
// 			_t_diffs[ichip]->Fill(pix->get_TOA() - refpix->get_TOA());
// 		}
// 	}
// 	//std::cout<<"\n";
// }








//-----------------------------------------------------------------------------

// bool Ccorrel_aligner::unsuitable_t(Cpixel* pix1, Cpixel* pix2){
// 	//Checks whether pixel2 is too far ahead of pixel1 in time - useful,
// 	//as breaks the second pixel loop, speeding up massively.
// 	if ((pix2->get_TOA() - pix1->get_TOA()) > _TOA_bound) return true;
// 	return false;
// }








//-----------------------------------------------------------------------------

void Ccorrel_aligner::align_direction(int dir){
	//Make a directory to save plots. Note, these plots do not need to be
	//appendable (i.e. only makes sense to do these once, then replace).

	std::string direc_name = Chandy::get_str_dir(dir) + "_correlation_aligner";
	TFile* save_file = new TFile(_save_file_name.c_str(),"update");
	save_file->rmdir(direc_name.c_str());
	TDirectory* instance_direc = save_file->mkdir(direc_name.c_str());
	instance_direc->cd();

	//Find the correlation line for each chip.
	for (int ichip=0; ichip<_x_correlation_plots->get_correlations().size(); ichip++){
		std::stringstream ss;
		ss<<ichip;

		//Make a directory for the plots for this chip.
		TDirectory* chip_direc = instance_direc->mkdir(("Chip_" + ss.str()).c_str());
		find_line(ichip, dir, chip_direc);


		if (ichip == _chip_loop_cut) break;
	}

	save_file->Close();
}








//-----------------------------------------------------------------------------

void Ccorrel_aligner::find_line(int ichip, int dir, TDirectory* chip_direc){
	//Get the appropriate correlation plot and pass to Ccorrel_line_finder.
	TH2F * h;
	if (dir == 0) h = _x_correlation_plots->get_correlations_backgroundless()[ichip];
	else if (dir == 1) h = _y_correlation_plots->get_correlations_backgroundless()[ichip];
	Ccorrel_line_finder line_finder(h, 0.74, 0.84, _n_proj_bins, true, chip_direc);

	line_finder.varying_projections();
	line_finder.save_figs();


}








//-----------------------------------------------------------------------------

void Ccorrel_aligner::clean_directories(){
	//Deletes the directories where plots are saved (to be replaced).
	TFile* save_file = new TFile(_save_file_name.c_str(),"update");

	std::string direc_name = "x_correlation_aligner";
	save_file->rmdir(direc_name.c_str());

	direc_name = "y_correlation_aligner";
	save_file->rmdir(direc_name.c_str());

	save_file->Close();
}








//-----------------------------------------------------------------------------

void Ccorrel_aligner::align_ts(){
	//for (int ichip=0; ichip<_x_correlation_plots->get_correlations().size(); ichip++){
		
}








//-----------------------------------------------------------------------------
