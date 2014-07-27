#include "DQM_widg.h"


//_____________________________________________________________________________

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent),
    ui(new Ui::MainWindow){
	this->setStyleSheet("QWidget{font-size:13px}");

    ui->setupUi(this);
    ui->RunNumBox->setMaximum(50000);
	this->resize(1450,950);
    //qApp->aboutQt();

    _AutoUpdate = false;
    _liveVsSaved = true;

    _DQMclock = new QDateTime();
    QDateTime start_time;
    start_time.setTime(_DQMclock->currentDateTime().time());
    start_time.setDate(_DQMclock->currentDateTime().date());
    //Set default values.
    _fileopen = false;
    _DQM_ran = false;
    _nchips = 0;
    _ref_file = "ref_DQM.root";
    _include_references = false;
    _DQM = (NULL);
    _nsample = 0;
    ui->SampleNumLab->setText("0");
    ui->SampleTimeLab->setText(_DQMclock->currentDateTime().toString());

    //The GUI has an instance of CDQM_options which is always editable, and
    //only deleted upon closing the GUI.
    _ops = new CDQM_options();

    set_ops_tab();


    //Set some window esthetics.
    int m=0;
    ui->centralWidget->setContentsMargins(m, m, m, m);
    ui->centralWidgetB->setContentsMargins(m, m, m, m);
    ui->centralWidgetB->layout()->setSpacing(0);




    if (_AutoUpdate) {
        QTimer *timer = new QTimer(this);
        connect(timer, SIGNAL(timeout()), this, SLOT(timer_update()));
        timer->start(6000);
    }
	ui->UpdatePauseBox->setEnabled(false);
	
}




//_____________________________________________________________________________

void MainWindow::timer_update(){
    std::cout<<"Updating..."<<std::endl;
    if (_DQM_ran && !ui->UpdatePauseBox->isChecked())
        on_append_but_clicked();
    else std::cout<<"Not appropriate to update."<<std::endl;
}


//_____________________________________________________________________________

void MainWindow::on_init_but_clicked() {
    //This button is also used as the restart button - so if DQM has already
    //ran, delete the current instance.

    ui->UpdatePauseBox->setChecked(true);
    if (_DQM_ran) {
        _DQM->finalize();
        delete _DQM;
    }
    if (_fileopen){
        _tfile->Close();
        _fileopen = false;
    }
    //Update the options.
    ops_apply();
    //Run the DQM, then open its file.
    _DQM = new CDQM(_ops);
    _DQM->initialize();
    double hr = (double)_DQMclock->currentDateTime().time().hour() +
                (double)_DQMclock->currentDateTime().time().minute()/60.0 +
                (double)_DQMclock->currentDateTime().time().second()/3600.0;
    _DQM->executeEventLoop(hr);
    _DQM_ran = true;
    _tfile = new TFile(_ops->save_file_name.c_str());
    _fileopen = true;
    //Set up the tabs with plots.
    fill_plot_tabs();

    //Esthetics.
    ui->append_but->setEnabled(true);
    ui->SamplingInfoBox->setEnabled(true);
    ui->plot_ops_box->setEnabled(true);
    ui->init_but->setText(QString("Restart"));
    ui->init_but->setEnabled(false);
    ui->pixel_tabs->setCurrentIndex(0);
    ui->DQM_all_tabs->setCurrentIndex(1);
    _nsample++;
    std::stringstream ss;
    ss << _nsample;
    ui->SampleNumLab->setText(QString(ss.str().c_str()));
    ui->SampleTimeLab->setText(_DQMclock->currentDateTime().toString());
}







//_____________________________________________________________________________

void MainWindow::on_load_but_clicked() {
    //This method simply needs to open a previously ran DQM file (not actually
    //run the DQM).


    //Check nothing is currently open.
    if (_fileopen){
        _tfile->Close();
        _fileopen = false;
    }

    QString fileName = QFileDialog::getOpenFileName(this,
        QString("Load DQM File"), QString(_ops->save_file_name.c_str()));

    if (fileName != NULL){
        _ops->save_file_name = fileName.toStdString();
        _ops->load_ops();

        std::cout<<"Loaded options:"<<std::endl;
        _ops->print_ops();


        //Open the old file (assume the same name as in _ops for now).
        _tfile = new TFile(_ops->save_file_name.c_str());
        _fileopen = true;


        //Set up the tabs with plots.
        fill_plot_tabs();


        //Esthetics.
        ui->pixel_tabs->setCurrentIndex(1);
        ui->plot_ops_box->setEnabled(true);
    }
    else std::cout<<"No file selected."<<std::endl;

}







//_____________________________________________________________________________

void MainWindow::on_append_but_clicked() {
    //Updates the DQM plots.


    //Close the root file, so plots can be replaced by update_DQM.
    _tfile->Close();
    _fileopen = false;


    //Do the update, then get new plots.
    double hr = (double)_DQMclock->currentDateTime().time().hour() +
                (double)_DQMclock->currentDateTime().time().minute()/60.0 +
                (double)_DQMclock->currentDateTime().time().second()/3600.0;
    _DQM->execute(hr);
    _tfile = new TFile(_ops->save_file_name.c_str());
    _fileopen = true;


    //Refill tabls.
    fill_plot_tabs();

    _nsample++;
    std::stringstream ss;
    ss << _nsample;
    std::string s = ss.str();
    ui->SampleNumLab->setText(QString(ss.str().c_str()));
    ui->SampleTimeLab->setText(_DQMclock->currentDateTime().toString());
}






//_____________________________________________________________________________

void MainWindow::fill_plot_tabs() {
    //Fill the tabs with the DQM plots - only those which have been made.
    //Empty all tabs first.
    empty_tab(ui->chip_view_tab);

    empty_tab(ui->hitmaps_tab);
    empty_tab(ui->z_dist_holder);
    empty_tab(ui->ADC_Distributions);
    empty_tab(ui->x_correlation_tabLocal);
    empty_tab(ui->y_correlation_tabLocal);
    empty_tab(ui->x_correlation_tabGlobal);
    empty_tab(ui->y_correlation_tabGlobal);
    empty_tab(ui->temporal_diff_tab);

    empty_tab(ui->ClustHitmapsLocalXY);
    empty_tab(ui->ClustHitmapsGlobalXY);
    empty_tab(ui->ClustHitmapsGlobalX);
    empty_tab(ui->ClustHitmapsGlobalY);

    empty_tab(ui->clust_z_dist_holder);
    empty_tab(ui->clust_ADC_Distributions);
    empty_tab(ui->clust_ADC_Distributions1);
    empty_tab(ui->clust_ADC_Distributions2);
    empty_tab(ui->clust_ADC_Distributions3);
    empty_tab(ui->clust_ADC_Distributions4);
    empty_tab(ui->clust_x_correlation_tabLocal);
    empty_tab(ui->clust_y_correlation_tabLocal);
    empty_tab(ui->clust_x_correlation_tabGlobal);
    empty_tab(ui->clust_y_correlation_tabGlobal);

    empty_tab(ui->x_diff_tabLocal);
    empty_tab(ui->y_diff_tabLocal);
    empty_tab(ui->x_diff_tabGlobal);
    empty_tab(ui->y_diff_tabGlobal);

    empty_tab(ui->clust_x_diff_tabLocal);
    empty_tab(ui->clust_y_diff_tabLocal);
    empty_tab(ui->clust_x_diff_tabGlobal);
    empty_tab(ui->clust_y_diff_tabGlobal);
    empty_tab(ui->ClustTDistsTab);

    empty_tab(ui->xResidualsVarsTab);
    empty_tab(ui->yResidualsVarsTab);
    empty_tab(ui->tResidualsVarsTab);

    empty_tab(ui->clust_samples_tab);
    empty_tab(ui->clust_size_tab);
    empty_tab(ui->clust_temporal_diff_tab);
    empty_tab(ui->XResidualsTab);
    empty_tab(ui->YResidualsTab);
    empty_tab(ui->AssociatedHitmapsTab);
    empty_tab(ui->NonAssociatedHitmapsTab);
    empty_tab(ui->TResidualsTab);
    empty_tab(ui->TResidualsTab);
    empty_tab(ui->GeneralTrackTab);

    _ADC_distribution_hists.clear();
    _temporal_differences_hists.clear();
    _hitmap_hists.clear();
    _xcorrel_hists.clear();
    _ycorrel_hists.clear();

    _clust_ADC_distribution_hists.clear();
    _clust_temporal_differences_hists.clear();
    _clust_hitmap_hists.clear();
    _clust_xcorrel_hists.clear();
    _clust_ycorrel_hists.clear();
    _clust_samples_hists.clear();
    _clust_size_dist_hists.clear();
    _XResidualsHists.clear();
    _YResidualsHists.clear();

    if (_ops->algorithms_contains(1)){
        fill_ADC_Distributions(-1);
        fill_z_distribution(0);
        fill_hitmaps(0);
    }


    if (_ops->algorithms_contains(2)) {
        fill_correlations(0, 0, true);
        fill_correlations(0, 0, false);
        fill_differences(0, 0, true);
        fill_differences(0, 0, false);
    }


    if (_ops->algorithms_contains(3)) {
        fill_correlations(1, 0, true);
        fill_correlations(1, 0, false);
        fill_differences(1, 0, true);
        fill_differences(1, 0, false);
    }


    if (_ops->algorithms_contains(4)) fill_temporal_differences(0);


    if (_ops->algorithms_contains(6)){
        fill_ADC_Distributions(0); // All clusters.
        fill_ADC_Distributions(1); // Single hit clusters, etc.
        fill_ADC_Distributions(2);
        fill_ADC_Distributions(3);
        fill_ADC_Distributions(4);
        fill_z_distribution(1);
        fill_hitmaps(1);
        fill_hitmaps(5);
        fill_hitmapsProj(0);
        fill_hitmapsProj(1);
        fill_hitmapsProj(2);
        fill_clust_sizes();
        fill_clust_samples();
    }


    if (_ops->algorithms_contains(7)) {
        fill_correlations(0, 1, true);
        fill_correlations(0, 1, false);
        fill_differences(0, 1, true);
        fill_differences(0, 1, false);
    }
    if (_ops->algorithms_contains(8)) {
        fill_correlations(1, 1, true);
        fill_correlations(1, 1, false);
        fill_differences(1, 1, true);
        fill_differences(1, 1, false);
    }
    if (_ops->algorithms_contains(9)) fill_temporal_differences(1);

    if (_ops->algorithms_contains(12)){
        fill_generalTrackPlots();
        fill_residuals(0);
        fill_residuals(1);
        fill_residuals(2);
        fill_hitmaps(2);
        fill_hitmaps(3);
    }

    if (_ops->algorithms_contains(15)) {
        fill_ResidualsVarsTab();
    }

    //fill_chip_views();
}





//_____________________________________________________________________________

void MainWindow::empty_tab(QWidget * my_tab) {
    QLayoutItem* item;
    if (my_tab->layout() != NULL)
    {
        while ((item = my_tab->layout()->takeAt(0)) != NULL){
            delete item->widget(); delete item;}
        delete my_tab->layout();
    }
}







//_____________________________________________________________________________

void MainWindow::fill_chip_views(){
    //Fills the chip view tabs with plots of each chip. Objects are remade,
    //as Qt doesn't like cloning objects or showing the same widget in two
    //places.


    //Make a set of tabs for the chip_view_tab and fill.
    QTabWidget * chip_tabs = new QTabWidget();
    for (int ichip = 0; ichip<_nchips; ichip++){
        std::string label;
        std::stringstream ss; ss<<ichip; label = ss.str();
        if (ichip == _ops->ref_chips[0]) label = label + " (ref)";

        QTabWidget * a_chip_view_tab = new QTabWidget();
        chip_tabs->addTab(a_chip_view_tab, QString(label.c_str()));

        QGridLayout * a_chip_view_tab_layout = new QGridLayout();
        a_chip_view_tab->setLayout(a_chip_view_tab_layout);

        QWidget * chip_pix_tab = new QWidget();
        QGridLayout * chip_pix_tab_layout = new QGridLayout();
        chip_pix_tab->setLayout(chip_pix_tab_layout);

        QWidget * chip_clust_tab = new QWidget();
        QGridLayout * chip_clust_tab_layout = new QGridLayout();
        chip_clust_tab->setLayout(chip_clust_tab_layout);

        a_chip_view_tab->addTab(chip_pix_tab, "Pixel Hits");
        a_chip_view_tab->addTab(chip_clust_tab, "Clusters");


        //Get the plot widgets.
        if (_ops->algorithms_contains(1)){
            hist_widget * ADC_widget = get_ADC_widget(ichip, 0);
            ADC_widget->toggle_axes_labels();
            chip_pix_tab_layout->addWidget(ADC_widget, 0, 0);

            twoDhist_widget * hitmap_widg = get_hitmap_widget(ichip, 0);
            hitmap_widg->toggle_axes_labels();
            chip_pix_tab_layout->addWidget(hitmap_widg, 0, 1);
        }

        if (_ops->algorithms_contains(2)){
            twoDhist_widget * x_correl_widg = get_correl_widget(ichip, 0, 0, false);
            x_correl_widg->toggle_axes_labels();
            chip_pix_tab_layout->addWidget(x_correl_widg, 1, 0);
        }

        if (_ops->algorithms_contains(3)){
            twoDhist_widget * y_correl_widg = get_correl_widget(ichip, 1, 0, false);
            y_correl_widg->toggle_axes_labels();
            chip_pix_tab_layout->addWidget(y_correl_widg, 1, 1);
        }

        if (_ops->algorithms_contains(4)){
            hist_widget * temp_diff_widget = get_temp_diff_widget(ichip, 0);
            temp_diff_widget->toggle_axes_labels();
            chip_pix_tab_layout->addWidget(temp_diff_widget, 2, 2);
        }

         if (_ops->algorithms_contains(6)){
            hist_widget * ADC_widget = get_ADC_widget(ichip, 1);

            //ADC_widget->_Qh->plotLayout()->insertRow(0);
            //ADC_widget->_Qh->plotLayout()->addElement(0, 0, new QCPPlotTitle(ADC_widget->_Qh, "ADC Distribution"));
            ADC_widget->toggle_axes_labels();
            chip_clust_tab_layout->addWidget(ADC_widget, 0, 0);
            ADC_widget->_Qh->graph(0)->setBrush(QBrush(QColor(255,0,0,100)));
            ADC_widget->_Qh->graph(0)->setPen(QPen(Qt::black));

            hist_widget * Size2_ADC_widget = get_ADC_widget(ichip, 2);
            //ADC_widget->_Qh->plotLayout()->insertRow(0);
            //ADC_widget->_Qh->plotLayout()->addElement(0, 0, new QCPPlotTitle(ADC_widget->_Qh, "ADC Distribution"));
            Size2_ADC_widget->toggle_axes_labels();
            chip_clust_tab_layout->addWidget(Size2_ADC_widget, 0, 1);
            Size2_ADC_widget->_Qh->graph(0)->setBrush(QBrush(QColor(255,0,0,100)));
            Size2_ADC_widget->_Qh->graph(0)->setPen(QPen(Qt::black));

            hist_widget * size_widget = get_cluster_size_widget(ichip);
            //ADC_widget->_Qh->plotLayout()->insertRow(0);
            //ADC_widget->_Qh->plotLayout()->addElement(0, 0, new QCPPlotTitle(ADC_widget->_Qh, "ADC Distribution"));
            size_widget->toggle_axes_labels();
            chip_clust_tab_layout->addWidget(size_widget, 0, 2);
            size_widget->_Qh->graph(0)->setBrush(QBrush(QColor(255,0,0,100)));
            size_widget->_Qh->graph(0)->setPen(QPen(Qt::black));

            twoDhist_widget * hitmap_widg = get_hitmap_widget(ichip, 1);
            hitmap_widg->toggle_axes_labels();
            chip_clust_tab_layout->addWidget(hitmap_widg, 1, 0);

            twoDhist_widget * sample_widg = get_cluster_samples_widget(ichip);
            //hitmap_widg->_Qh->plotLayout()->insertRow(0);
            //hitmap_widg->_Qh->plotLayout()->addElement(0, 0, new QCPPlotTitle(hitmap_widg->_Qh, "Hitmap"));
            sample_widg->toggle_axes_labels();
            chip_clust_tab_layout->addWidget(sample_widg, 1, 1);
        }

         if (_ops->algorithms_contains(7)){
            twoDhist_widget * x_correl_widg = get_correl_widget(ichip, 0, 1, false);
            //x_correl_widg->_Qh->plotLayout()->insertRow(0);
            //x_correl_widg->_Qh->plotLayout()->addElement(0, 0, new QCPPlotTitle(x_correl_widg->_Qh, "X Correlations"));
            x_correl_widg->toggle_axes_labels();
            chip_clust_tab_layout->addWidget(x_correl_widg, 0, 2);
         }

         if (_ops->algorithms_contains(8)){
            twoDhist_widget * y_correl_widg = get_correl_widget(ichip, 1, 1, false);
            //y_correl_widg->_Qh->plotLayout()->insertRow(0);
            //y_correl_widg->_Qh->plotLayout()->addElement(0, 0, new QCPPlotTitle(y_correl_widg->_Qh, "Y Correlations"));
            y_correl_widg->toggle_axes_labels();
            chip_clust_tab_layout->addWidget(y_correl_widg, 2, 0);
         }

         if (_ops->algorithms_contains(9)){
            hist_widget * temp_diff_widget = get_temp_diff_widget(ichip, 1);
                        //temp_diff_widget->_Qh->plotLayout()->insertRow(0);
                        //temp_diff_widget->_Qh->plotLayout()->addElement(0, 0, new QCPPlotTitle(temp_diff_widget->_Qh, "Temporal Differences"));
            temp_diff_widget->toggle_axes_labels();
            chip_clust_tab_layout->addWidget(temp_diff_widget, 2, 1);
         }

        //Add the new tab with an apporpriate name.

    }


    //Add the new set of tabs.
    QGridLayout * chip_view_layout = new QGridLayout();
    chip_view_layout->addWidget(chip_tabs);


    //Esthetics.
    int m = 7;
    chip_view_layout->setContentsMargins(m, m, m, m);
    ui->chip_view_tab->setLayout(chip_view_layout);
}







//_____________________________________________________________________________

void MainWindow::fill_ResidualsVarsTab(){
    QGridLayout * lx = new QGridLayout();
    QGridLayout * ly = new QGridLayout();
    QGridLayout * lt = new QGridLayout();
    for (int ichip = 0; ichip<_nchips; ichip++){
        hist_widget * wx = get_residual_var(ichip, 0);
        hist_widget * wy = get_residual_var(ichip, 1);
        hist_widget * wt = get_residual_var(ichip, 2);
        lx->addWidget(wx,(int)ichip/3.0, ichip%3);
        ly->addWidget(wy, (int)ichip/3.0, ichip%3);
        lt->addWidget(wt, (int)ichip/3.0, ichip%3);

        wx->_Qh->yAxis->setRange(-2.0, 2.0);
        wy->_Qh->yAxis->setRange(-2.0, 2.0);
        wt->_Qh->yAxis->setRange(-2.0, 2.0);
    }
    ui->xResidualsVarsTab->setLayout(lx);
    ui->yResidualsVarsTab->setLayout(ly);
    ui->tResidualsVarsTab->setLayout(lt);


    //Esthetics.
    int m = 0;
    lx->setSpacing(0);
    lx->setContentsMargins(m, m, m, m);
    ly->setSpacing(0);
    ly->setContentsMargins(m, m, m, m);
    lt->setSpacing(0);
    lt->setContentsMargins(m, m, m, m);
}




//_____________________________________________________________________________

void MainWindow::fill_hitmapsProj(int kind){

    //Create the layout and fill with plot widgets.
    QGridLayout * hitmap_layout = new QGridLayout();
    for (int ichip = 0; ichip<_nchips; ichip++){
        hist_widget * hitmap_widg = get_hitmapProj_widget(ichip, kind);
        hitmap_layout->addWidget(hitmap_widg,(int)ichip/3.0, ichip%3);
    }
    if (kind == 0) ui->ClustHitmapsGlobalX->setLayout(hitmap_layout);
    else if (kind==2) ui->ClustTDistsTab->setLayout(hitmap_layout);
    else ui->ClustHitmapsGlobalY->setLayout(hitmap_layout);

    //Esthetics.
    int m = 0;
    hitmap_layout->setSpacing(0);
    hitmap_layout->setContentsMargins(m, m, m, m);
}



//_____________________________________________________________________________

void MainWindow::fill_hitmaps(int kind){

    //Create the layout and fill with plot widgets.
    QGridLayout * hitmap_layout = new QGridLayout();
    for (int ichip = 0; ichip<_nchips; ichip++){
        twoDhist_widget * hitmap_widg = get_hitmap_widget(ichip, kind);
        hitmap_widg->_Qh->xAxis->setScaleRatio(hitmap_widg->_Qh->yAxis, 1.0);
        hitmap_layout->addWidget(hitmap_widg,(int)ichip/3.0, ichip%3);
    }
    if (kind == 0) ui->hitmaps_tab->setLayout(hitmap_layout);
    else if (kind ==1) ui->ClustHitmapsLocalXY->setLayout(hitmap_layout);
    else if (kind ==2) ui->AssociatedHitmapsTab->setLayout(hitmap_layout);
    else if (kind ==3) ui->NonAssociatedHitmapsTab->setLayout(hitmap_layout);
    else if (kind ==5) ui->ClustHitmapsGlobalXY->setLayout(hitmap_layout);
    else std::cout<<"wtf"<<std::endl;


    //Esthetics.
    int m = 0;
    hitmap_layout->setSpacing(0);
    hitmap_layout->setContentsMargins(m, m, m, m);
}







//_____________________________________________________________________________

void MainWindow::fill_correlations(int dir, int kind, bool locVsGlob){

    //Decide on the directory and tab to fill.
    QWidget * correl_tab;

    if (locVsGlob) {
        if (dir == 0) {
            if (kind == 0) correl_tab = ui->x_correlation_tabLocal;
            else correl_tab = ui->clust_x_correlation_tabLocal;
        }

        else if (dir ==1) {
            if (kind == 0) correl_tab = ui->y_correlation_tabLocal;
            else correl_tab = ui->clust_y_correlation_tabLocal;
        }
    }
    else {
        if (dir == 0) {
            if (kind == 0) correl_tab = ui->x_correlation_tabGlobal;
            else correl_tab = ui->clust_x_correlation_tabGlobal;
        }

        else if (dir ==1) {
            if (kind == 0) correl_tab = ui->y_correlation_tabGlobal;
            else correl_tab = ui->clust_y_correlation_tabGlobal;
        }
    }



    //Make the new layout and set.
    QGridLayout * correl_layout = new QGridLayout();
    for (int ichip = 0; ichip<_nchips; ichip++){
        twoDhist_widget * correl_widget = get_correl_widget(ichip, dir, kind, locVsGlob);
        correl_layout->addWidget(correl_widget,(int)ichip/3.0, ichip%3);
    }
    correl_tab->setLayout(correl_layout);


    //Esthetics.
    int m = 0;
    correl_layout->setSpacing(0);
    correl_layout->setContentsMargins(m, m, m, m);
}






//_____________________________________________________________________________

void MainWindow::fill_differences(int dir, int kind, bool locVsGlob){

    //Decide on the directory and tab to fill.
    QWidget * tab;

    if (locVsGlob) {
        if (dir == 0) {
            if (kind == 1) tab = ui->clust_x_diff_tabLocal;
            else tab = ui->x_diff_tabLocal;
        }

        else if (dir ==1) {
            if (kind == 1) tab = ui->clust_y_diff_tabLocal;
            else tab = ui->y_diff_tabLocal;
        }
    }
    else {
        if (dir == 0) {
            if (kind == 1) tab = ui->clust_x_diff_tabGlobal;
            else tab = ui->x_diff_tabGlobal;
        }

        else if (dir ==1) {
            if (kind == 1) tab = ui->clust_y_diff_tabGlobal;
            else tab = ui->y_diff_tabGlobal;
        }
    }



    //Make the new layout and set.
    QGridLayout * l = new QGridLayout();
    for (int ichip = 0; ichip<_nchips; ichip++){
        hist_widget * w = get_diff_widget(ichip, dir, kind, locVsGlob);
        l->addWidget(w,(int)ichip/3.0, ichip%3);

        if (kind ==0) w->_Qh->graph(0)->setBrush(QBrush(QColor(0,0,255,100)));
        else w->_Qh->graph(0)->setBrush(QBrush(QColor(255,0,0,100)));
    }
    tab->setLayout(l);


    //Esthetics.
    int m = 0;
    l->setSpacing(0);
    l->setContentsMargins(m, m, m, m);
}







//_____________________________________________________________________________

void MainWindow::fill_z_distribution(int kind){
    //Place a new layout containing the plot.
    QGridLayout * z_layout = new QGridLayout();
    QCustomPlot * z_widget = get_z_dist_widget(kind);


    if (_liveVsSaved && kind == 1) {
        z_layout->addWidget(z_widget, 0, 0);
        twoDhist_widget * xz_prof = new twoDhist_widget(_DQM->_cluster_plots->_zVsX_distribution, 0, _sel_Q, _sel_ichip, _sel_widg,
               _sel_twoDhist, ui->plot_ops_statsbox,  ui->clicked_x_lab,
               ui->clicked_y_lab, ui->clicked_z_lab, ui->logx_chbox, ui->logy_chbox,
               ui->logz_chbox, ui->plot_name_box, ui->fit_result_box, ui->clicked_pos_lab, NULL);

        twoDhist_widget * yz_prof = new twoDhist_widget(_DQM->_cluster_plots->_zVsY_distribution, 0, _sel_Q, _sel_ichip, _sel_widg,
               _sel_twoDhist, ui->plot_ops_statsbox,  ui->clicked_x_lab,
               ui->clicked_y_lab, ui->clicked_z_lab, ui->logx_chbox, ui->logy_chbox,
               ui->logz_chbox, ui->plot_name_box, ui->fit_result_box, ui->clicked_pos_lab, NULL);

        //Auto set log scale.
        xz_prof->_colormap->colorScale()->setDataScaleType(QCPAxis::stLogarithmic);
        xz_prof->_logged_z = true;

        yz_prof->_colormap->colorScale()->setDataScaleType(QCPAxis::stLogarithmic);
        yz_prof->_logged_z = true;

        xz_prof->_x_label = QString("z (mm)");
        xz_prof->_y_label = QString("x (mm)");
        xz_prof->_z_label = QString("N");

        yz_prof->_x_label = QString("z (mm)");
        yz_prof->_y_label = QString("y (mm)");
        yz_prof->_z_label = QString("N");

        z_layout->addWidget(xz_prof, 1, 0);
        z_layout->addWidget(yz_prof, 2, 0);
    }

    else z_layout->addWidget(z_widget);


    if (kind == 0) ui->z_dist_holder->setLayout(z_layout);
    else ui->clust_z_dist_holder->setLayout(z_layout);
}







//_____________________________________________________________________________

void MainWindow::fill_ADC_Distributions(int kind){
    //Should be ran before all other tab fill options (to set _nchips).

    //Start by creating and assigning the layout to contain the plots.
    QGridLayout * ADC_layout = new QGridLayout();
    if (kind == -1) ui->ADC_Distributions->setLayout(ADC_layout);
    else if (kind == 0)ui->clust_ADC_Distributions->setLayout(ADC_layout);
    else if (kind == 1)ui->clust_ADC_Distributions1->setLayout(ADC_layout);
    else if (kind == 2)ui->clust_ADC_Distributions2->setLayout(ADC_layout);
    else if (kind == 3)ui->clust_ADC_Distributions3->setLayout(ADC_layout);
    else if (kind == 4)ui->clust_ADC_Distributions4->setLayout(ADC_layout);


    //Cycle over chips.
    _nchips = _ops->nchips;
    for (int ichip = 0; ichip<_nchips; ichip++){

        hist_widget * ADC_widget = get_ADC_widget(ichip, kind);
        ADC_layout->addWidget(ADC_widget,(int)ichip/3.0, ichip%3);


        if (kind == -1) ADC_widget->_Qh->graph(0)->setBrush(QBrush(QColor(0,0,255,100))); //blue fill.
        else ADC_widget->_Qh->graph(0)->setBrush(QBrush(QColor(255,0,0,100)));


        //ADC_widget->_Qh->replot();
            //red fill.

        if (ichip == 0 && kind == -1) { //make the first one selected.
            _sel_widg = ADC_widget->_plot_frame;
            _sel_ichip = ichip;
            _sel_widg->setStyleSheet("background-color:red;");
            _sel_Q = ADC_widget->_Qh;
            ADC_widget->make_selected();
        }
    }


    //Some tab esthetics.
    int m = 0;
    ADC_layout->setSpacing(0);
    ADC_layout->setContentsMargins(m, m, m, m);
}




//_____________________________________________________________________________

void MainWindow::fill_generalTrackPlots(){

    //Start by creating and assigning the layout to contain the plots.
    QGridLayout * l = new QGridLayout();
    ui->GeneralTrackTab->setLayout(l);


    twoDhist_widget * hitmap = get_hitmap_widget(-1, 4);
    l->addWidget(hitmap, 1, 0);
    hitmap->toggle_axes_labels();

    hist_widget * sizes = get_track_size_widget();
    l->addWidget(sizes, 1, 1);
    sizes->toggle_axes_labels();

    sizes->_Qh->graph()->setBrush(QBrush(QColor(0,180,0,255))); //green fill.
    sizes->_Qh->graph()->setPen(QPen(Qt::black));



    //Some tab esthetics.
    int m = 0;
    l->setSpacing(0);
    l->setContentsMargins(m, m, m, m);
}





//_____________________________________________________________________________

void MainWindow::fill_clust_samples(){

    //Start by creating and assigning the layout to contain the plots.
    QGridLayout * cluster_samples_layout = new QGridLayout();
    ui->clust_samples_tab->setLayout(cluster_samples_layout);


    //Cycle over chips.
    for (int ichip = 0; ichip<_nchips; ichip++){
        twoDhist_widget * cluster_samples_widget = get_cluster_samples_widget(ichip);
        cluster_samples_layout->addWidget(cluster_samples_widget,(int)ichip/3.0, ichip%3);
    }


    //Some tab esthetics.
    int m = 0;
    cluster_samples_layout->setSpacing(0);
    cluster_samples_layout->setContentsMargins(m, m, m, m);
}







//_____________________________________________________________________________

void MainWindow::fill_temporal_differences(int kind){

    //Start by creating and assigning the layout to contain the plots.
    QGridLayout * temp_diff_layout = new QGridLayout();
    if (kind == 0) ui->temporal_diff_tab->setLayout(temp_diff_layout);
    else ui->clust_temporal_diff_tab->setLayout(temp_diff_layout);


    //Cycle over chips.
    for (int ichip = 0; ichip<_nchips; ichip++){
        hist_widget * temp_diff_widget = get_temp_diff_widget(ichip, kind);
        if (kind == 0) temp_diff_widget->_Qh->graph()->setBrush(QBrush(QColor(0,0,255,100))); //blue fill.
        else {
            temp_diff_widget->_Qh->graph()->setBrush(QBrush(QColor(255,0,0,100))); //red fill.
            temp_diff_widget->_Qh->graph()->setPen(QPen(Qt::black));
        }
        temp_diff_layout->addWidget(temp_diff_widget,(int)ichip/3.0, ichip%3);
    }


    //Some tab esthetics.
    int m = 0;
    temp_diff_layout->setSpacing(0);
    temp_diff_layout->setContentsMargins(m, m, m, m);
}






//_____________________________________________________________________________

void MainWindow::fill_residuals(int dir){

    //Start by creating and assigning the layout to contain the plots.
    QGridLayout * residuals_layout = new QGridLayout();
    if (dir == 0) ui->XResidualsTab->setLayout(residuals_layout);
    if (dir == 1) ui->YResidualsTab->setLayout(residuals_layout);
    if (dir == 2) ui->TResidualsTab->setLayout(residuals_layout);


    //Cycle over chips.
    for (int ichip = 0; ichip<_nchips; ichip++){
        hist_widget * residual_widget = get_residual(ichip, dir);
        residual_widget->_Qh->graph()->setBrush(QBrush(QColor(0,180,0,255))); //green fill.
        residual_widget->_Qh->graph()->setPen(QPen(Qt::black));
        if (ui->FitResidualsBox->isChecked()) residual_widget->fit_gaussian(Qt::blue);
        residuals_layout->addWidget(residual_widget,(int)ichip/3.0, ichip%3);
    }


    //Some tab esthetics.
    int m = 0;
    residuals_layout->setSpacing(0);
    residuals_layout->setContentsMargins(m, m, m, m);
}








//_____________________________________________________________________________

void MainWindow::fill_clust_sizes(){

    //Start by creating and assigning the layout to contain the plots.
    QGridLayout * cluster_size_layout = new QGridLayout();
    ui->clust_size_tab->setLayout(cluster_size_layout);


    //Cycle over chips.
    for (int ichip = 0; ichip<_nchips; ichip++){
        hist_widget * cluster_size_widget = get_cluster_size_widget(ichip);
        cluster_size_widget->_Qh->graph()->setBrush(QBrush(QColor(255,0,0,100))); //red fill.
        cluster_size_widget->_Qh->graph()->setPen(QPen(Qt::black)); //red fill.
        cluster_size_layout->addWidget(cluster_size_widget,(int)ichip/3.0, ichip%3);
    }


    //Some tab esthetics.
    int m = 0;
    cluster_size_layout->setSpacing(0);
    cluster_size_layout->setContentsMargins(m, m, m, m);
}







//_____________________________________________________________________________

QCustomPlot * MainWindow::get_z_dist_widget(int kind) {
    //Returns a QCusomtPlot* showing the z_distribution TH1F.


    //Get the TH1F.
    std::string direc;
    if (kind == 0) direc = "Pixel_plots/Pix_Z_Dist";
    else direc = "Cluster_plots/Clust_Z_Dist";
    TH1F* h;
    if (!_liveVsSaved) h = (TH1F*) _tfile->Get(direc.c_str());
    else {
        if (kind == 0) h = _DQM->_pixel_plots->_z_distribution;
        if (kind == 1) h = _DQM->_cluster_plots->_z_distribution;
    }


    //Some variables for the conversion.
    int Nbins = h->GetNbinsX();
    QVector<double> x(Nbins-2), y(Nbins-2); //ignore flows here.
    float min_y = y[0]; 
    float max_y = y[0];


    //Cycle over bins, getting their contents.
    for (int i=1; i<Nbins-1; ++i){
        int ibin = h->GetBin(i);
        x[i-1] = h->GetBinCenter(ibin);
        y[i-1] = h->GetBinContent(ibin);

        //Just for y range.
        if (y[i-1] < min_y) min_y = y[i-1];
        if (y[i-1] > max_y) max_y = y[i-1];
    }


    //Create graph and assign data to it.
    QCustomPlot * Qh = new QCustomPlot();
    Qh->addGraph();
    Qh->graph()->setLineStyle(QCPGraph::lsStepCenter); //points are bin centers.
    Qh->graph()->setData(x, y);


    //Set axis range.
    Qh->yAxis->setRange(min_y, max_y);
    Qh->xAxis->setRange(h->GetXaxis()->GetXmin(), h->GetXaxis()->GetXmax());


    //Some plot esthetics.
    Qh->axisRect()->setupFullAxesBox();
    if (kind == 0) Qh->graph()->setBrush(QBrush(QColor(0,0,255,100))); //blue fill.
    else {
        Qh->graph()->setBrush(QBrush(QColor(255,0,0,100))); //red fill.
    }

    Qh->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectAxes |
                              QCP::iSelectLegend);

    Qh->replot();

    Qh->xAxis->setLabel(QString("z (mm)"));
    Qh->yAxis->setLabel(QString("N"));

    return Qh;
}







//_____________________________________________________________________________

hist_widget * MainWindow::get_residual(int ichip, int dir) {

    //Get the TH1F (naming convention in plot classes must be consistent).
    std::stringstream ss;
    ss<<ichip;
    std::string direc;
    if (dir == 0) direc = "Track_plots/xResiduals/Chip_" + ss.str();
    else if (dir == 1) direc = "Track_plots/yResiduals/Chip_" + ss.str();
    else direc = "Track_plots/tResiduals/Chip_" + ss.str();
    TH1F* h;

    if (!_liveVsSaved) h = (TH1F*) _tfile->Get(direc.c_str());
    else {
        if (dir == 0) h = _DQM->_track_plots->_xresiduals[ichip];
        else if (dir == 1) h = _DQM->_track_plots->_yresiduals[ichip];
        else h = _DQM->_track_plots->_tresiduals[ichip];
    }

    hist_widget * h_widg = new hist_widget(h, ichip, _sel_Q, _sel_ichip, _sel_widg, _sel_hist,
             ui->plot_ops_statsbox, ui->clicked_x_lab, ui->clicked_y_lab, ui->clicked_z_lab,
             ui->logx_chbox, ui->logy_chbox, ui->logz_chbox, ui->plot_name_box,
             ui->fit_result_box, ui->clicked_pos_lab);


    if (dir == 0) h_widg->_x_label = QString("delta x (mm)");
    if (dir == 1) h_widg->_x_label = QString("delta y (mm)");
    if (dir == 2) h_widg->_x_label = QString("delta t (25/16 ns)");
    h_widg->_y_label = QString("N");

    if (dir == 0) _XResidualsHists.push_back(h_widg);
    else if (dir == 1) _YResidualsHists.push_back(h_widg);
    else _TResidualsHists.push_back(h_widg);

    h_widg->show_axes_labels();
    return h_widg;
}







//_____________________________________________________________________________

hist_widget * MainWindow::get_temp_diff_widget(int ichip, int kind) {

    //Get the TH1F (naming convention in plot classes must be consistent).
    std::stringstream ss, ss_ref;
    ss_ref<<_ops->ref_chips[0];
    ss<<ichip;
    std::string direc;
    if (kind == 0) direc = "t_pixs_correlations/Differences/Chip_" + ss.str();
    else direc = "t_clusts_correlations/Differences/Chip_" + ss.str();

    TH1F* h;
    if (!_liveVsSaved) h = (TH1F*) _tfile->Get(direc.c_str());
    else {
        if (kind == 0) h = _DQM->_t_pix_correlations->_differencesGlobal[ichip];
        else h = _DQM->_t_clust_correlations->_differencesGlobal[ichip];
    }



    hist_widget * h_widg = new hist_widget(h, ichip, _sel_Q, _sel_ichip, _sel_widg, _sel_hist,
             ui->plot_ops_statsbox, ui->clicked_x_lab, ui->clicked_y_lab, ui->clicked_z_lab,
             ui->logx_chbox, ui->logy_chbox, ui->logz_chbox, ui->plot_name_box,
             ui->fit_result_box, ui->clicked_pos_lab);


    h_widg->_x_label = QString("t_chip - t_ref (25/16 ns)");
    h_widg->_y_label = QString("N");

    if (kind == 0) _temporal_differences_hists.push_back(h_widg);
    else _clust_temporal_differences_hists.push_back(h_widg);

    return h_widg;
}








//_____________________________________________________________________________

hist_widget * MainWindow::get_ADC_widget(int ichip, int kind) {
    //Returns the a hist_widget* containing a QCustomPlot version of an ADC
    //distribution, which was originally a TH1F, for the chip ID ichip.


    //Get the TH1F (naming convention in plot classes must be consistent).
    std::stringstream ss, ss_kind;
    ss<<ichip;
    ss_kind<<kind;
    std::string direc;
    if (kind == -1) direc = "Pixel_plots/ToT_dists/Chip_" + ss.str();
    else if (kind == 0) direc = "Cluster_plots/ToT_dists/ToT_AllSizes/Chip_" + ss.str();
    else direc = "Cluster_plots/ToT_dists/ToT_Size" + ss_kind.str() + "/Chip_" + ss.str();


    TH1F* h;
    if (!_liveVsSaved) h = (TH1F*) _tfile->Get(direc.c_str());
    else {
        if (kind == -1) h = _DQM->_pixel_plots->_ADC_dists[ichip];
        else if (kind == 0) h = _DQM->_cluster_plots->_ToT_dists[ichip];
        else if (kind == 1) h = _DQM->_cluster_plots->_ToT_Size1_dists[ichip];
        else if (kind == 2) h = _DQM->_cluster_plots->_ToT_Size2_dists[ichip];
        else if (kind == 3) h = _DQM->_cluster_plots->_ToT_Size3_dists[ichip];
        else h = _DQM->_cluster_plots->_ToT_Size4_dists[ichip];
    }

    TH1F * ref_h = NULL;
    if (_include_references){
        TFile * ref_file = new TFile(_ref_file.c_str());
        ref_h = (TH1F*) ref_file->Get(direc.c_str());
    }


    hist_widget * h_widg = new hist_widget(h, ichip, _sel_Q, _sel_ichip, _sel_widg, _sel_hist,
             ui->plot_ops_statsbox, ui->clicked_x_lab, ui->clicked_y_lab, ui->clicked_z_lab,
             ui->logx_chbox, ui->logy_chbox, ui->logz_chbox, ui->plot_name_box,
             ui->fit_result_box, ui->clicked_pos_lab, ref_h);

    //h_widg->add_mean_line();
    if (kind == -1 && ui->PixFitLandauBox->isChecked()) h_widg->fit_landau(Qt::red);
    if (kind == 0 && ui->ClustFitLandauBox->isChecked()) h_widg->fit_landau(Qt::blue);
    if (kind == 1 && ui->ClustFitLandauBox->isChecked()) h_widg->fit_landau(Qt::blue);
    if (kind == 2 && ui->ClustFitLandauBox->isChecked()) h_widg->fit_landau(Qt::blue);
    if (kind == 3 && ui->ClustFitLandauBox->isChecked()) h_widg->fit_landau(Qt::blue);
    if (kind == 4 && ui->ClustFitLandauBox->isChecked()) h_widg->fit_landau(Qt::blue);

    h_widg->_x_label = QString("ToT");
    h_widg->_y_label = QString("N");


    if (kind == -1) _ADC_distribution_hists.push_back(h_widg);
    else if (kind == 0) _clust_ADC_distribution_hists.push_back(h_widg);
    else if (kind == 1) _clust_ADC_distribution_hists1.push_back(h_widg);
    else if (kind == 2) _clust_ADC_distribution_hists2.push_back(h_widg);
    else if (kind == 3) _clust_ADC_distribution_hists3.push_back(h_widg);
    else if (kind == 4) _clust_ADC_distribution_hists4.push_back(h_widg);

    h_widg->show_axes_labels();
    return h_widg;
}








//_____________________________________________________________________________

hist_widget * MainWindow::get_track_size_widget() {
    //Returns the a hist_widget* containing a QCustomPlot version of an ADC
    //distribution, which was originally a TH1F, for the chip ID ichip.


    //Get the TH1F (naming convention in plot classes must be consistent).
    std::stringstream ss;
    std::string direc = "Track_plots/size_dist";
    TH1F* h;
    if (!_liveVsSaved) h = (TH1F*) _tfile->Get(direc.c_str());
    else h=_DQM->_track_plots->_size_dist;

    int ichip = 0;

    hist_widget * h_widg = new hist_widget(h, ichip, _sel_Q, _sel_ichip, _sel_widg, _sel_hist,
             ui->plot_ops_statsbox, ui->clicked_x_lab, ui->clicked_y_lab, ui->clicked_z_lab,
             ui->logx_chbox, ui->logy_chbox, ui->logz_chbox, ui->plot_name_box,
             ui->fit_result_box, ui->clicked_pos_lab);

    h_widg->_x_label = QString("Track size");
    h_widg->_y_label = QString("N");

    _clust_size_dist_hists.push_back(h_widg);

    return h_widg;
}



//_____________________________________________________________________________

hist_widget * MainWindow::get_residual_var(int ichip, int idir) {
    TFile* save_file = new TFile(_ops->save_file_nameLongScale.c_str());
    std::stringstream ss;
    ss<<ichip;
    std::string DirecName;
    if (idir == 0) DirecName = "xResidualsVsTime";
    else if (idir == 1) DirecName = "yResidualsVsTime";
    else DirecName = "tResidualsVsTime";

    std::string direc = DirecName + "/Chip_" + ss.str();
    //std::cout<<direc<<std::endl;
    TGraph * g = (TGraph*) save_file->Get(direc.c_str());
    save_file->Close();
    hist_widget * h_widg = new hist_widget(g, ichip, _sel_Q, _sel_ichip, _sel_widg, _sel_hist,
             ui->plot_ops_statsbox, ui->clicked_x_lab, ui->clicked_y_lab, ui->clicked_z_lab,
             ui->logx_chbox, ui->logy_chbox, ui->logz_chbox, ui->plot_name_box,
             ui->fit_result_box, ui->clicked_pos_lab);

    return h_widg;
}





//_____________________________________________________________________________

hist_widget * MainWindow::get_cluster_size_widget(int ichip) {
    //Returns the a hist_widget* containing a QCustomPlot version of an ADC
    //distribution, which was originally a TH1F, for the chip ID ichip.


    //Get the TH1F (naming convention in plot classes must be consistent).
    std::stringstream ss; 
    ss<<ichip;
    std::string direc = "Cluster_plots/size_dists/Chip_" + ss.str();
    TH1F* h;
    if (!_liveVsSaved) h = (TH1F*) _tfile->Get(direc.c_str());
    else h = _DQM->_cluster_plots->_size_dists[ichip];


    hist_widget * h_widg = new hist_widget(h, ichip, _sel_Q, _sel_ichip, _sel_widg, _sel_hist,
             ui->plot_ops_statsbox, ui->clicked_x_lab, ui->clicked_y_lab, ui->clicked_z_lab,
             ui->logx_chbox, ui->logy_chbox, ui->logz_chbox, ui->plot_name_box,
             ui->fit_result_box, ui->clicked_pos_lab);

    h_widg->_x_label = QString("Cluster size");
    h_widg->_y_label = QString("N");

    _clust_size_dist_hists.push_back(h_widg);
    h_widg->show_axes_labels();
    return h_widg;
}








//_____________________________________________________________________________

twoDhist_widget * MainWindow::get_correl_widget(int ichip, int idir, int kind, bool locVsGlob) {
    //Returns the a twoD_widget* containing a QCustomPlot version of an correlation
    //plot, which was originally a TH2F, for the chip ID ichip in direction idir.
    bool bgless = false;


    //Get the plot (naming convention in plot classes must be consistent).
    std::stringstream ss_ichip, ss_irefchip;
    ss_ichip<<ichip;
    ss_irefchip<<_ops->ref_chips[0];
    std::string th2_direc, fit_result_direc, localOrGlobal;

    if (locVsGlob) localOrGlobal = "Local";
    else localOrGlobal = "Global";

    //Decide on th2_direc.
    if (kind == 0){
        if (bgless){
            if (idir == 0) {
                th2_direc = "/x_pixs_correlations/" + localOrGlobal + "/Correlations_minus_backgrounds/Chip_" + ss_ichip.str() + localOrGlobal;
            }
            else if (idir == 1) {
                th2_direc = "/y_pixs_correlations/" + localOrGlobal + "/Correlations_minus_backgrounds/Chip_" + ss_ichip.str() + localOrGlobal;
            }
        }

        else {
            if (idir == 0) {
                th2_direc = "/x_pixs_correlations/" + localOrGlobal + "/Correlations/Chip_" + ss_ichip.str() + localOrGlobal;
            }
            else if (idir == 1) {
                th2_direc = "/y_pixs_correlations/" + localOrGlobal + "/Correlations/Chip_" + ss_ichip.str() + localOrGlobal;
            }
        }
    }

    else {
        if (bgless){
            if (idir == 0) {
                th2_direc = "/x_clusts_correlations/" + localOrGlobal + "/Correlations_minus_backgrounds/Chip_" + ss_ichip.str() + localOrGlobal;
            }
            else if (idir == 1) {
                th2_direc = "/y_clusts_correlations/" + localOrGlobal + "/Correlations_minus_backgrounds/Chip_" + ss_ichip.str() + localOrGlobal;
            }
        }

        else {
            if (idir == 0) {
                th2_direc = "/x_clusts_correlations/" + localOrGlobal + "/Correlations/Chip_" + ss_ichip.str() + localOrGlobal;
            }
            else if (idir == 1) {
                th2_direc = "/y_clusts_correlations/" + localOrGlobal + "/Correlations/Chip_" + ss_ichip.str() + localOrGlobal;
            }
        }
    }

    TH1F * h_fit_results = NULL;
    TH2F * h_correl;
    if (!_liveVsSaved) h_correl = (TH2F*) _tfile->Get(th2_direc.c_str());
    else {
        if (kind == 0) {
            if (idir == 0) {
                if (localOrGlobal == "Local") h_correl = _DQM->_x_pix_correlations->_correlationsLocal[ichip];
                else h_correl = _DQM->_x_pix_correlations->_correlationsGlobal[ichip];
            }
            else {
                if (localOrGlobal == "Local") h_correl = _DQM->_y_pix_correlations->_correlationsLocal[ichip];
                else h_correl = _DQM->_y_pix_correlations->_correlationsGlobal[ichip];
            }

        }
        else {
            if (idir == 0) {
                if (localOrGlobal == "Local") h_correl = _DQM->_x_clust_correlations->_correlationsLocal[ichip];
                else h_correl = _DQM->_x_clust_correlations->_correlationsGlobal[ichip];
            }
            else {
                if (localOrGlobal == "Local") h_correl = _DQM->_y_clust_correlations->_correlationsLocal[ichip];
                else h_correl = _DQM->_y_clust_correlations->_correlationsGlobal[ichip];
            }
        }
    }


    twoDhist_widget * my_hist_widg = new twoDhist_widget(h_correl, ichip, _sel_Q, _sel_ichip, _sel_widg,
           _sel_twoDhist, ui->plot_ops_statsbox,  ui->clicked_x_lab,
           ui->clicked_y_lab, ui->clicked_z_lab, ui->logx_chbox, ui->logy_chbox,
           ui->logz_chbox, ui->plot_name_box, ui->fit_result_box, ui->clicked_pos_lab,
            h_fit_results);


    if (idir == 0) {

        if (localOrGlobal == "Local") {
            my_hist_widg->_x_label = QString("x_ref (pixels)");
            my_hist_widg->_y_label = QString("x_chip (pixels)");
        }

        else {
            my_hist_widg->_x_label = QString("x_ref (mm)");
            my_hist_widg->_y_label = QString("x_chip (mm)");
        }
    }

    else {
        if (localOrGlobal == "Local") {
            my_hist_widg->_x_label = QString("y_ref (pixels)");
            my_hist_widg->_y_label = QString("y_chip (pixels)");
        }

        else {
            my_hist_widg->_x_label = QString("y_ref (mm)");
            my_hist_widg->_y_label = QString("y_chip (mm)");
        }
    }

    my_hist_widg->_z_label = QString("N");
    if (kind == 0){
        if (idir == 0) _xcorrel_hists.push_back(my_hist_widg);
        else _ycorrel_hists.push_back(my_hist_widg);
    }
    else{
        if (idir == 0) _clust_xcorrel_hists.push_back(my_hist_widg);
        else _clust_ycorrel_hists.push_back(my_hist_widg);
    }

    my_hist_widg->show_axes_labels();
    return my_hist_widg;
}



//_____________________________________________________________________________

hist_widget * MainWindow::get_diff_widget(int ichip, int idir, int kind, bool locVsGlob) {
    //Returns the a twoD_widget* containing a QCustomPlot version of an correlation
    //plot, which was originally a TH2F, for the chip ID ichip in direction idir.


    //Get the plot (naming convention in plot classes must be consistent).
    std::stringstream ss_ichip, ss_irefchip;
    ss_ichip<<ichip;
    ss_irefchip<<_ops->ref_chips[0];
    std::string direc, fit_result_direc, localOrGlobal;

    if (locVsGlob) localOrGlobal = "Local";
    else localOrGlobal = "Global";

    //Decide on th2_direc.
    if (kind == 0){
        if (idir == 0) {
            direc = "/x_pixs_correlations/" + localOrGlobal + "/Differences/Chip_" + ss_ichip.str() + localOrGlobal;
        }
        else if (idir == 1) {
            direc = "/y_pixs_correlations/" + localOrGlobal + "/Differences/Chip_" + ss_ichip.str() + localOrGlobal;
        }
    }

    else {
        if (idir == 0) {
            direc = "/x_clusts_correlations/" + localOrGlobal + "/Differences/Chip_" + ss_ichip.str() + localOrGlobal;
        }
        else if (idir == 1) {
            direc = "/y_clusts_correlations/" + localOrGlobal + "/Differences/Chip_" + ss_ichip.str() + localOrGlobal;
        }
    }

    TH1F * h;
    if (!_liveVsSaved) h = (TH1F*) _tfile->Get(direc.c_str());
    else {
        if (kind == 0) {
            if (idir == 0) {
                if (localOrGlobal == "Local") h = _DQM->_x_pix_correlations->_differencesLocal[ichip];
                else h = _DQM->_x_pix_correlations->_differencesGlobal[ichip];
            }
            else {
                if (localOrGlobal == "Local") h = _DQM->_y_pix_correlations->_differencesLocal[ichip];
                else h = _DQM->_y_pix_correlations->_differencesGlobal[ichip];
            }

        }
        else {
            if (idir == 0) {
                if (localOrGlobal == "Local") h = _DQM->_x_clust_correlations->_differencesLocal[ichip];
                else h = _DQM->_x_clust_correlations->_differencesGlobal[ichip];
            }
            else {
                if (localOrGlobal == "Local") h = _DQM->_y_clust_correlations->_differencesLocal[ichip];
                else h = _DQM->_y_clust_correlations->_differencesGlobal[ichip];
            }
        }
    }

    hist_widget * h_widg = new hist_widget(h, ichip, _sel_Q, _sel_ichip, _sel_widg, _sel_hist,
             ui->plot_ops_statsbox, ui->clicked_x_lab, ui->clicked_y_lab, ui->clicked_z_lab,
             ui->logx_chbox, ui->logy_chbox, ui->logz_chbox, ui->plot_name_box,
             ui->fit_result_box, ui->clicked_pos_lab);

    if (idir == 0) {
        if (localOrGlobal == "Local") h_widg->_x_label = QString("x_ref - x_chip (pixels)");
        else h_widg->_x_label = QString("x_ref - x_chip (mm)");
        h_widg->_y_label = QString("N");
    }

    else {
        if (localOrGlobal == "Local") h_widg->_x_label = QString("y_ref - y_chip (pixels)");
        else h_widg->_x_label = QString("y_ref - y_chip (mm)");
        h_widg->_y_label = QString("N");
    }

    h_widg->show_axes_labels();
    return h_widg;
}







//_____________________________________________________________________________

twoDhist_widget * MainWindow::get_cluster_samples_widget(int ichip) {
    //Returns the a twoD_widget* containing a QCustomPlot version of an correlation
    //plot, which was originally a TH2F, for the chip ID ichip in direction idir.


    //Get the plot (naming convention in plot classes must be consistent).
    std::stringstream ss_ichip;
    ss_ichip<<ichip;
    std::string direc = "Cluster_plots/cluster_samples/Chip_" + ss_ichip.str();
    TH2F* h;
    if (!_liveVsSaved) h = (TH2F*) _tfile->Get(direc.c_str());
    else h = _DQM->_cluster_plots->_cluster_samples[ichip];


    twoDhist_widget * my_hist_widg = new twoDhist_widget(h, ichip, _sel_Q, _sel_ichip, _sel_widg,
           _sel_twoDhist, ui->plot_ops_statsbox,  ui->clicked_x_lab,
           ui->clicked_y_lab, ui->clicked_z_lab, ui->logx_chbox, ui->logy_chbox,
           ui->logz_chbox, ui->plot_name_box, ui->fit_result_box, ui->clicked_pos_lab, NULL);


    my_hist_widg->_x_label = QString("shifted x (pixels)");
    my_hist_widg->_y_label = QString("shifted y (pixels)");
    my_hist_widg->_z_label = QString("pix_hit ADC");
    _clust_samples_hists.push_back(my_hist_widg);

    my_hist_widg->show_axes_labels();
    return my_hist_widg;
}







//_____________________________________________________________________________

twoDhist_widget * MainWindow::get_hitmap_widget(int ichip, int kind) {
    //Returns the a twoD_widget* containing a QCustomPlot version of an correlation
    //plot, which was originally a TH2F, for the chip ID ichip in direction idir.


    //Get the plot (naming convention in plot classes must be consistent).
    std::stringstream ss_ichip; 
    ss_ichip<<ichip;
    std::string direc;
    if (kind == 0) direc = "Pixel_plots/hitmaps/Chip_"+ ss_ichip.str();
    else if (kind == 1) direc = "Cluster_plots/Local_hitmaps/Chip_" + ss_ichip.str();
    else if (kind == 2) direc = "Track_plots/TrackedClusters/Chip_" + ss_ichip.str();
    else if (kind == 3) direc = "Track_plots/NonTrackedClusters/Chip_" + ss_ichip.str();
    else if (kind == 4) direc = "Track_plots/track_hitmap";
    else direc = direc = "Cluster_plots/Global_hitmaps/Chip_" + ss_ichip.str();

    TH2F* h;
    if (!_liveVsSaved) h = (TH2F*) _tfile->Get(direc.c_str());
    else {
        if (kind == 0) h = _DQM->_pixel_plots->_hitmaps[ichip];
        else if (kind == 1) h = _DQM->_cluster_plots->_hitmapsLocal[ichip];
        else if (kind == 2) h = _DQM->_track_plots->_trackedClusters[ichip];
        else if (kind == 3) h = _DQM->_track_plots->_nonTrackedClusters[ichip];
        else if (kind == 4) h = _DQM->_track_plots->_track_hitmap;
        else h = _DQM->_cluster_plots->_hitmapsGlobal[ichip];
    }


    twoDhist_widget * my_hist_widg = new twoDhist_widget(h, ichip, _sel_Q, _sel_ichip, _sel_widg,
           _sel_twoDhist, ui->plot_ops_statsbox,  ui->clicked_x_lab,
           ui->clicked_y_lab, ui->clicked_z_lab, ui->logx_chbox, ui->logy_chbox,
           ui->logz_chbox, ui->plot_name_box, ui->fit_result_box, ui->clicked_pos_lab, NULL);

    //Auto set log scale.
//    my_hist_widg->_colormap->colorScale()->setDataScaleType(QCPAxis::stLogarithmic);
//    my_hist_widg->_logged_z = true;
    //my_hist_widg->_Qh->xAxis->setScaleRatio(my_hist_widg->_Qh->yAxis, 1.0);
    //my_hist_widg->_Qh->replot();

    if (kind == 0 || kind == 1){
        my_hist_widg->_x_label = QString("column (pixels)");
        my_hist_widg->_y_label = QString("row (pixels)");
        my_hist_widg->_z_label = QString("N");
        _hitmap_hists.push_back(my_hist_widg);
    }

    else {
        my_hist_widg->_x_label = QString("x (mm)");
        my_hist_widg->_y_label = QString("y (mm)");
        my_hist_widg->_z_label = QString("N");
        _clust_hitmap_hists.push_back(my_hist_widg);
    }


    my_hist_widg->show_axes_labels();
    return my_hist_widg;
}



//_____________________________________________________________________________

hist_widget * MainWindow::get_hitmapProj_widget(int ichip, int kind) {
    //Returns the a twoD_widget* containing a QCustomPlot version of an correlation
    //plot, which was originally a TH2F, for the chip ID ichip in direction idir.


    //Get the plot (naming convention in plot classes must be consistent).
    std::stringstream ss_ichip;
    ss_ichip<<ichip;
    std::string direc;
    if (kind == 0) direc = "Cluster_plots/Global_XDist/Chip_"+ ss_ichip.str();
    else if (kind ==1) direc = "Cluster_plots/Global_YDist/Chip_" + ss_ichip.str();
    else {
        std::cout<<"Plot not supported in this DQM version nonlive mode."<<std::endl;
        direc = "Cluster_plots/Global_YDist/Chip_" + ss_ichip.str();
    }


    TH1F* h;
    if (!_liveVsSaved) h = (TH1F*) _tfile->Get(direc.c_str());
    else {
        if (kind == 0) h = _DQM->_cluster_plots->_hitmapsGlobalXDist[ichip];
        else if (kind == 1) h = _DQM->_cluster_plots->_hitmapsGlobalYDist[ichip];
        else h = _DQM->_cluster_plots->_t_dists[ichip];
    }


    hist_widget * my_hist_widg = new hist_widget(h, ichip, _sel_Q, _sel_ichip, _sel_widg, _sel_hist,
                 ui->plot_ops_statsbox, ui->clicked_x_lab, ui->clicked_y_lab, ui->clicked_z_lab,
                 ui->logx_chbox, ui->logy_chbox, ui->logz_chbox, ui->plot_name_box,
                 ui->fit_result_box, ui->clicked_pos_lab);


    if (kind == 0){
        my_hist_widg->_x_label = QString("x (mm)");
        my_hist_widg->_y_label = QString("N");
    }

    else if (kind ==1){
        my_hist_widg->_x_label = QString("y (mm)");
        my_hist_widg->_y_label = QString("N");
    }

    else {
        my_hist_widg->_x_label = QString("t (25/16 ns)");
        my_hist_widg->_y_label = QString("N");
    }

    my_hist_widg->show_axes_labels();
    return my_hist_widg;
}




//_____________________________________________________________________________

void MainWindow::on_logx_chbox_clicked(bool checked) {
    //Method to log (or unlog) the x axis of the currently selected plot. Case of a 
    //twoD_hist is slightly different, so start by deciding which type of plot.

    QString label = ui->plot_name_box->text();
    if (label.contains("hitmap") || label.contains("correlation") || label.contains("sample")) { //twoD_hist.
        if (checked) {
            _sel_Q->xAxis->setScaleType(QCPAxis::stLogarithmic);
            _sel_twoDhist->_logged_x = true;    
        }
        else {
            _sel_Q->xAxis->setScaleType(QCPAxis::stLinear);
            _sel_twoDhist->_logged_x = false;
        }
    }


    else { //hist_widget.
        if (checked) {
            _sel_Q->xAxis->setScaleType(QCPAxis::stLogarithmic);
            _sel_hist->_logged_x = true;
        }
        else {
            _sel_Q->xAxis->setScaleType(QCPAxis::stLinear);
            _sel_hist->_logged_x = false;
        }
    }
    _sel_Q->replot();
}







//_____________________________________________________________________________

void MainWindow::on_logy_chbox_clicked(bool checked) {
    //Method to log (or unlog) the x axis of the currently selected plot. Case of a 
    //twoD_hist is slightly different, so start by deciding which type of plot.

    QString label = ui->plot_name_box->text();
    if (label.contains("hitmap") || label.contains("correlation") || label.contains("sample")) { //twoD_hist.
        if (checked) {
            _sel_Q->yAxis->setScaleType(QCPAxis::stLogarithmic);
            _sel_twoDhist->_logged_y = true;
        }
        else {
            _sel_Q->yAxis->setScaleType(QCPAxis::stLinear);
            _sel_twoDhist->_logged_y = false;
        }
    }


    else { //hist_widget.
        if (checked) {
            _sel_Q->yAxis->setScaleType(QCPAxis::stLogarithmic);
            _sel_Q->yAxis->setRangeLower(0.5); //not to waste space.
            _sel_hist->_logged_y = true;
        }
        else {
            _sel_Q->yAxis->setScaleType(QCPAxis::stLinear);
            _sel_hist->_logged_y = false;
        }
    }
    _sel_Q->replot();
}







//_____________________________________________________________________________

void MainWindow::on_logz_chbox_clicked(bool checked){
    //Only valid to use this method whilst a twoD_hist is selected.

    QString label = ui->plot_name_box->text();
    if (label.contains("TH2")) {
        if (checked) {
            _sel_twoDhist->_colormap->colorScale()->setDataScaleType(QCPAxis::stLogarithmic);
            _sel_twoDhist->_colormap->colorScale()->axis()->setRangeLower(0.1);
            _sel_twoDhist->_logged_z = true;
        }

        else {
            _sel_twoDhist->_colormap->colorScale()->setDataScaleType(QCPAxis::stLinear);
            _sel_twoDhist->_logged_z = false;
        }
    }
    _sel_Q->replot();
}





//_____________________________________________________________________________

void MainWindow::ops_apply(){
    //_ops->runNumber = ui->RunNumBox->value();
    _ops->save_file_name = ui->save_file_box->text().toStdString();
    _ops->alignment_save_file_name = ui->AlignmentFileBox->text().toStdString();
    //_ref_file = ui->RefFileBox->text().toStdString();
    _ops->ref_chips.clear();
    _ops->ref_chips.push_back(ui->RefChip1Box->value());
    _ops->ref_chips.push_back(ui->RefChip2Box->value());
    _ops->truncate = ui->TruncateCheckBox->isChecked();
    _ops->tcut = (float)ui->TruncateTimeBox->value();
    _ops->algorithms.clear();

    if (ui->OldRunRadBut->isChecked()) _ops->algorithms.push_back(0);
    if (ui->PSRadBut->isChecked()) _ops->algorithms.push_back(14);
    if (ui->PixPlotsBox->isChecked()) _ops->algorithms.push_back(1);
    if (ui->PixXCorrelBox->isChecked()) _ops->algorithms.push_back(2);
    if (ui->PixYCorrelBox->isChecked()) _ops->algorithms.push_back(3);
    if (ui->PixTCorrelBox->isChecked()) _ops->algorithms.push_back(4);

    if (ui->ClusteringBox->isChecked()) _ops->algorithms.push_back(5);
    if (ui->ClustPlotsBox->isChecked()) _ops->algorithms.push_back(6);
    if (ui->ClustXCorrelBox->isChecked()) _ops->algorithms.push_back(7);
    if (ui->ClustYCorrelBox->isChecked()) _ops->algorithms.push_back(8);
    if (ui->ClustTCorrelBox->isChecked()) _ops->algorithms.push_back(9);

    if (ui->ChipPlotsBoxs->isChecked()) _ops->algorithms.push_back(10);
    if (ui->TrackingBox->isChecked()) _ops->algorithms.push_back(11);
    if (ui->TrackPlotsBox->isChecked()) _ops->algorithms.push_back(12);

    if (ui->SampleVarBox->isChecked()) _ops->algorithms.push_back(15);
    if (ui->RenewSampleVarBox->isChecked()) _ops->replaceLongScalesFile = true;
    else _ops->replaceLongScalesFile = false;

    _ops->load_previous_alignment = ui->LoadPrevAlignBox->isChecked();
    _ops->align_by_pix_differences = ui->AlignByPixelsBox->isChecked();
    _ops->align_by_clust_differences = ui->AlignByClustersBox->isChecked();

    _ops->pix_ToT_nbins = ui->PixADCnBinsBox->value();
    _ops->pix_hitmap_nbins = ui->PixHitmapsnBinsBox->value();
    _ops->pix_correl_xynbins = ui->PixXYCorrelnBinsBox->value();
    _ops->pix_correl_tnbins = ui->PixTDiffnBinsBox->value();

    _ops->pix_xlow = ui->PixXYCorrelLowCutBox->text().toFloat();
    _ops->pix_xup = ui->PixXYCorrelHighCutBox->text().toFloat();
    _ops->pix_ylow = ui->PixXYCorrelLowCutBox->text().toFloat();
    _ops->pix_yup = ui->PixXYCorrelHighCutBox->text().toFloat();
    _ops->pix_tup = ui->PixTCorrelHighCutBox->text().toFloat();
    _ops->pix_tlow = ui->PixTCorrelLowCutBox->text().toFloat();

    _ops->clust_ToT_nbins = ui->ClustADCnBinsBox->value();
    _ops->clust_hitmap_nbins = ui->ClustHitmapsnBinsBox->value();
    _ops->clust_correl_xynbins = ui->ClustXYCorrelnBinsBox->value();
    _ops->clust_correl_tnbins = ui->ClustTDiffnBinsBox->value();

    _ops->clust_xlow = ui->ClustXYCorrelLowCutBox->text().toFloat();
    _ops->clust_xup = ui->ClustXYCorrelHighCutBox->text().toFloat();
    _ops->clust_ylow = ui->ClustXYCorrelLowCutBox->text().toFloat();
    _ops->clust_yup = ui->ClustXYCorrelHighCutBox->text().toFloat();
    _ops->clust_tlow = ui->ClustTCorrelLowCutBox->text().toFloat();
    _ops->clust_tup = ui->ClustTCorrelHighCutBox->text().toFloat();

    _ops->track_resolution_nbins = ui->TrackResnBinsBox->text().toFloat();
}







//_____________________________________________________________________________

void MainWindow::on_rescale_but_clicked() {
    //Reset the range of the plot - useful if plot is lost via zooming.

    //Decide what kind of plot it is.
    QString label = ui->plot_name_box->text();
    if (label.contains("TH2")) { //twoD hist.
        _sel_twoDhist->_colormap->rescaleDataRange();
        _sel_Q->xAxis->setRange(_sel_twoDhist->_nonzero_xmin, _sel_twoDhist->_nonzero_xmax);
        _sel_Q->yAxis->setRange(_sel_twoDhist->_nonzero_ymin, _sel_twoDhist->_nonzero_ymax);
    }

    else { //oneD hist.
        _sel_Q->yAxis->rescale();
        _sel_Q->xAxis->rescale();
    }

    _sel_Q->replot();
}





//_____________________________________________________________________________

void MainWindow::on_open_TBrowser_but_clicked() {
    //Opens a TBrowser - old school (retro man).

    this->close();
    system("./bash_show_TBrowser.sh");
}






//_____________________________________________________________________________

void MainWindow::on_pixel_tabs_currentChanged(int index) {
    //Plot options box is only valid when on a plot tab (and not on z_dist).

    if (_DQM_ran) {
        //if (index == 5 || index == 0) ui->plot_ops_box->setEnabled(false);
        ui->plot_ops_box->setEnabled(true);
    }
}





//_____________________________________________________________________________

void MainWindow::on_exit_but_clicked() {
    this->close();
}





//_____________________________________________________________________________

void MainWindow::set_ops_tab(){
    //Sets up the options tab with default values in CDQM_options.
    //ui->RunNumBox->setValue(_ops->runNumber);

    // Set the boxes and sliders.
    ui->save_file_box->setText(QString(_ops->save_file_name.c_str()));
    //ui->RefFileBox->setText(QString(_ref_file.c_str()));
    ui->AlignmentFileBox->setText(QString(_ops->alignment_save_file_name.c_str()));

    ui->RefChip1Box->setValue(_ops->ref_chips[0]);
    ui->RefChip2Box->setValue(_ops->ref_chips[1]);

    ui->TruncateCheckBox->setChecked(_ops->truncate);
    ui->TruncateTimeBox->setValue(_ops->tcut);

    ui->LoadPrevAlignBox->setChecked(_ops->load_previous_alignment);
    ui->AlignByClustersBox->setChecked(_ops->align_by_clust_differences);
    ui->AlignByPixelsBox->setChecked(_ops->align_by_pix_differences);

    // Particular algorithms.
    if (_ops->algorithms_contains(0)) ui->OldRunRadBut->setChecked(true);
    if (_ops->algorithms_contains(14)) ui->PSRadBut->setChecked(true);
    if (_ops->algorithms_contains(1)) ui->PixPlotsBox->setChecked(true);
    if (_ops->algorithms_contains(2)) ui->PixXCorrelBox->setChecked(true);
    if (_ops->algorithms_contains(3)) ui->PixYCorrelBox->setChecked(true);
    if (_ops->algorithms_contains(4)) ui->PixTCorrelBox->setChecked(true);

    if (_ops->algorithms_contains(5)) ui->ClusteringBox->setChecked(true);
    if (_ops->algorithms_contains(6)) ui->ClustPlotsBox->setChecked(true);
    if (_ops->algorithms_contains(7)) ui->ClustXCorrelBox->setChecked(true);
    if (_ops->algorithms_contains(8)) ui->ClustYCorrelBox->setChecked(true);
    if (_ops->algorithms_contains(9)) ui->ClustTCorrelBox->setChecked(true);

    if (_ops->algorithms_contains(11)) ui->TrackingBox->setChecked(true);
    if (_ops->algorithms_contains(12)) ui->TrackPlotsBox->setChecked(true);
    if (_ops->algorithms_contains(10)) ui->ChipPlotsBoxs->setChecked(true);

    if (_ops->algorithms_contains(15)) ui->SampleVarBox->setChecked(true);
    if (_ops->replaceLongScalesFile) ui->RenewSampleVarBox->setChecked(true);


    // Pix general stuff.
    ui->PixADCnBinsBox->setValue(_ops->pix_ToT_nbins);
    ui->PixHitmapsnBinsBox->setValue(_ops->pix_hitmap_nbins);
    ui->PixXYCorrelnBinsBox->setValue(_ops->pix_correl_xynbins);
    ui->PixTDiffnBinsBox->setValue(_ops->pix_correl_tnbins);

    // Pix correl stuff.
    std::stringstream ss_PixrCorrelCutLow;
    ss_PixrCorrelCutLow << _ops->pix_xlow;
    std::stringstream ss_PixrCorrelCutHigh;
    ss_PixrCorrelCutHigh << _ops->pix_xup;
    ui->PixXYCorrelLowCutBox->setText(QString(ss_PixrCorrelCutLow.str().c_str()));
    ui->PixXYCorrelHighCutBox->setText(QString(ss_PixrCorrelCutHigh.str().c_str()));

    std::stringstream ss_PixtCorrelCutLow;
    ss_PixtCorrelCutLow << _ops->pix_tlow;
    std::stringstream ss_PixtCorrelCutHigh;
    ss_PixtCorrelCutHigh << _ops->pix_tup;
    ui->PixTCorrelLowCutBox->setText(QString(ss_PixtCorrelCutLow.str().c_str()));
    ui->PixTCorrelHighCutBox->setText(QString(ss_PixtCorrelCutHigh.str().c_str()));


    // Cluster general stuff.
    ui->ClustADCnBinsBox->setValue(_ops->clust_ToT_nbins);
    ui->ClustHitmapsnBinsBox->setValue(_ops->clust_hitmap_nbins);
    ui->ClustXYCorrelnBinsBox->setValue(_ops->clust_correl_xynbins);

    // Cluster correl stuff.
    std::stringstream ss_ClustrCorrelCutLow;
    ss_ClustrCorrelCutLow << _ops->clust_xlow;
    std::stringstream ss_ClustrCorrelCutHigh;
    ss_ClustrCorrelCutHigh << _ops->clust_xup;
    ui->ClustXYCorrelLowCutBox->setText(QString(ss_ClustrCorrelCutLow.str().c_str()));
    ui->ClustXYCorrelHighCutBox->setText(QString(ss_ClustrCorrelCutHigh.str().c_str()));

    std::stringstream ss_ClusttCorrelCutLow;
    ss_ClusttCorrelCutLow << _ops->clust_tlow;
    std::stringstream ss_ClusttCorrelCutHigh;
    ss_ClusttCorrelCutHigh << _ops->clust_tup;
    ui->ClustTCorrelLowCutBox->setText(QString(ss_ClusttCorrelCutLow.str().c_str()));
    ui->ClustTCorrelHighCutBox->setText(QString(ss_ClusttCorrelCutHigh.str().c_str()));


    // Track general stuff.
    ui->TrackResnBinsBox->setValue(_ops->track_resolution_nbins);
}








//_____________________________________________________________________________

MainWindow::~MainWindow(){
    if (_DQM_ran && _DQM!=NULL) {
        _DQM->finalize();
        delete _DQM;
    }

    delete ui;
}






//_____________________________________________________________________________

void MainWindow::on_togglelabBut_clicked()
{
    if (ui->DQM_all_tabs->currentIndex() == 1){
        int tab_index = ui->pixel_tabs->currentIndex();
        if (tab_index == 0) {
            for (unsigned int i=0; i<_ADC_distribution_hists.size(); i++) _ADC_distribution_hists[i]->toggle_axes_labels();
        }

        else if (tab_index == 1) {
            for (unsigned int i=0; i<_xcorrel_hists.size(); i++) _xcorrel_hists[i]->toggle_axes_labels();
            for (unsigned int i=0; i<_ycorrel_hists.size(); i++) _ycorrel_hists[i]->toggle_axes_labels();
        }

        else if (tab_index == 2) {
            for (unsigned int i=0; i<_temporal_differences_hists.size(); i++) _temporal_differences_hists[i]->toggle_axes_labels();
        }

        else if (tab_index == 3) {
            for (unsigned int i=0; i<_hitmap_hists.size(); i++) _hitmap_hists[i]->toggle_axes_labels();
        }
    }


    else if (ui->DQM_all_tabs->currentIndex() == 2){
        int tab_index = ui->clust_tabs->currentIndex();
        if (tab_index == 0) {
            for (unsigned int i=0; i<_clust_ADC_distribution_hists.size(); i++) _clust_ADC_distribution_hists[i]->toggle_axes_labels();
        }

        else if (tab_index == 3) {
            for (unsigned int i=0; i<_clust_xcorrel_hists.size(); i++) _clust_xcorrel_hists[i]->toggle_axes_labels();
            for (unsigned int i=0; i<_clust_ycorrel_hists.size(); i++) _clust_ycorrel_hists[i]->toggle_axes_labels();
        }

        else if (tab_index == 4) {
            for (unsigned int i=0; i<_clust_temporal_differences_hists.size(); i++) _clust_temporal_differences_hists[i]->toggle_axes_labels();
        }

        else if (tab_index == 5) {
            for (unsigned int i=0; i<_clust_hitmap_hists.size(); i++) _clust_hitmap_hists[i]->toggle_axes_labels();
        }

        else if (tab_index == 1) {
            for (unsigned int i=0; i<_clust_size_dist_hists.size(); i++) _clust_size_dist_hists[i]->toggle_axes_labels();
        }

        else if (tab_index == 2) {
            for (unsigned int i=0; i<_clust_samples_hists.size(); i++) _clust_samples_hists[i]->toggle_axes_labels();
        }
    }



}






//_____________________________________________________________________________


void MainWindow::on_togglerefBut_clicked()
{
    if (ui->DQM_all_tabs->currentIndex() == 1){
        int tab_index = ui->pixel_tabs->currentIndex();
        if (tab_index == 0) {
            for (unsigned int i=0; i<_ADC_distribution_hists.size(); i++) _ADC_distribution_hists[i]->toggle_ref_plot();
        }

        else if (tab_index == 1) {
            for (unsigned int i=0; i<_xcorrel_hists.size(); i++) _xcorrel_hists[i]->toggle_ref_plot();
            for (unsigned int i=0; i<_ycorrel_hists.size(); i++) _ycorrel_hists[i]->toggle_ref_plot();
        }

        else if (tab_index == 2) {
            for (unsigned int i=0; i<_temporal_differences_hists.size(); i++) _temporal_differences_hists[i]->toggle_ref_plot();
        }

        else if (tab_index == 3) {
            for (unsigned int i=0; i<_hitmap_hists.size(); i++) _hitmap_hists[i]->toggle_ref_plot();
        }
    }


    else if (ui->DQM_all_tabs->currentIndex() == 2){
        int tab_index = ui->clust_tabs->currentIndex();
        if (tab_index == 0) {
            for (unsigned int i=0; i<_clust_ADC_distribution_hists.size(); i++) _clust_ADC_distribution_hists[i]->toggle_ref_plot();
        }

        else if (tab_index == 3) {
            for (unsigned int i=0; i<_clust_xcorrel_hists.size(); i++) _clust_xcorrel_hists[i]->toggle_ref_plot();
            for (unsigned int i=0; i<_clust_ycorrel_hists.size(); i++) _clust_ycorrel_hists[i]->toggle_ref_plot();
        }

        else if (tab_index == 4) {
            for (unsigned int i=0; i<_clust_temporal_differences_hists.size(); i++) _clust_temporal_differences_hists[i]->toggle_ref_plot();
        }

        else if (tab_index == 5) {
            for (unsigned int i=0; i<_clust_hitmap_hists.size(); i++) _clust_hitmap_hists[i]->toggle_ref_plot();
        }

        else if (tab_index == 1) {
            for (unsigned int i=0; i<_clust_size_dist_hists.size(); i++) _clust_size_dist_hists[i]->toggle_ref_plot();
        }

        else if (tab_index == 2) {
            for (unsigned int i=0; i<_clust_samples_hists.size(); i++) _clust_samples_hists[i]->toggle_ref_plot();
        }
    }



}





//_____________________________________________________________________________


void MainWindow::on_pushButton_2_clicked()
{
    if (_include_references) _sel_hist->toggle_ref_plot();
}





//_____________________________________________________________________________

void MainWindow::on_pushButton_clicked()
{
    QString label = ui->plot_name_box->text();
    std::cout<<label.toStdString()<<std::endl;
    if (label.contains("hitmap") || label.contains("correlation") || label.contains("sample")) { //twoD_hist.
        _sel_twoDhist->toggle_axes_labels();
    }
    else _sel_hist->toggle_axes_labels();

    _sel_Q->replot();
}





//_____________________________________________________________________________
