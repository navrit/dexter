//main.cpp - the main of the command line version of the DQM. This function
//just creates an instance of DQM options (which should have been edited by 
//now), and runs the DQM.

//Author: Dan Saunders
//Date created: 07/01/13
//Last modified: 21/01/13
// #include "../DFitter.cpp"

#include "CDQM_options.h"
#include "headers/CDQM.h"



int main(int argc, char *argv[]) {
	gErrorIgnoreLevel = 5000;

	//Create an instance of options.
	CDQM_options* ops = new CDQM_options();
	std::cout<<"argc: "<<argc<<std::endl;
	if (argc > 3) std::cout<<"Too many arguements passed! Ignoring them"<<std::endl;
    else if (argc == 2) {
        ops->runNumber = atoi(argv[1]);
        std::cout<<"Setting default run number from command line as: "<<ops->runNumber<<std::endl;
    }

	else if (argc == 3) {
        ops->runNumber = atoi(argv[1]);
        ops->PSNumFix = atoi(argv[2]);
        std::cout<<"Setting default run number from command line as: "<<ops->runNumber<<std::endl;
        std::cout<<"Setting PSNumFix as: "<<ops->PSNumFix<<std::endl;
    }

	//Pass options to DQM and run.
	CDQM my_DQM(ops);
	my_DQM.initialize();
	my_DQM.execute(0.0);
	my_DQM.finalize();

	return 0;
}
