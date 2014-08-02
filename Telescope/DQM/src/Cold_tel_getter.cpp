#include "../headers/Cold_tel_getter.h"


//-----------------------------------------------------------------------------

Cold_tel_getter::Cold_tel_getter(CDQM_options * ops){
	std::cout<<"Constructor of Cold_tel_getter."<<std::endl;
	_ops = ops;
}



//-----------------------------------------------------------------------------

void Cold_tel_getter::initialize(){
	_tcut = _ops->tcut;
}



//-----------------------------------------------------------------------------

void Cold_tel_getter::execute(Ctel_chunk * tel){
	// Add the chips (according to the alignment file).
	_tel = tel;
	fill_chips();
	fill_pixels();


	// Order them.
	_tel->time_order_pixels_comb();
	if(_ops->also_order_pixs_x) _tel->setup_pix_hits_by_glob_x();
}



//-----------------------------------------------------------------------------

void Cold_tel_getter::finalize(){
}



//-----------------------------------------------------------------------------

void Cold_tel_getter::fill_pixels(){
	//Each pixel is a line in a text file.

	std::ifstream myfile;
	myfile.open("test_data/MayData.txt");
	std::string line;
	while (!myfile.eof()){

		//Get the pixel line as a string, then convert to instance of pixel_hit.
		getline (myfile, line);
		if (line.size()>2) {
			int chip_ID;
			Cpix_hit* my_pixel = line_to_pixel(line, chip_ID);
			my_pixel->set_chipID(chip_ID);

			//Add to the chip.
			_tel->get_chip(chip_ID)->add_pix_hit(my_pixel);
			if (my_pixel->get_TOA() > _tcut && _ops->truncate) break;
		}
	}
	myfile.close();
	set_npixels();
	reset_all_pixel_ids();
}					







//-----------------------------------------------------------------------------

void Cold_tel_getter::reset_all_pixel_ids(){
	//Cycle over chips, and set pixel ID's via a method in the chip instance.

	std::vector<Cchip*>::iterator ichip;
	for (ichip = _tel->get_chips().begin();
		 ichip != _tel->get_chips().end(); ++ichip){
		(*ichip)->reset_pixel_IDs();
	}
}







//-----------------------------------------------------------------------------

void Cold_tel_getter::set_npixels(){
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

Cpix_hit * Cold_tel_getter::line_to_pixel(std::string line, int &chip_id){
	Cpix_hit* my_pixel = new Cpix_hit();

	//Old data has a spacing of 4.
	std::vector<std::string> line_bits = Chandy::split_line(line, 4);

	//Put the bits into a Cpixel_hit instance.
	chip_id = _tel->chipname_to_id(line_bits[0]);

	my_pixel->set_ADC(atoi(line_bits[1].c_str()));
	my_pixel->set_TOA(make_TOA(atof(line_bits[4].c_str()), chip_id));

	int x = atoi(line_bits[3].c_str());
	int y = atoi(line_bits[2].c_str());
	my_pixel->set_column(x);
	my_pixel->set_row(y);

	return my_pixel;
}







//-----------------------------------------------------------------------------

double Cold_tel_getter::make_TOA(double TOA, int ichip){
	//Simulates a more realistic TOA. Say a small random jitter, and a
	//cummulative event a-sync of 0.1 events.

	double gitter = 0.01*(rand()/((double)RAND_MAX));
	gitter = 0.0;
	double chip_async = 0.1*ichip;
	//double TOA += exact_TOA + chip_async + gitter;

	return TOA;
}







//-----------------------------------------------------------------------------

void Cold_tel_getter::fill_chips(){
	//Chip information is in the alignment file.
	std::ifstream myfile;
	myfile.open(_ops->alignment_save_file_name.c_str());
	std::string line;


	//Cycle over the .dat lines, noting each line corresponds to one chip.
	int chipID = 0;
	while (!myfile.eof()){
		getline (myfile, line);
		_tel->add_chip(make_chip(line, chipID));
		chipID++;
	}
	_tel->set_nchips(_tel->get_chips().size());


	std::cout << "Num chips created:\t"<< _tel->get_nchips() << std::endl;
	myfile.close();
}







//-----------------------------------------------------------------------------

Cchip * Cold_tel_getter::make_chip(std::string line, int ichip){
	//Function to turn the line of an alignment file into a chip.
	//**Should be edited for each telescope**.


	Cchip * my_chip = new Cchip();
	//Split the line according to tabs or spaces.
	std::vector<std::string> line_bits = Chandy::split_line(line, 1);


	//Find the chip attributes.
	my_chip->set_name(line_bits[0]);
	my_chip->set_ID(ichip);
	// double temp_gposn[4] = {(double)atof(line_bits[1].c_str()),
	// 						(double)atof(line_bits[2].c_str()),
	// 						(double)atof(line_bits[3].c_str()), 
	// 						0.0};


	double temp_gposn[4] = {0.0,
							0.0,
							(double)atof(line_bits[3].c_str()), 
							0.0};

	my_chip->set_gposn(temp_gposn);

	double temp_orientation[3] = {0.157, 0.157, 0.0};
	my_chip->set_orientation(temp_orientation);

	my_chip->set_rotn(0.0); 
	return my_chip;
}






//-----------------------------------------------------------------------------

Cold_tel_getter::~Cold_tel_getter(){

}





//-----------------------------------------------------------------------------


