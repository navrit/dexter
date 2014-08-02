#include "../../headers/data_holders/Ctel_chunk.h"


//-----------------------------------------------------------------------------

Ctel_chunk::Ctel_chunk(CDQM_options * ops){
	isLastChunk = false;
	_ops = ops;
	_tzero_set = false;
	std::cout<<"Constructor of a telescope."<<std::endl;
	_handy = new Chandy();
	_nchips = 0;
	_ntracks = 0;

	_save_file_name = "tel_save_file";
}








//-----------------------------------------------------------------------------

void Ctel_chunk::rm_all_hot_pixels(double sigma_cut){
	//cycle over chips.
	std::cout<<"\nDisabling hot pixels."<<std::endl;
	std::vector<Cchip*>::iterator ichip;
	for (ichip = _chips.begin(); ichip != _chips.end(); ++ichip){
		(*ichip)->rm_hot_pixels(sigma_cut);
	}
}







//-----------------------------------------------------------------------------

void Ctel_chunk::fill_all_hit_maps(){

	//cycle over chips.
	std::vector<Cchip*>::iterator ichip;
	for (ichip = _chips.begin(); ichip != _chips.end(); ++ichip){
		(*ichip)->fill_hit_map();
	}
}







//-----------------------------------------------------------------------------

int Ctel_chunk::chipname_to_id(std::string chipname){
	int i = 0;

	//cycle over chips.
	for (; i<get_nchips(); i++){
		if (get_chip(i)->get_name() == chipname){
			break;
		}
	}
	return i;
}







//-----------------------------------------------------------------------------

void Ctel_chunk::time_order_pixels_comb(){
	//Sorts the cluster TOA's using the comb sorting algorithm (which performs
	//well if data is already partially sorted, and quicker than bubble).
	std::cout<<"\n\nTime ordering..."<<std::endl;


	//Cycle over the chips.
	std::vector<Cchip*>::iterator ichip;
	for (ichip = _chips.begin(); ichip != _chips.end(); ++ichip){
		(*ichip)->time_order_pix_hits_comb();
	}
}







//-----------------------------------------------------------------------------

Ctel_chunk::~Ctel_chunk(){
	//Need to delete the chips and tracks.

	std::vector<Cchip*>::iterator ichip;
	for (ichip = _chips.begin(); ichip != _chips.end(); ++ichip){
		if ((*ichip)!=NULL) delete (*ichip);
	}

	//For tracks.
	 std::vector<Ctrack*>::iterator it;
	 for (it = _tracks.begin(); it != _tracks.end(); ++ichip){
	 	if ((*it)!=NULL) delete (*it);
	 }

}







//-----------------------------------------------------------------------------

int Ctel_chunk::get_total_hits_clustered(){
	//Need to delete the chips and tracks.
	int total = 0;
	std::vector<Cchip*>::iterator ichip;
	for (ichip = _chips.begin(); ichip != _chips.end(); ++ichip){

		std::vector<Ccluster*>::iterator iclust;
		for (iclust = (*ichip)->get_clusters().begin();
			 iclust != (*ichip)->get_clusters().end(); iclust++){
			total += (*iclust)->get_size();
		}
	}

	return total;

}







//-----------------------------------------------------------------------------

int Ctel_chunk::get_total_pix_hits(){
	//Need to delete the chips and tracks.
	int total = 0;
	std::vector<Cchip*>::iterator ichip;
	for (ichip = _chips.begin(); ichip != _chips.end(); ++ichip){
		total += (*ichip)->get_npix_hits();
	}

	return total;

}







//-----------------------------------------------------------------------------

void Ctel_chunk::truncate(int n){
	//Need to delete the chips and tracks.
	int total = 0;
	std::vector<Cchip*>::iterator ichip;
	for (ichip = _chips.begin(); ichip != _chips.end(); ++ichip){
		(*ichip)->truncate(n);
	}
}




//-----------------------------------------------------------------------------

void Ctel_chunk::save_alignments(std::string save_file_name){
    ofstream myfile;
    myfile.open (_ops->alignment_save_file_name.c_str());
    for (unsigned int i=0; i<_nchips; i++){
    	Cchip * c = this->get_chips()[i];
    	std::string s = c->get_name();
    	s += " ";
		s += Chandy::DFtS(c->get_gx()) + " " +
			 Chandy::DFtS(c->get_gy()) + " " +
			 Chandy::DFtS(c->get_gz()) + " " +
			 Chandy::DFtS(c->get_ox()) + " " +
			 Chandy::DFtS(c->get_oy()) + " " +
			 Chandy::DFtS(c->get_oz()) + " " +
			 Chandy::DFtS(c->get_gt()) + " ";
    	if (i != _nchips-1) myfile << s <<"\n";
    	else myfile << s;
    }
    
    myfile.close();
}







//-----------------------------------------------------------------------------

void Ctel_chunk::load_alignments(std::string save_file_name) {
	std::ifstream myfile;
	myfile.open(_ops->alignment_save_file_name.c_str());
	std::string line;
	int ichip = 0;
	//Cycle over the .dat lines, noting each line corresponds to one chip.
	while (!myfile.eof() && ichip<_ops->nchips){
		getline (myfile, line);
		std::vector<std::string> line_bits = Chandy::split_line(line, 1);
		double temp_gposn[4] = {(double)atof(line_bits[1].c_str()),
							    (double)atof(line_bits[2].c_str()),
							    (double)atof(line_bits[3].c_str()),
							    0.0};

		double temp_orientation[3] = {(double)atof(line_bits[4].c_str()), 
									  (double)atof(line_bits[5].c_str()),
									  (double)atof(line_bits[6].c_str())};

		
		get_chips()[ichip]->set_orientation(temp_orientation);

		get_chips()[ichip]->set_gposn(temp_gposn);
		ichip++;
	}
	myfile.close();
}







//-----------------------------------------------------------------------------

void Ctel_chunk::setup_pix_hits_by_glob_x(){
	std::vector<Cchip*>::iterator ichip;
	for (ichip = get_chips().begin();
		 ichip != get_chips().end(); ++ichip){
		(*ichip)->setup_pix_hits_by_glob_x();
	}
}







//-----------------------------------------------------------------------------

void Ctel_chunk::dump_tel(bool appending, std::string save_file_name, bool check){
	//If appending, will add to the current tel in the DQM file.
	//Otherwise, will replace.

	std::cout<<"\n\nDumping tel ---------"<<std::endl;
	dump_chips(save_file_name, check);
	dump_tracks(appending, save_file_name, check);
	dump_clusters(appending, save_file_name, check);
	std::cout<<"\n\nDump done ---------"<<std::endl;
}







//-----------------------------------------------------------------------------

void Ctel_chunk::dump_chips(std::string s, bool check){
/*
	std::cout<<"Dumping chips."<<std::endl;
	TNtuple * ntuple = new TNtuple("all_chips","all_chips","ID:gx:gy:gz:gt:ox:oy:rotn:npix_hits:nclusters:pixel_width:pixel_height:chip_width:chip_height:corner1gx:corner1gy:corner1gz:corner2gx:corner2gy:corner2gz:corner3gx:corner3gy:corner3gz");
	
	double corner1g[4], corner2g[4], corner3g[4];
	//Fill Ntuple.
	std::vector<Cchip*>::iterator i;
	for (i = get_chips().begin(); i != get_chips().end(); ++i){
		double corner1l[4] = {0.0, (double)(*i)->get_size_pixs()[1], 0.0, 0.0};
		double corner2l[4] = {(double)(*i)->get_size_pixs()[0], (double)(*i)->get_size_pixs()[1], 0.0, 0.0};
		double corner3l[4] = {(double)(*i)->get_size_pixs()[0], 0.0, 0.0, 0.0};

		(*i)->lposn_to_gposn(corner1l, corner1g);
		(*i)->lposn_to_gposn(corner2l, corner2g);
		(*i)->lposn_to_gposn(corner3l, corner3g);


		double x[23] = {(double)(*i)->get_ID(), (*i)->get_gx(), (*i)->get_gy(), (*i)->get_gz(),
			(*i)->get_gt(), (*i)->get_ox(), (*i)->get_oy(), (*i)->get_rotn(), (double)(*i)->get_npix_hits(),
			(double)(*i)->get_nclusters(), (*i)->get_pixel_width(), (*i)->get_pixel_height(), 
			(*i)->get_chip_width(), (*i)->get_chip_height(), corner1g[0], corner1g[1], corner1g[2], 
			corner2g[0], corner2g[1], corner2g[2], corner3g[0], corner3g[1], corner3g[2]};

		ntuple->Fill(x);
	}

	//Save.
	TFile* save_file = new TFile(s.c_str(),"update");
	ntuple->Write("all_chips");
	delete ntuple;
	save_file->Close();






	//Check -------------------------------------------------------------------
	if (check){
		TFile* save_file = new TFile(s.c_str());
		TNtuple * ntuple = (TNtuple*)save_file->Get("all_chips");


		double ID, gx, gy, gz, gt, ox, oy, rotn, npix_hits, nclusters, pix_w, pix_h, chip_w, chip_h;
		ntuple->SetBranchAddress("ID", &ID);
		ntuple->SetBranchAddress("gx", &gx);
		ntuple->SetBranchAddress("gy", &gy);
		ntuple->SetBranchAddress("gz", &gz);
		ntuple->SetBranchAddress("gt", &gt);
		ntuple->SetBranchAddress("ox", &ox);
		ntuple->SetBranchAddress("oy", &oy);
		ntuple->SetBranchAddress("rotn", &rotn);
		ntuple->SetBranchAddress("npix_hits", &npix_hits);
		ntuple->SetBranchAddress("nclusters", &nclusters);
		ntuple->SetBranchAddress("pixel_width", &pix_w);
		ntuple->SetBranchAddress("pixel_height", &pix_h);
		ntuple->SetBranchAddress("chip_width", &chip_w);
		ntuple->SetBranchAddress("chip_height", &chip_h);


		for (int i=0; i < get_nchips(); ++i){
			Cchip * chip = get_chip(i);
			ntuple->GetEntry(i);
			if (chip->get_ID() != (int)ID || 
				chip->get_gx() != gx ||
				chip->get_gy() != gy ||
				chip->get_gz() != gz ||
				chip->get_gt() != gt ||
				chip->get_ox() != ox ||
				chip->get_oy() != oy ||
				chip->get_rotn() != rotn ||
				chip->get_npix_hits() != (int)npix_hits ||
				chip->get_nclusters() != (int)nclusters ||
				chip->get_pixel_width() != pix_w ||
				chip->get_pixel_height() != pix_h ||
				chip->get_chip_width() != chip_w ||
				chip->get_chip_height() != chip_h){
				std::cout<<"Chip TNtuple *not* read correctly :("<<std::endl;
			}
			else std::cout<<"Chip TNtuple read correctly :)"<<std::endl;
		}

		save_file->Close();
	}
	*/
}







//-----------------------------------------------------------------------------

void Ctel_chunk::dump_clusters(bool appending, std::string s, bool check){
/*
	std::cout<<"Dumping Clusters."<<std::endl;
	TNtuple * ntuple = new TNtuple("all_clusters","all_clusters","ID:chipID:column:row:TOA:gx:gy:gz:gt:ADC:size:tracked:pixID1:pixID2:pixID3:pixID4:pixID5:pixID6");


	//Allow for up to 6 pixel clusters.
	int pixIDs[6];
	std::vector<Cchip*>::iterator ichip;
	for (ichip = get_chips().begin(); ichip != get_chips().end(); ++ichip){
		std::vector<Ccluster*>::iterator ic;
		for (ic = (*ichip)->get_clusters().begin(); ic != (*ichip)->get_clusters().end(); ++ic){
			

			//Fill pix IDs.
			for (int i=0; i<6; i++){
				if (i < (*ic)->get_size()) pixIDs[i] = (*ic)->get_pix_hits()[i]->get_ID();
				else pixIDs[i] = -1;
			}

			//Fill tuple.
			double x[18] = {(*ic)->get_ID(), (*ic)->get_chipID(), (*ic)->get_column(), (*ic)->get_row(), (*ic)->get_TOA(),
				 (*ic)->get_gx(), (*ic)->get_gy(), (*ic)->get_gz(), (*ic)->get_gt(), (*ic)->get_ADC(),
				 (*ic)->get_size(),(*ic)->get_tracked(), pixIDs[0], pixIDs[1], pixIDs[2], pixIDs[3], pixIDs[4], pixIDs[5]};
			ntuple->Fill(x);
		}
	}



	//Save.
	TFile* save_file = new TFile(s.c_str(),"update");
	if (!appending) ntuple->Write("all_clusters"); //replace only for now.
	save_file->Close();
	delete ntuple;






	//Check //-----------------------------------------------------------------
	if (check){
		int ientry=0, nbad = 0;
		TFile* save_file = new TFile(s.c_str());
		TNtuple * ntuple = (TNtuple*)save_file->Get("all_clusters");


		//Temp variables.
		double ID, chipID, column, row, TOA, gx, gy, gz, gt, ADC, size, tracked;
		double check_pixIDs[6];


		//Assign variables.
		ntuple->SetBranchAddress("ID", &ID);
		ntuple->SetBranchAddress("chipID", &chipID);
		ntuple->SetBranchAddress("column", &column);
		ntuple->SetBranchAddress("row", &row);
		ntuple->SetBranchAddress("TOA", &TOA);
		ntuple->SetBranchAddress("gx", &gx);
		ntuple->SetBranchAddress("gy", &gy);
		ntuple->SetBranchAddress("gz", &gz);
		ntuple->SetBranchAddress("gt", &gt);
		ntuple->SetBranchAddress("ADC", &ADC);
		ntuple->SetBranchAddress("size", &size);
		ntuple->SetBranchAddress("tracked", &tracked);

		ntuple->SetBranchAddress("pixID1", &check_pixIDs[0]);
		ntuple->SetBranchAddress("pixID2", &check_pixIDs[1]);
		ntuple->SetBranchAddress("pixID3", &check_pixIDs[2]);
		ntuple->SetBranchAddress("pixID4", &check_pixIDs[3]);
		ntuple->SetBranchAddress("pixID5", &check_pixIDs[4]);
		ntuple->SetBranchAddress("pixID6", &check_pixIDs[5]);


		//Loop.
		std::vector<Cchip*>::iterator check_ichip;
		for (check_ichip = get_chips().begin(); check_ichip != get_chips().end(); ++check_ichip){
			std::vector<Ccluster*>::iterator check_ic;
			for (check_ic = (*check_ichip)->get_clusters().begin(); 
				check_ic != (*check_ichip)->get_clusters().end(); ++check_ic){
				ntuple->GetEntry(ientry);


				//Compare.
				if ((*check_ic)->get_ID() != (int)ID || 
					(*check_ic)->get_chipID() != (int)chipID ||
					(*check_ic)->get_column() != column ||
					(*check_ic)->get_row() != row ||
					(*check_ic)->get_TOA() != TOA ||
					(*check_ic)->get_gx() != gx ||
					(*check_ic)->get_gy() != gy ||
					(*check_ic)->get_gz() != gz ||
					(*check_ic)->get_gt() != gt ||
					(*check_ic)->get_ADC() != (int)ADC ||
					(*check_ic)->get_size() != (int)size ||
					(*check_ic)->get_tracked() != (int)tracked){
						std::cout<<"Cluster TNtuple *not* read correctly :(. Entry: "<<ientry<<std::endl;
						nbad++;

				}
				for (int i=0; i<fmin((*check_ic)->get_size(), 6); i++){
					if ((*check_ic)->get_pix_hits()[i]->get_ID() != (int)check_pixIDs[i]){
						std::cout<<"Cluster TNtuple (pix_hit) *not* read correctly :(. Entry: "<<std::endl;
						nbad++;
					}
				}
				ientry++;
			}
		}


		save_file->Close();
		//All done.
		std::cout<<"Num bad cluster reads: "<<nbad<<std::endl;
	}
*/
}







//-----------------------------------------------------------------------------

void Ctel_chunk::dump_tracks(bool appending, std::string s, bool check){
/*
	std::cout<<"Dumping tracks."<<std::endl;
	TNtuple * ntuple = new TNtuple("all_tracks","all_tracks","ID:mx:cx:my:cy:gTOA:clustID1:clustID2:clustID3:clustID4:clustID5:clustID6:clustID7:clustID8:clustID9:clustID10");


	//Allow for up to 10 clusters.
	int clustIDs[10];
	std::vector<Ctrack*>::iterator it;
	for (it = get_tracks().begin(); it != get_tracks().end(); ++it){

		//Fill clust IDs (per chip; -1 if not found).
		for (int ichip=0; ichip<10; ichip++) clustIDs[ichip] = (*it)->get_clustID_for_chipID(ichip);
		//Fill
		double x[16] = {(*it)->get_ID(), (*it)->get_mx(), (*it)->get_cx(), (*it)->get_my(), (*it)->get_cy(),
			 (*it)->get_gTOA(), clustIDs[0], clustIDs[1], clustIDs[2], clustIDs[3], clustIDs[4], clustIDs[5],
			 clustIDs[6], clustIDs[7], clustIDs[8], clustIDs[9]}; 
		ntuple->Fill(x);
	}


	//Save.
	TFile * save_file = new TFile(s.c_str(),"update");
	if (!appending) ntuple->Write("all_tracks"); //replace only for now.
	save_file->Close();
	delete ntuple;






	//Check //-----------------------------------------------------------------
	if (check){
		int ientry=0, nbad = 0;
		TFile* save_file = new TFile(s.c_str());
		TNtuple * ntuple = (TNtuple*)save_file->Get("all_tracks");


		//Temp variables.
		double ID, mx, cx, my, cy, gTOA;
		double check_clustIDs[10];


		//Assign variables.
		ntuple->SetBranchAddress("ID", &ID);
		ntuple->SetBranchAddress("mx", &mx);
		ntuple->SetBranchAddress("cx", &cx);
		ntuple->SetBranchAddress("my", &my);
		ntuple->SetBranchAddress("cy", &cy);
		ntuple->SetBranchAddress("gTOA", &gTOA);

		ntuple->SetBranchAddress("clustID1", &check_clustIDs[0]);
		ntuple->SetBranchAddress("clustID2", &check_clustIDs[1]);
		ntuple->SetBranchAddress("clustID3", &check_clustIDs[2]);
		ntuple->SetBranchAddress("clustID4", &check_clustIDs[3]);
		ntuple->SetBranchAddress("clustID5", &check_clustIDs[4]);
		ntuple->SetBranchAddress("clustID6", &check_clustIDs[5]);
		ntuple->SetBranchAddress("clustID7", &check_clustIDs[6]);
		ntuple->SetBranchAddress("clustID8", &check_clustIDs[7]);
		ntuple->SetBranchAddress("clustID9", &check_clustIDs[8]);
		ntuple->SetBranchAddress("clustID10", &check_clustIDs[9]);



		//Loop.
		std::vector<Ctrack*>::iterator check_it;
		for (check_it = get_tracks().begin(); check_it != get_tracks().end(); ++check_it){
			ntuple->GetEntry(ientry);

			//Compare.
			if ((*check_it)->get_ID() != (int)ID || 
				(*check_it)->get_mx() != mx ||
				(*check_it)->get_cx() != cx ||
				(*check_it)->get_my() != my ||
				(*check_it)->get_cy() != cy){
					std::cout<<"\nTrack TNtuple *not* read correctly :(. Entry: "<<ientry<<std::endl;
					// std::cout<<"ID\t"<<(*check_it)->get_ID()<<"\t"<<(int)ID<<std::endl;
					std::cout<<"mx\t"<<(*check_it)->get_mx()<<"\t"<<mx<<std::endl;
					// std::cout<<"my\t"<<(*check_it)->get_my()<<"\t"<<my<<std::endl;
					// std::cout<<"cx\t"<<(*check_it)->get_cx()<<"\t"<<cx<<std::endl;
					// std::cout<<"cy\t"<<(*check_it)->get_cy()<<"\t"<<cy<<std::endl;

					nbad++;
			}

			for (int i=0; i<10; i++){
				if ((*check_it)->get_clustID_for_chipID(i) != (int)check_clustIDs[i]){
					std::cout<<"Cluster TNtuple (cluster) *not* read correctly :(. Entry: "<<std::endl;
					nbad++;
				}
			}
			ientry++;
		}

		save_file->Close();
		//All done.
		std::cout<<"Num bad track reads: "<<nbad<<std::endl;
	}
*/
}







//-----------------------------------------------------------------------------
