#include "../headers/Ccorrel_line_finder.h"


//-----------------------------------------------------------------------------




Ccorrel_line_finder::Ccorrel_line_finder(TH2F* plot, double theta_lower, 
	double theta_upper, int nproj_bins, bool save_switch, TDirectory* save_dir){
	_save_dir = save_dir;
	_save_switch = save_switch;
	_nproj_bins = nproj_bins;
	set_plot(plot);
	set_theta_upper(theta_upper);
	set_theta_lower(theta_lower);
	_line = new Ccorrel_line();
	_n_projections = 100;
	_c1 = new TCanvas("Hist&Line");
}




//-----------------------------------------------------------------------------




void Ccorrel_line_finder::varying_projections(){
	fill_projections();
	fill_stationary_plot();
	set_line();
	add_line();

	if (_save_switch) save_figs();
}




//-----------------------------------------------------------------------------




void Ccorrel_line_finder::add_line(){
	double xlow = _plot->GetXaxis()->GetXmin();
	double xup = _plot->GetXaxis()->GetXmax();

	double ylow = _line->y(xlow);
	double yup = _line->y(xup);

	_plot->Draw("colz");
	TLine *line = new TLine(xlow, ylow, xup, yup);
	line->SetLineColor(kRed);
	line->Draw();

	std::cout<<"Correlation m:"<<_line->get_theta()<<"\t"<<"c:"<<_line->get_c_shift()<<std::endl;
}




//-----------------------------------------------------------------------------



void Ccorrel_line_finder::set_line(){
	_line->set_theta(_temp_theta);
	_line->set_del_theta(0.0);

	//make the best projection.
	TH1F* h = Chandy::project_2dhist(get_plot(), _line->get_theta(), _nproj_bins, "Best_Proj");
	_projections.push_back(h);
	fill_signal_projection(_projections.size()-1);

	double c = _signal_projections[_signal_projections.size()-1]->GetMean();
	_line->set_c_shift(-c/cos(_line->get_theta()));

	double inner_bandN = 0;
	double outer_bandN = 0;

	//1 bin (either side, and including) about c_shift divided by 2 bins about c_shift.
	int icbin = h->FindBin(_line->get_c_shift());
	inner_bandN += (h->GetBinContent(icbin - 1) + h->GetBinContent(icbin) + h->GetBinContent(icbin + 1));
	outer_bandN += (inner_bandN + h->GetBinContent(icbin - 2) + h->GetBinContent(icbin + 2));

	double SN = inner_bandN/(1.0*outer_bandN);
	_line->set_SN(SN);
}




//-----------------------------------------------------------------------------




void Ccorrel_line_finder::fill_stationary_plot(){
	_temp_theta = 0.0;
	double temp_x = 0.0;
	double xs[_n_projections];
	double thetas[_n_projections];
	double theta_step = (get_theta_upper() - get_theta_lower())/_n_projections;
	//double shift_Cs[_n_projections];

	int dummy;
	//cycle over projections.
	for (int i=0; i!=_n_projections; i++){
		xs[i] = find_height_stationary_parameter(i, dummy);
		thetas[i] = get_theta_lower() + i*theta_step;

		if (xs[i] > temp_x){
			 _temp_theta = thetas[i];
			 temp_x = xs[i];
		}
	}

	//make graph.
	_stationary_graph = new TGraph(_n_projections, thetas, xs);
	_stationary_graph->SetTitle("Stationary_plot");
}




//-----------------------------------------------------------------------------



void Ccorrel_line_finder::fill_signal_projection(int iproj){

	TH1F* h = _projections[iproj];
	//still need the bin num of the heighest.
	int mid_bin;
	double dummy = find_height_stationary_parameter(iproj, mid_bin);

	//cycle over 2 bins either side - no need to do more, as signal should
	//be inside here - and if its not, the width will be massive anyway.
	//Use similar triangles to get the predicted height.
	double left_av = 0.0;
	double right_av = 0.0;
	for (int i=0; i<3; i++){
		left_av += h->GetBinContent(h->GetBin(mid_bin-3-i));
		right_av += h->GetBinContent(h->GetBin(mid_bin+3+i));
	}

	//average.
	left_av/=3; right_av/=3;


	//check this actually works on a copy.
	TH1F *hcapped = (TH1F*)h->Clone("hcapped");
	TH1F *hsignal = (TH1F*)h->Clone("hcapped");


	//Empty hsignal.
	for (int i=0; i<hsignal->GetNbinsX(); i++){
		hsignal->SetBinContent(hsignal->GetBin(i), 0.0);
	}


	//Fill capps and subtracted signals.
	for (int i=0; i<6; i++){
		double capped_height
			= left_av + ((right_av - left_av)/5)*i;
		double signal_height
			= h->GetBinContent(h->GetBin(mid_bin+i-2)) - capped_height;

		hcapped->SetBinContent(hcapped->GetBin(mid_bin+i-2), capped_height);
		hsignal->SetBinContent(hsignal->GetBin(mid_bin+i-2), signal_height);
	}
	_capped_projections.push_back(hcapped);
	_signal_projections.push_back(hsignal);
}




//-----------------------------------------------------------------------------




double Ccorrel_line_finder::find_height_stationary_parameter(
		int iproj, int & mid_bin){
	//for now we'll use the height of the two heighest bin cause im lazy/curious.
	//assume its 0; then iterate.
	double height = 0.0;
	double trial_height = 0.0;

	TH1F* h = _projections[iproj];
	for (int i=1; i < h->GetNbinsX()-1; i++){
		int x = h->GetBin(i);
		trial_height = h->GetBinContent(x-1) + h->GetBinContent(x);
		if (trial_height > height){
			height = trial_height;
			mid_bin = i;
		}
	}
	return (double) height;
}




//-----------------------------------------------------------------------------




void Ccorrel_line_finder::fill_projections(){
	double theta_step = (get_theta_upper() - get_theta_lower())/_n_projections;

	for (double itheta = get_theta_lower();
			   itheta<get_theta_upper();
			   itheta += theta_step){

		_projections.push_back(Chandy::project_2dhist(get_plot(), itheta, _nproj_bins));
	}
}




//-----------------------------------------------------------------------------




void Ccorrel_line_finder::save_figs(){
	_save_dir->cd();
	_plot->Write();
	_stationary_graph->Write("Stationary_plot");
	_c1->Write();
	_projections[_projections.size()-1]->Write("", TObject::kWriteDelete);


	TH1F * h = new TH1F("correl_results_holder", "correl_results_holder", 5, 0, 5);
	h->SetBinContent(h->GetBin(1), _line->get_theta());
	h->SetBinContent(h->GetBin(2), 0.0);
	h->SetBinContent(h->GetBin(3), _line->get_c_shift());
	h->SetBinContent(h->GetBin(4), 0.0);
	h->SetBinContent(h->GetBin(5), _line->get_SN());
	h->Write("correl_results_holder");
	delete h;


	//Just for the sake of it.
	_save_dir->mkdir("Projections");
	_save_dir->cd("Projections");
	for (int i=0; i<_projections.size(); i+=10){
		std::stringstream ss;
		ss<<i;
		_projections[i]->Write((ss.str()).c_str(), TObject::kWriteDelete);
	}
	_save_dir->cd("../");



	_save_dir->mkdir("Signal_projections");
	_save_dir->cd("Signal_projections");
	for (int i=0; i<_signal_projections.size(); i+=10){
		std::stringstream ss;
		ss<<i;
		_signal_projections[i]->Write((ss.str()).c_str(), TObject::kWriteDelete);
	}
	_save_dir->cd("../");



	_save_dir->mkdir("Capped_projections");
	_save_dir->cd("Capped_projections");
	for (int i=0; i<_capped_projections.size(); i+=10){
		std::stringstream ss;
		ss<<i;
		_capped_projections[i]->Write((ss.str()).c_str(), TObject::kWriteDelete);
	}
	
	_save_dir->cd("../");

}




//-----------------------------------------------------------------------------





Ccorrel_line_finder::~Ccorrel_line_finder(){
	for (int i=0; i<_projections.size(); i+=10) delete _projections[i];
	for (int i=0; i<_capped_projections.size(); i+=10) delete _capped_projections[i];
	for (int i=0; i<_signal_projections.size(); i+=10) delete _signal_projections[i];

	delete _stationary_graph;
	delete _c1;
	delete _line;
}




//-----------------------------------------------------------------------------
