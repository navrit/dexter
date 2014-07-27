#include "hist_widget.h"


//-----------------------------------------------------------------------------

 hist_widget::hist_widget(TH1F * h, int ichip, QCustomPlot* & sel_Qh,
        int & sel_ichip, QWidget* & sel_widg, hist_widget* & sel_hist_widg,
        QLabel * plot_ops_statsbox, QLabel * sel_x_lab, QLabel * sel_y_lab,
        QLabel * sel_z_lab, QCheckBox* xbox, QCheckBox* ybox, QCheckBox* zbox,
        QLabel * plot_ops_name_box, QLabel * fit_results_box, QLabel * clicked_pos_lab,
        TH1F * h_ref){

    _h = h;
    _sel_Qh = &sel_Qh;
    _sel_hist_widg = &sel_hist_widg;
    _sel_ichip = &sel_ichip;
    _sel_x_lab = sel_x_lab;
    _sel_y_lab = sel_y_lab;
    _sel_z_lab = sel_z_lab;
    _sel_widg = &sel_widg;
    _plot_ops_statsbox = plot_ops_statsbox;
    _plot_ops_name_box = plot_ops_name_box;
    _fit_results_box = fit_results_box;
    _clicked_pos_lab = clicked_pos_lab;

    _ref_h = h_ref;
    _fit_parameters[0] = 0.0;
    _fit_parameters[1] = 0.0;
    _fit_parameters[2] = 0.0;
    _ref_showing = false;
    _fit_function_name = "none";

    _logx_chbox = xbox;
    _logy_chbox = ybox;
    _logz_chbox = zbox;

    _logged_x = false;
    _logged_y = false;

    _ichip = ichip;
    _Qh = new QCustomPlot();
    TH1F_to_QcustomPlot(h, _Qh);



    _plot_frame = new QWidget();
    _plot_frame->setStyleSheet("background-color:white;");
    QGridLayout * frame_layout = new QGridLayout();
    frame_layout->setContentsMargins(3, 3, 3, 3);
    frame_layout->addWidget(_Qh);
    _plot_frame->setLayout(frame_layout);

    QGridLayout * ADC_layout = new QGridLayout();
    ADC_layout->setContentsMargins(1, 1, 1, 1);
    ADC_layout->addWidget(_plot_frame);
    setLayout(ADC_layout);
    add_qh_functionality();

    _x_label = "";
    _y_label = "";
}



 //-----------------------------------------------------------------------------

  hist_widget::hist_widget(TGraph * h, int ichip, QCustomPlot* & sel_Qh,
         int & sel_ichip, QWidget* & sel_widg, hist_widget* & sel_hist_widg,
         QLabel * plot_ops_statsbox, QLabel * sel_x_lab, QLabel * sel_y_lab,
         QLabel * sel_z_lab, QCheckBox* xbox, QCheckBox* ybox, QCheckBox* zbox,
         QLabel * plot_ops_name_box, QLabel * fit_results_box, QLabel * clicked_pos_lab,
         TH1F * h_ref){

     _h = NULL;
     _sel_Qh = &sel_Qh;
     _sel_hist_widg = &sel_hist_widg;
     _sel_ichip = &sel_ichip;
     _sel_x_lab = sel_x_lab;
     _sel_y_lab = sel_y_lab;
     _sel_z_lab = sel_z_lab;
     _sel_widg = &sel_widg;
     _plot_ops_statsbox = plot_ops_statsbox;
     _plot_ops_name_box = plot_ops_name_box;
     _fit_results_box = fit_results_box;
     _clicked_pos_lab = clicked_pos_lab;

     _ref_h = h_ref;
     _fit_parameters[0] = 0.0;
     _fit_parameters[1] = 0.0;
     _fit_parameters[2] = 0.0;
     _ref_showing = false;
     _fit_function_name = "none";

     _logx_chbox = xbox;
     _logy_chbox = ybox;
     _logz_chbox = zbox;

     _logged_x = false;
     _logged_y = false;

     _ichip = ichip;
     _Qh = new QCustomPlot();
     TGraph_to_QcustomPlot(h, _Qh);



     _plot_frame = new QWidget();
     _plot_frame->setStyleSheet("background-color:white;");
     QGridLayout * frame_layout = new QGridLayout();
     frame_layout->setContentsMargins(3, 3, 3, 3);
     frame_layout->addWidget(_Qh);
     _plot_frame->setLayout(frame_layout);

     QGridLayout * ADC_layout = new QGridLayout();
     ADC_layout->setContentsMargins(1, 1, 1, 1);
     ADC_layout->addWidget(_plot_frame);
     setLayout(ADC_layout);
     add_qh_functionality();

     _x_label = "";
     _y_label = "";
 }





 //-----------------------------------------------------------------------------

void hist_widget::add_qh_functionality(){
    _Qh->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectAxes |
                              QCP::iSelectLegend);


    // connect slot that ties some axis selections together (especially opposite axes):
    connect(_Qh, SIGNAL(selectionChangedByUser()), this, SLOT(selectionChanged()));


    // connect slots that takes care that when an axis is selected, only that direction can be dragged and zoomed:
    connect(_Qh, SIGNAL(mousePress(QMouseEvent*)), this, SLOT(mousePress(QMouseEvent*)));
    connect(_Qh, SIGNAL(mouseWheel(QWheelEvent*)), this, SLOT(mouseWheel()));


    // make bottom and left axes transfer their ranges to top and right axes:
    connect(_Qh->xAxis, SIGNAL(rangeChanged(QCPRange)), _Qh->xAxis2, SLOT(setRange(QCPRange)));
    connect(_Qh->yAxis, SIGNAL(rangeChanged(QCPRange)), _Qh->yAxis2, SLOT(setRange(QCPRange)));


    // connect some interaction slots:
//    connect(_Qh, SIGNAL(titleDoubleClick(QMouseEvent*,QCPPlotTitle*)), this, SLOT(titleDoubleClick(QMouseEvent*,QCPPlotTitle*)));
//    connect(_Qh, SIGNAL(axisDoubleClick(QCPAxis*,QCPAxis::SelectablePart,QMouseEvent*)), this, SLOT(axisLabelDoubleClick(QCPAxis*,QCPAxis::SelectablePart)));
//    connect(_Qh, SIGNAL(legendDoubleClick(QCPLegend*,QCPAbstractLegendItem*,QMouseEvent*)), this, SLOT(legendDoubleClick(QCPLegend*,QCPAbstractLegendItem*)));
    _Qh->replot();
}





//_____________________________________________________________________________

void hist_widget::mousePress(QMouseEvent* e){
    if ((*_sel_Qh) != _Qh) make_selected();

    // if an axis is selected, only allow the direction of that axis to be dragged
    // if no axis is selected, both directions may be dragged
    std::stringstream ssx, ssy;
    if (_Qh->xAxis->selectedParts().testFlag(QCPAxis::spAxis))
    _Qh->axisRect()->setRangeDrag(_Qh->xAxis->orientation());
    else if (_Qh->yAxis->selectedParts().testFlag(QCPAxis::spAxis))
    _Qh->axisRect()->setRangeDrag(_Qh->yAxis->orientation());
    else {_Qh->axisRect()->setRangeDrag(Qt::Horizontal|Qt::Vertical);
        ssx << std::setprecision(3) << _Qh->xAxis->pixelToCoord(e->x());
        ssy << std::setprecision(3) << _Qh->yAxis->pixelToCoord(e->y());
        _sel_x_lab->setText(QString(("x: " + ssx.str()).c_str()));
        _sel_y_lab->setText(QString(("y: " + ssy.str()).c_str()));
        _sel_z_lab->setText(QString(" "));
    }
}





//_____________________________________________________________________________

void hist_widget::make_selected(){
    (*_sel_widg)->setStyleSheet("background-color:white;");
    (*_sel_widg)->repaint();
    (*_sel_widg) = _plot_frame;
    (*_sel_hist_widg) = this;
    (*_sel_widg)->setStyleSheet("background-color:red;");
    (*_sel_widg)->repaint();
    (*_sel_Qh) = _Qh;
    (*_sel_ichip) = _ichip;
    update_plotops_statsbox();
    _sel_z_lab->setText(QString(" "));

    _sel_x_lab->setText(QString("x:"));
    _sel_y_lab->setText(QString("y:"));
    _sel_z_lab->setText(QString(" "));


    if (_logged_x) _logx_chbox->setChecked(true);
    else _logx_chbox->setChecked(false);

    if (_logged_y) _logy_chbox->setChecked(true);
    else _logy_chbox->setChecked(false);

    _logz_chbox->setEnabled(false);
}





//_____________________________________________________________________________

void hist_widget::update_plotops_statsbox(){
    if (_h!=NULL) {
        std::stringstream ss_mean, ss_STD, ssup, sslow, ssN, ss_const, ss_MPV, ss_sigma, ss_peak;
        ssN << std::setprecision(2) << _h->GetEntries();
        ss_mean << std::setprecision(2) << _h->GetMean();
        ss_STD << std::setprecision(2) << _h->GetRMS();
        ss_const << std::setprecision(2) << _fit_parameters[0];
        ss_MPV << std::setprecision(2) << _fit_parameters[1];
        ss_sigma << std::setprecision(2) << _fit_parameters[2];
        ss_peak << std::setprecision(2) << _h->GetXaxis()->GetBinCenter(_h->GetMaximumBin());


        //Name.
        std::string s = std::string(_h->GetName()) + ", " + _h->GetTitle() + ", " + "TH1";
        _plot_ops_name_box->setText(QString(s.c_str()));

        //Stats.
        std::string x = "N: " + ssN.str() + "     "
                + "mu: "  + ss_mean.str() + "     " + "sigma: " + ss_STD.str();
        _plot_ops_statsbox->setText(QString(x.c_str()));

        //Fit.
        std::string y;
        if (_fit_function_name == "landau") {
            y = "C: " + ss_const.str() + "     " + "MPV"
                + ": "  + ss_MPV.str() + "     " + "sigma: " + ss_sigma.str();
        }

        else if (_fit_function_name == "gaussian") {
            y = "C: " + ss_const.str() + "     "
                + "mu: "  + ss_MPV.str() + "     " + "sigma: " + ss_sigma.str();
        }

        else {
            y = "Peak Bin Position: " + ss_peak.str() + " events";
        }
         _fit_results_box->setText(QString(y.c_str()));

         //Clicked title units.
         _clicked_pos_lab->setText(QString("Clicked position (plot units):"));

    }


}






//_____________________________________________________________________________

void hist_widget::mouseWheel(){
    if ((*_sel_Qh) != _Qh) make_selected();
  // if an axis is selected, only allow the direction of that axis to be zoomed
  // if no axis is selected, both directions may be zoomed

  if (_Qh->xAxis->selectedParts().testFlag(QCPAxis::spAxis))
    _Qh->axisRect()->setRangeZoom(_Qh->xAxis->orientation());
  else if (_Qh->yAxis->selectedParts().testFlag(QCPAxis::spAxis))
    _Qh->axisRect()->setRangeZoom(_Qh->yAxis->orientation());
  else
    _Qh->axisRect()->setRangeZoom(Qt::Horizontal|Qt::Vertical);
}







//_____________________________________________________________________________


void hist_widget::selectionChanged(){
  /*
   normally, axis base line, axis tick labels and axis labels are selectable separately, but we want
   the user only to be able to select the axis as a whole, so we tie the selected states of the tick labels
   and the axis base line together. However, the axis label shall be selectable individually.

   The selection state of the left and right axes shall be synchronized as well as the state of the
   bottom and top axes.

   Further, we want to synchronize the selection of the graphs with the selection state of the respective
   legend item belonging to that graph. So the user can select a graph by either clicking on the graph itself
   or on its legend item.
  */

  // make top and bottom axes be selected synchronously, and handle axis and tick labels as one selectable object:
  if (_Qh->xAxis->selectedParts().testFlag(QCPAxis::spAxis) || _Qh->xAxis->selectedParts().testFlag(QCPAxis::spTickLabels) ||
      _Qh->xAxis2->selectedParts().testFlag(QCPAxis::spAxis) || _Qh->xAxis2->selectedParts().testFlag(QCPAxis::spTickLabels))
  {
    _Qh->xAxis2->setSelectedParts(QCPAxis::spAxis|QCPAxis::spTickLabels);
    _Qh->xAxis->setSelectedParts(QCPAxis::spAxis|QCPAxis::spTickLabels);
  }
  // make left and right axes be selected synchronously, and handle axis and tick labels as one selectable object:
  if (_Qh->yAxis->selectedParts().testFlag(QCPAxis::spAxis) || _Qh->yAxis->selectedParts().testFlag(QCPAxis::spTickLabels) ||
      _Qh->yAxis2->selectedParts().testFlag(QCPAxis::spAxis) || _Qh->yAxis2->selectedParts().testFlag(QCPAxis::spTickLabels))
  {
    _Qh->yAxis2->setSelectedParts(QCPAxis::spAxis|QCPAxis::spTickLabels);
    _Qh->yAxis->setSelectedParts(QCPAxis::spAxis|QCPAxis::spTickLabels);
  }

  // synchronize selection of graphs with selection of corresponding legend items:
  for (int i=0; i<_Qh->graphCount(); ++i)
  {
    QCPGraph *graph = _Qh->graph(i);
    QCPPlottableLegendItem *item = _Qh->legend->itemWithPlottable(graph);
    if (item->selected() || graph->selected())
    {
      item->setSelected(true);
      graph->setSelected(true);
    }
  }
}






//-----------------------------------------------------------------------------

void hist_widget::TH1F_to_QcustomPlot(TH1F * h, QCustomPlot * Qh){
    //_h = h;
    //ROOT 1D histogram converter to Qh. WCP has a really neat function that
    //can take the bin heights and positions, and plot them like a histogram
    //(in steps). Still have to get the bins individually.
    //DOESNT PLOT OUTERFLOW BINS.

    int Nbins = h->GetNbinsX();
    QVector<double> x(Nbins+1), y(Nbins+1); //ignore overflows, add extra bits.
    int min_y; int max_y;

    //Cycle over bins.
    for (int i=1; i<=Nbins; ++i){
        int ibin = h->GetBin(i); //get the correct bin number.

        x[i-1] = h->GetBinLowEdge(ibin);
        y[i-1] = h->GetBinContent(ibin);

        //For y range.
        if (i==1){
            min_y = y[0]; max_y = y[0];
        }
        else {
            if (y[i-1] < min_y) min_y = y[i-1];
            if (y[i-1] > max_y) max_y = y[i-1];
        }
    }

    x[Nbins] = h->GetBinLowEdge(h->GetBin(Nbins)) + h->GetBinWidth(h->GetBin(Nbins));
    y[Nbins] = h->GetBinContent(h->GetBin(Nbins));

    float vol1 = 0.0, vol2 = 0.0;
    //Normalize if a reference.
    if (Qh->plottableCount()>1){
        for (int i=1; i<=Nbins; i++){
            int ibin = _h->GetBin(i);
            vol1 += _h->GetBinContent(ibin);
            vol2 += _ref_h->GetBinContent(ibin);
        }

        float r = vol1/vol2;
        for (int i=0; i<=Nbins; i++) y[i]*=r;
        min_y *= r;
        max_y *= r;
    }


    //Create graph and assign data to it:
    Qh->addGraph();
    Qh->graph(Qh->plottableCount()-1)->setLineStyle(QCPGraph::lsStepLeft);
    Qh->graph(Qh->plottableCount()-1)->setData(x, y);


    //Reduce the number of ticks.
    Qh->xAxis->setAutoTickCount(3);
    Qh->yAxis->setAutoTickCount(3);
    Qh->xAxis->setTickLabelFont(QFont(font().family(), 11));
    Qh->yAxis->setTickLabelFont(QFont(font().family(), 11));
    Qh->yAxis->setRange(min_y, max_y);
    Qh->xAxis->setRange(h->GetXaxis()->GetXmin(), h->GetXaxis()->GetXmax());

    Qh->axisRect()->setupFullAxesBox();
    Qh->graph(Qh->plottableCount()-1)->setBrush(QBrush(QColor(0,0,255,100)));

    Qh->xAxis->setLabelFont(QFont(font().family(), 11));


    Qh->yAxis->setLabelFont(QFont(font().family(), 11));

    if (Qh->plottableCount()==1) {
        add_stats_box(h);
        Qh->xAxis->setLabel(_x_label);
        Qh->yAxis->setLabel(_y_label);
    }

    _Qh->graph()->setPen(QPen(Qt::black));
}




//-----------------------------------------------------------------------------

void hist_widget::TGraph_to_QcustomPlot(TGraph * g, QCustomPlot * Qh) {
    int N = g->GetN();
    QVector<double> x(N), y(N); //ignore overflows, add extra bits.

    //Cycle over points.
    for (int i=0; i<N; ++i) g->GetPoint(i, x[i], y[i]);

    //Create graph and assign data to it:
    Qh->addGraph();
    Qh->graph(Qh->plottableCount()-1)->setLineStyle(QCPGraph::lsLine);
    Qh->graph(Qh->plottableCount()-1)->setData(x, y);


    //Reduce the number of ticks.
    Qh->xAxis->setAutoTickCount(3);
    Qh->yAxis->setAutoTickCount(3);
    Qh->xAxis->setTickLabelFont(QFont(font().family(), 11));
    Qh->yAxis->setTickLabelFont(QFont(font().family(), 11));
    //Qh->yAxis->setRange(min_y, max_y);
    //Qh->xAxis->setRange(h->GetXaxis()->GetXmin(), h->GetXaxis()->GetXmax());

    Qh->axisRect()->setupFullAxesBox();
    Qh->graph(Qh->plottableCount()-1)->setBrush(QBrush(QColor(0,0,255,100)));

    Qh->xAxis->setLabelFont(QFont(font().family(), 11));
    Qh->yAxis->setLabelFont(QFont(font().family(), 11));
    Qh->rescaleAxes();
    Qh->graph()->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssDisc, 5));
}






//-----------------------------------------------------------------------------

void hist_widget::add_stats_box(TH1F* h){
    // add the text label at the top:
    std::stringstream ssup, sslow;
    sslow << h->GetBinContent(h->GetBin(0));
    ssup << h->GetBinContent(h->GetBin(h->GetNbinsX())+1);
    std::string Statsbox_text = "<" + sslow.str() + "\n>" + ssup.str();
    QCPItemText * Statsbox = new QCPItemText(_Qh);
    _Qh->addItem(Statsbox);
    Statsbox->position->setType(QCPItemPosition::ptAxisRectRatio);
    Statsbox->setPositionAlignment(Qt::AlignRight|Qt::AlignTop);
    Statsbox->position->setCoords(0.97, 0.03); // lower right corner of axis rect
    Statsbox->setTextAlignment(Qt::AlignLeft);
    Statsbox->setFont(QFont(font().family(), 11));
    Statsbox->setPadding(QMargins(2, 2, 2, 2));
    Statsbox->setText(QString(Statsbox_text.c_str()));
    Statsbox->setPen(QPen(Qt::black));
    Statsbox->setBrush(QBrush(Qt::white));
    Statsbox->setSelectable(true);
}






//-----------------------------------------------------------------------------

void hist_widget::add_mean_line(){
    //Adds a big verticle green line showing the mean.
    QVector<double> x(2), y(2);
    x[0] = _h->GetMean(); x[1] = x[0];
    y[0] = 0;
    y[1] = _h->GetBinContent(_h->GetMaximumBin());

    _Qh->addGraph();
    QPen mean_pen;
    mean_pen.setColor(QColor(0, 230, 0, 255));
    mean_pen.setWidth(3);
    _Qh->graph(1)->setPen(mean_pen);
    _Qh->graph(1)->setData(x, y);
}






//-----------------------------------------------------------------------------

void hist_widget::fit_landau(QColor c){
    if (_h->Integral()!=0){
        _fit_function_name = "landau";
        //Fits a landau to the TH1F in this instance, and fills the parameters.
        _h->Fit("landau", "Q");
        TF1 * fit = _h->GetFunction("landau");
        _fit_parameters[0] = fit->GetParameter(0);
        _fit_parameters[1] = fit->GetParameter(1);
        _fit_parameters[2] = fit->GetParameter(2);

        //Add the curve (done after mean line).
        int ndots = 500;
        QVector<double> x(ndots), y(ndots);
        double xmin = _h->GetXaxis()->GetXmin();
        double xstep = (_h->GetXaxis()->GetXmax() - _h->GetXaxis()->GetXmin())/(1.0*ndots);
        for (int i=0; i<ndots; i++){
            x[i] = xmin + i*xstep;
            y[i] = fit->Eval(x[i]);
        }

        _Qh->addGraph();
        QPen fit_pen;
        fit_pen.setColor(c);
        fit_pen.setWidthF(2);
        _Qh->graph(_Qh->plottableCount()-1)->setPen(fit_pen);
        _Qh->graph(_Qh->plottableCount()-1)->setData(x, y);
    }
}



//-----------------------------------------------------------------------------

void hist_widget::fit_gaussian(QColor c){
    if (_h->Integral() != 0) {
        _fit_function_name = "gaussian";
        //Fits a landau to the TH1F in this instance, and fills the parameters.
        _h->Fit("gaus", "Q");
        TF1 * fit = _h->GetFunction("gaus");
        _fit_parameters[0] = fit->GetParameter(0);
        _fit_parameters[1] = fit->GetParameter(1);
        _fit_parameters[2] = fit->GetParameter(2);

        //Add the curve (done after mean line).
        int ndots = 500;
        QVector<double> x(ndots), y(ndots);
        double xmin = _h->GetXaxis()->GetXmin();
        double xstep = (_h->GetXaxis()->GetXmax() - _h->GetXaxis()->GetXmin())/(1.0*ndots);
        for (int i=0; i<ndots; i++){
            x[i] = xmin + i*xstep;
            y[i] = fit->Eval(x[i]);
        }

        _Qh->addGraph();
        QPen fit_pen;
        fit_pen.setColor(c);
        fit_pen.setWidthF(2);
        _Qh->graph(_Qh->plottableCount()-1)->setPen(fit_pen);
        _Qh->graph(_Qh->plottableCount()-1)->setData(x, y);
    }
}






//-----------------------------------------------------------------------------

void hist_widget::show_axes_labels(){
    //std::cout<<_x_label.toStdString()<<"\t"<<_Qh->xAxis->label().toStdString()<<std::endl;
    if (_Qh->xAxis->label() != _x_label) _Qh->xAxis->setLabel(_x_label);

    if (_Qh->yAxis->label() != _y_label) _Qh->yAxis->setLabel(_y_label);

    _Qh->replot();
}





//-----------------------------------------------------------------------------

void hist_widget::toggle_axes_labels(){
    //std::cout<<_x_label.toStdString()<<"\t"<<_Qh->xAxis->label().toStdString()<<std::endl;
    if (_Qh->xAxis->label() != _x_label) _Qh->xAxis->setLabel(_x_label);
    else _Qh->xAxis->setLabel(QString(""));

    if (_Qh->yAxis->label() != _y_label) _Qh->yAxis->setLabel(_y_label);
    else _Qh->yAxis->setLabel(QString(""));

    _Qh->replot();
}






//-----------------------------------------------------------------------------

void hist_widget::toggle_ref_plot(){
    if (_ref_h != NULL){
        if (!_ref_showing) {
            _ref_showing = true;
            TH1F_to_QcustomPlot(_ref_h, _Qh);
            _Qh->graph(_Qh->plottableCount()-1)->setBrush(Qt::NoBrush);
            _Qh->graph(_Qh->plottableCount()-1)->setLineStyle(QCPGraph::lsNone);
            _Qh->graph(_Qh->plottableCount()-1)->setScatterStyle(QCPScatterStyle::ssDisc);
        }
        else {
            _ref_showing = false;
            _Qh->graph(_Qh->plottableCount()-1)->clearData();
        }
        _Qh->replot();
    }
    else std::cout<<"No reference available."<<std::endl;
}






//-----------------------------------------------------------------------------
