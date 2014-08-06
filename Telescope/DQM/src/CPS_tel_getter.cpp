#include "../headers/CPS_tel_getter.h"
#include <dirent.h>


//-----------------------------------------------------------------------------

CPS_tel_getter::CPS_tel_getter(CDQM_options * ops){
	std::cout<<"Constructor of CPS_tel_getter."<<std::endl;
	_eventNum = 0;
	_ops = ops;
	_nPlanes = _ops->nchips;


	// Fill possible directory names.
	std::vector<std::string> possibleDevDirecNames = getPossibleDevDirecNames();
	// if (possibleDevDirecNames.size() < _ops->nchips) {
	// 	std::cout<<"Not enough device directories to match nchips (as set in CDQM_options) for this run."<<std::endl;
	// 	exit (EXIT_FAILURE);
	// }
	// else if (possibleDevDirecNames.size() > _ops->nchips) {
	// 	std::cout<<"Too many device directories to match nchips (as set in CDQM_options) for this run."<<std::endl;
	// 	exit (EXIT_FAILURE);
	// }

	_possibleFileNames = getPossibleFileNames(possibleDevDirecNames);

	for (std::vector<std::string>::iterator idir = _possibleFileNames.begin();
		 idir != _possibleFileNames.end(); idir++)
		std::cout<<(*idir)<<std::endl;
}



//-----------------------------------------------------------------------------

std::vector<std::string> CPS_tel_getter::getPossibleFileNames(std::vector<std::string> possibleDevDirecNames) {
	std::vector<std::string> fileNames;

	// Loop over to find possible file names.
	for (std::vector<std::string>::iterator idir = possibleDevDirecNames.begin();
		 idir != possibleDevDirecNames.end(); idir++) {

		DIR* dir = opendir((*idir).c_str());
		if (dir == NULL) {
			std::cout<<"Unable to open data directory: "<<"\t"<<(*idir).c_str()<<std::endl;
			std::cout<<"Check alignment file has correct plane information."<<std::endl;
			exit (EXIT_FAILURE);
		}

		// Loop over entries.
		struct dirent* entry;
		while ((entry = readdir(dir)) != NULL) {
			int i = strlen(entry->d_name);
			std::string fileName = std::string(entry->d_name);
			std::cout<<"Considering if appropriate file: "<<fileName<<std::endl;
			if (i<7) continue;
			if (fileName.substr(i-6, i-1) == "-1.dat") fileNames.push_back((*idir) + fileName);
		}

		delete dir;
		delete entry;
	}
	return fileNames;
}




//-----------------------------------------------------------------------------

std::vector<std::string> CPS_tel_getter::getPossibleDevDirecNames() {
	std::vector<std::string> direcNames;
	std::stringstream ssRun;
	ssRun<<_ops->runNumber;

	// Open the data directory.
	std::string dataDirec = "/mnt/DATA/";
	DIR* dir = opendir(dataDirec.c_str());
	if (dir == NULL) {
		std::cout<<"Unable to open data directory."<<std::endl;
		std::cout<<"Check alignment file has correct plane information."<<std::endl;
		exit (EXIT_FAILURE);
	}

	// Loop over entries.
	struct dirent* entry;
	int ichip = 0;
	while ((entry = readdir(dir)) != NULL && ichip != _ops->nchips) {
		int i = strlen(entry->d_name);
		std::string devDirec = std::string(entry->d_name);
		if (devDirec.substr(0, 3) == "Dev" && devDirec.substr(i-1, i) != "9") {
			direcNames.push_back(dataDirec + devDirec + "/Run" + ssRun.str() + "/");
			ichip++;
			std::cout<<devDirec<<"\t"<<ichip<<std::endl;
		}
	}

	return direcNames;
}



//-----------------------------------------------------------------------------

//	// Set up conditions for selecting the correct data files.
//
//	std::stringstream ssPSThing;
//	ssPSThing<<ops->PSNumFix;
//	std::string CheckStr;
//	if (ops->PSNumFix<10) {
//		CheckStr = "-" + ssPSThing.str() + ".dat";
//		std::cout<<":)"<<std::endl;
//	}
//	else CheckStr = ssPSThing.str() + ".dat";
//
//
//
//    // Set the file names.
//    for (unsigned int i=0; i<_nPlanes; i++) {
//    	_lsbs.push_back(0);
//    	_global_times.push_back(0);
//        std::stringstream ssChip, ssRun;
//		ssChip<<i;
//		ssRun<<ops->runNumber;
//		std::string direcName = fileTempS1 + ssChip.str() + "/Run" + ssRun.str();
//		std::cout<<"Searching directory: "<<direcName<<std::endl;
//
//        DIR* dir = opendir(direcName.c_str());
//		if (dir == NULL) {
//			std::cout<<"Unable to open directory: " <<direcName<<std::endl;
//			exit (EXIT_FAILURE);
//		}
//
//		struct dirent* entry;
//		int ifile = 0;
//		while ((entry = readdir(dir)) != NULL) {
//			int i = strlen(entry->d_name);
//			std::string fileNameStr = std::string(entry->d_name);
//			std::cout<<"Considering file ID with name: "<<ifile<<" "<<entry->d_name<<std::endl;
//			if (i>7) {
//				std::string subFileNameStr = fileNameStr.substr(i-6, i-1);
//				std::cout << subFileNameStr << "\t (Actual) vs (Should be)\t " << CheckStr<<std::endl;
//  	    		if (subFileNameStr == CheckStr){
//					_input_file_names.push_back(direcName + "/" + fileNameStr);
//					break;
//				}
//			}
//			ifile ++;
//		}
//		std::cout<<"Used file with chip ID: "<<_input_file_names[i]<<" "<<i<<std::endl;
//    }

    //exit (EXIT_FAILURE);
//}



//-----------------------------------------------------------------------------

void CPS_tel_getter::initialize(){
	std::cout<<"Init of CPS_tel_getter"<<std::endl;
	_tcut = _ops->tcut;
}



//-----------------------------------------------------------------------------

void CPS_tel_getter::execute(Ctel_chunk * tel){
	_tel = tel;

	// Add the chips (according to the alignment file).
	fill_chips();
	fill_pixels();


	// Order them.
	_tel->time_order_pixels_comb();
	if(_ops->algorithms_contains(4)) _tel->setup_pix_hits_by_glob_x();
	_eventNum++;
}



//-----------------------------------------------------------------------------

void CPS_tel_getter::finalize(){
}


//-----------------------------------------------------------------------------

bool CPS_tel_getter::fileNameCorrespondsToChip(std::string fileName, std::string chipName){
	if (fileName.substr(23, chipName.size()) == chipName) {
		return true;
		std::cout<<fileName<<"\t"<<chipName<<std::endl;
	}
	else {
	// 	std::cout<<"Shit."<<std::endl;
	// 	std::cout<<fileName.substr(23, chipName.size())<<"\t"<<chipName<<std::endl;
		return false;
	}
}



//-----------------------------------------------------------------------------

std::string CPS_tel_getter::getChipFileByName(int chip_ID){
	std::string fileName;
	std::string chipName = _tel->get_chip(chip_ID)->get_name();
	for (std::vector<std::string>::iterator it = _possibleFileNames.begin();
		 it != _possibleFileNames.end(); it++) {
		if (fileNameCorrespondsToChip((*it), chipName)) {
			fileName = (*it);
			break;
		}
	}
	return fileName;
}


//-----------------------------------------------------------------------------

void CPS_tel_getter::fill_pixels(){
	int header_size = 704;
	for (int chip_ID=0; chip_ID<_ops->nchips && chip_ID < _ops->chip_cut; chip_ID++) {
		std::string fileName = getChipFileByName(chip_ID);
		//std::string fileName = _possibleFileNames[chip_ID];
		std::cout<<"Getting data for chip\t:"<<chip_ID<<"\t"<<"from file:\t"<<fileName<<std::endl;
		std::ifstream file (fileName.c_str(), std::ios::in|std::ios::binary|std::ios::ate); 
		if (file.is_open()) {
			int i=0;
			// Set up some counters, the total, and set position to start of header.
			int c = header_size;
			int total = (int)file.tellg();
			file.seekg(header_size, std::ios::beg);

			int bufferSize = 10000;
			// Loop.
			while (c != total && i<_ops->nPixHitCut) {
				int size = 8*bufferSize;
				char * memblock = new char [size];
				file.read (memblock, size);
				long long * m_data = (long long*)(&memblock[0]);

				for (int k=0; k<bufferSize; k++){
					Cpix_hit* my_pixel = line_to_pixel((*m_data), chip_ID);
					if (my_pixel!=NULL &&
						my_pixel->get_TOA() > _eventNum*_ops->tEventStep &&
						my_pixel->get_TOA() < (_eventNum+1)*_ops->tEventStep) {
						// Take the hit.
						my_pixel->set_chipID(chip_ID);
						_tel->get_chip(chip_ID)->add_pix_hit(my_pixel);
						i++;
					}
					else {
						if (my_pixel != NULL && my_pixel->get_TOA() > (_eventNum+1)*_ops->tEventStep) {
							std::cout<<"Truncating event time at: "<<my_pixel->get_TOA()<<std::endl;
							break;
						}
						if (my_pixel != NULL) delete my_pixel;
					}

					m_data++;
				}

				// Preparation to move on.
				file.seekg (size, std::ios::cur); // Move ahead.
				if (c==total) _tel->isLastChunk = true;
				//if (c%10000==0) std::cout<<c<<"\t"<<total<<std::endl;
				c++;
			}
			file.close();
		}
		else std::cout<<"Can't open file:\t"<<chip_ID<<"\t"<<_input_file_names[chip_ID]<<std::endl;
	}

	set_npixels();
	reset_all_pixel_ids();
}					







//-----------------------------------------------------------------------------

void CPS_tel_getter::reset_all_pixel_ids(){
	//Cycle over chips, and set pixel ID's via a method in the chip instance.

	std::vector<Cchip*>::iterator ichip;
	for (ichip = _tel->get_chips().begin();
		 ichip != _tel->get_chips().end(); ++ichip){
		(*ichip)->reset_pixel_IDs();
	}
}







//-----------------------------------------------------------------------------

void CPS_tel_getter::set_npixels(){
	//set npixels, and print out.

	std::cout<<"\nChip\t\t"<<"Num pixels"<<std::endl;
	int tot_n = 0;
	for (int ichip = 0; ichip < _tel->get_nchips(); ichip++){

		Cchip* my_chip = _tel->get_chip(ichip);
		my_chip->set_npix_hits(my_chip->get_pix_hits().size());


		//Just print-outs left.
		std::cout<< my_chip->get_name()<<"\t"<<
					my_chip->get_npix_hits()<< std::endl;

		tot_n += my_chip->get_npix_hits();
	}


	std::cout<<"Total num pixel hits:"<<tot_n<<std::endl;
}







//-----------------------------------------------------------------------------

Cpix_hit * CPS_tel_getter::line_to_pixel(long long pixdata, int chip_id){
	const unsigned int header = ((pixdata & 0xF000000000000000) >> 60);

	// Ask if a pixel packet.
    if (header == 0xA || header == 0xB) {
    	Cpix_hit* my_pixel = new Cpix_hit();
    	const unsigned int pixdata2 = (pixdata & 0x00000FFFFFFF0000) >> 16;
		unsigned int x = (pixdata2 & 0x00003FF0) >> 4;
		my_pixel->set_ADC((int)x);
		if (my_pixel->get_ADC() == 0) my_pixel->set_ADC(1);
	    const unsigned int pix  = (pixdata & 0x0000700000000000) >> 44;
		my_pixel->set_chipID(chip_id);
		// Get the pixel address, first the double column. 
	    const unsigned int dcol = (pixdata & 0x0FE0000000000000) >> 52;
	    // Get the super pixel address.
	    const unsigned int spix = (pixdata & 0x001F800000000000) >> 45;
	    // Get the address of the pixel within the super pixel. 
	    my_pixel->set_column(dcol + pix / 4);
	    my_pixel->set_row(spix + (pix & 0x3));
	    const unsigned int data = (pixdata & 0x00000FFFFFFF0000) >> 16;

	    if (!_ops->maskedPixel(my_pixel->get_column(), my_pixel->get_row())) {
	    	long long spidrTime( pixdata & 0x000000000000FFFF),
                      ftoa(data & 0x0000000F), toa((data & 0x0FFFC000) >> 14);

	    	// Cast as double.
            //double t = (double)extendTimeStamp(((spidrTime << 18) + (toa << 4) + (15 - ftoa)), chip_id);
	    	double t = (double)((spidrTime << 18) + (toa << 4) + (15 - ftoa));

            // Assign this time to the pixel.
			if (!_tel->_tzero_set) {
				// First tel chunk hit case.
				_tel->_tzero = t;
				_tel->_tzero_set = true;
				my_pixel->set_TOA(0.0);
			}
			else my_pixel->set_TOA(t - _tel->_tzero);


			return my_pixel;
	    }
	    else {
	    	delete my_pixel;
	    	return NULL;
	    }
	}

	else if (header == 0x4 && (0xF & (pixdata >> 56)) != 0xF) {
		// New timing packet case.
		//addTimingPacket(pixdata, chip_id);
		return NULL;
	}

	else return NULL;
}





//-----------------------------------------------------------------------------

void CPS_tel_getter::fill_chips(){
	//Chip information is in the alignment file.
	std::ifstream myfile;
	myfile.open(_ops->alignment_save_file_name.c_str());
	std::string line;


	//Cycle over the .dat lines, noting each line corresponds to one chip.
	int chipID = 0;
	while (!myfile.eof() && chipID < _ops->nchips){
		getline (myfile, line);
		_tel->add_chip(make_chip(line, chipID));
		//if (chipID == _ops->chip_cut) break;
		chipID++;
	}
	_tel->set_nchips(_tel->get_chips().size());
	_ops->nchips = _tel->get_chips().size();


	std::cout << "Num chips created:\t"<< _tel->get_nchips() << std::endl;
	myfile.close();
}







//-----------------------------------------------------------------------------

Cchip * CPS_tel_getter::make_chip(std::string line, int ichip){
	//Function to turn the line of an alignment file into a chip.
	//**Should be edited for each telescope**.
	Cchip * my_chip = new Cchip(_ops);
	//Split the line according to tabs or spaces.
	std::vector<std::string> line_bits = Chandy::split_line(line, 1);

	//Find the chip attributes.
	my_chip->set_name(line_bits[0]);
	my_chip->set_ID(ichip);
	// float temp_gposn[4] = {(float)atof(line_bits[1].c_str()),
	// 						(float)atof(line_bits[2].c_str()),
	// 						(float)atof(line_bits[3].c_str()), 
	// 						0.0};

	double temp_gposn[4] = {0.0,
							0.0,
							atof(line_bits[3].c_str()), 
							0.0};
	my_chip->set_gposn(temp_gposn);
	double temp_orientation[3] = {0.0, 0.0, 0.0};
	my_chip->set_orientation(temp_orientation);
	my_chip->set_rotn(0.0); 
	return my_chip;
}






//-----------------------------------------------------------------------------

CPS_tel_getter::~CPS_tel_getter(){

}





//-----------------------------------------------------------------------------

long long CPS_tel_getter::extendTimeStamp(long long packet_time, int ichip) {
  int diff = (0x3 & (_global_times[ichip] >> 40)) - (0x3 & (packet_time >> 40));
  long long one = (uint64_t)(1) << 40;

  if (diff == 1 || diff == -3) _global_times[ichip] -= one;
  else if (diff == -1 || diff == 3) _global_times[ichip] += one;

  return (0x3FFFFFFFFFF & packet_time) + (_global_times[ichip] & 0xFFFFC0000000000);
}



//-----------------------------------------------------------------------------

void CPS_tel_getter::addTimingPacket(long long data_packet, int ichip) {
	const unsigned int subheader = 0xF & (data_packet >> 56);
	if ((0xFFFF & data_packet) == 0xAAAA) {
	  if (subheader == 0x5) _global_times[ichip] = (_lsbs[ichip] + (0xFFFFFFFF & (data_packet >> 16) << 32)) << 12;
	  else if (subheader == 0x4) _lsbs[ichip] = (0xFFFFFFFF & (data_packet >> 16));
	}

	// Might be an issue with a ten second jump.
}



//-----------------------------------------------------------------------------

