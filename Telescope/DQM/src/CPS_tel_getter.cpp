#include "../headers/CPS_tel_getter.h"
#include <dirent.h>


//-----------------------------------------------------------------------------

CPS_tel_getter::CPS_tel_getter(CDQM_options * ops){
	_eventNum = 0;
	// FUNCTION HAS BEEN HACKED TO DEATH.

	std::stringstream ssPSThing;
	ssPSThing<<ops->PSNumFix;
	std::string CheckStr;
	std::cout<<"Guys!\t"<<ops->PSNumFix<<std::endl;
	if (ops->PSNumFix<10) {
		CheckStr = "-" + ssPSThing.str() + ".dat";
		std::cout<<":)"<<std::endl;
	}
	else CheckStr = ssPSThing.str() + ".dat";
	std::cout<<"Constructor of CPS_tel_getter."<<std::endl;
	_ops = ops;
	_nPlanes = _ops->nchips;
    std::string fileTempS1 = "/mnt/DATA/Dev";

   // Set the file names.
    for (unsigned int i=0; i<_nPlanes; i++) {
    	if (i==7 && ops->runNumber == 1022){
    		std::cout<<"Shifting name as last chip."<<std::endl;
    		std::stringstream ssPSThing2;
    		ssPSThing2 << (ops->PSNumFix + 8);

    		if (!ops->PSNumFix>9) CheckStr = "-" + ssPSThing2.str() + ".dat";
    		else CheckStr = ssPSThing2.str() + ".dat";

    		std::cout<<CheckStr<<std::endl;
    	}
        std::stringstream ssChip, ssRun;
		ssChip<<i;
		ssRun<<ops->runNumber;
		std::string direcName = fileTempS1 + ssChip.str() + "/Run" + ssRun.str(); 
		std::cout<<"Searching directory: "<<direcName<<std::endl;

        DIR* dir = opendir(direcName.c_str());
		if (dir == NULL) {
			std::cout<<"Unable to open directory: " <<direcName<<std::endl;
			exit (EXIT_FAILURE);
		}

		struct dirent* entry; 
		int ifile = 0;
		while ((entry = readdir(dir)) != NULL) {
			int i = strlen(entry->d_name);
			std::string fileNameStr = std::string(entry->d_name);
			std::cout<<"Considering file ID with name: "<<ifile<<" "<<entry->d_name<<std::endl;
			if (i>7) {
				std::string subFileNameStr = fileNameStr.substr(i-6, i-1);
				std::cout << subFileNameStr << "\t (Actual) vs (Should be)\t " << CheckStr<<std::endl;
  	    		if (subFileNameStr == CheckStr){
					_input_file_names.push_back(direcName + "/" + fileNameStr);
					break;
				}
			}
			ifile ++;
		}


		std::cout<<"Used file with chip ID: "<<_input_file_names[i]<<" "<<i<<std::endl;
    }

    //exit (EXIT_FAILURE);
}



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

void CPS_tel_getter::fill_pixels(){
	int header_size = 704;
	for (int chip_ID=0; chip_ID<_ops->nchips && chip_ID < _ops->chip_cut; chip_ID++) {
		std::cout<<"Getting data for chip\t:"<<chip_ID<<"\t"<<"from file:\t"<<_input_file_names[chip_ID]<<std::endl;
		std::ifstream file (_input_file_names[chip_ID].c_str(), std::ios::in|std::ios::binary|std::ios::ate); 
		if (file.is_open()) {
			std::streampos size = file.tellg();
			char * memblock = new char [size];
			file.seekg (0, std::ios::beg);
			file.read (memblock, size);
			file.close();

			int i=0;
			long long * m_data = (long long*)(&memblock[0] + header_size);
			long long * m_end = (long long*)(&memblock[size]);

			while (m_data != m_end && i<_ops->nEvents * _ops->nPixHitCut) {
				Cpix_hit* my_pixel = line_to_pixel(*(m_data++), chip_ID);
				if (my_pixel!=NULL &&
					my_pixel->get_TOA() > _eventNum*_ops->tEventStep &&
					my_pixel->get_TOA() < (_eventNum+1)*_ops->tEventStep) {

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
			}
			delete memblock;
		}
		else std::cout<<"Can't open file:\t"<<_input_file_names[chip_ID]<<std::endl;
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
			if (!_tel->_tzero_set) {
				_tel->_tzero = ((pixdata & 0x000000000000FFFF) << 18) + (((data & 0x0FFFC000) >> 14) << 4) + (15 - (data & 0xF0000000));
				my_pixel->set_TOA(0.0);
				_tel->_tzero_set = true;
				//std::cout<<"DAN!\t"<<_tel->_tzero<<std::endl;
			}

			else {
				my_pixel->set_TOA((double)((pixdata & 0x000000000000FFFF) << 18) + (((data & 0x0FFFC000) >> 14) << 4) + (15 - (data & 0xF0000000)) - _tel->_tzero);
				//std::cout<<my_pixel->get_TOA()<<std::endl;
			}
			return my_pixel;
	    }
	    else {
	    	delete my_pixel;
	    	return NULL;
	    }
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


	std::cout << "Num chips created:\t"<< _tel->get_nchips() << std::endl;
	myfile.close();
}







//-----------------------------------------------------------------------------

Cchip * CPS_tel_getter::make_chip(std::string line, int ichip){
	//Function to turn the line of an alignment file into a chip.
	//**Should be edited for each telescope**.
	std::cout<<__LINE__<<std::endl;

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


	float temp_gposn[4] = {0.0,
							0.0,
							(float)atof(line_bits[3].c_str()), 
							0.0};

	my_chip->set_gposn(temp_gposn);

	float temp_orientation[3] = {0.0, 0.0, 0.0};
	my_chip->set_orientation(temp_orientation);

	my_chip->set_rotn(0.0); 
	return my_chip;
}






//-----------------------------------------------------------------------------

CPS_tel_getter::~CPS_tel_getter(){

}





//-----------------------------------------------------------------------------


