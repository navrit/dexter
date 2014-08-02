#include "twoDhist_widget.h"

//_____________________________________________________________________________

 twoDhist_widget::twoDhist_widget(TH2F * h, int ichip, QCustomPlot* & sel_Qh,
        int & sel_ichip, QWidget* & sel_widg, twoDhist_widget* & sel_hist_widg,
        QLabel * plot_ops_statsbox, QLabel * sel_x_lab, QLabel * sel_y_lab,
        QLabel * sel_z_lab, QCheckBox* xbox, QCheckBox* ybox, QCheckBox* zbox,
        QLabel * plot_ops_name_box, QLabel * fit_results_box, QLabel * clicked_pos_lab,
        TH1F * fit_results){

	 mRubberBand = new QRubberBand(QRubberBand::Rectangle, this);

     _ichip = ichip;
     _h = h;

     _x_label = "";
     _y_label = "";
     _z_label = "";



    //The below passed references are to objects from a higher level of
    //scope, which change when a plot is selected (clicked).
    _sel_hist_widg = &sel_hist_widg;
    _sel_Qh = &sel_Qh;
    _sel_ichip = &sel_ichip;
    _sel_x_lab = sel_x_lab;
    _sel_y_lab = sel_y_lab;
    _sel_z_lab = sel_z_lab;
    _sel_widg = &sel_widg;
    _plot_ops_statsbox = plot_ops_statsbox;
    _plot_ops_name_box = plot_ops_name_box;
    _fit_results_box = fit_results_box;
    _clicked_pos_lab = clicked_pos_lab;
    _fit_results = fit_results;

    _logged_x = false;
    _logged_y = false;
    _logged_z = false;

    _logx_chbox = xbox;
    _logy_chbox = ybox;
    _logz_chbox = zbox;

    _Qh = new QCustomPlot();
    TH2F_to_QcustomPlot();
    add_qh_functionality();


    //Put it in another widget to make a highlightable frame.
    _plot_frame = new QWidget();
    _plot_frame->setStyleSheet("background-color:white;");
    QGridLayout * frame_layout = new QGridLayout(); //frame needs a layout.
    frame_layout->setContentsMargins(3, 3, 3, 3);
    frame_layout->addWidget(_Qh);
    _plot_frame->setLayout(frame_layout);


    //Fill the twoDhist_widget with a layout.
    QGridLayout * ADC_layout = new QGridLayout();
    ADC_layout->setContentsMargins(1, 1, 1, 1);
    ADC_layout->addWidget(_plot_frame);
    setLayout(ADC_layout);
}





 //-----------------------------------------------------------------------------

void twoDhist_widget::add_qh_functionality(){
    //Setup interactions from the GUI to the plot.
    _Qh->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectAxes |
                              QCP::iSelectLegend);


    // connect slot that ties some axis selections together (especially opposite axes):
    connect(_Qh, SIGNAL(selectionChangedByUser()), this, SLOT(selectionChanged()));


    // connect slots that takes care that when an axis is selected, only that direction can be dragged and zoomed:
    connect(_Qh, SIGNAL(mousePress(QMouseEvent*)), this, SLOT(mousePress(QMouseEvent*)));
    connect(_Qh, SIGNAL(mouseWheel(QWheelEvent*)), this, SLOT(mouseWheel()));
    connect(_Qh, SIGNAL(mouseRelease(QMouseEvent*)), this, SLOT(mouseRelease(QMouseEvent*)));
    connect(_Qh, SIGNAL(mouseMove(QMouseEvent*)), this, SLOT(mouseMove(QMouseEvent*)));

    // make bottom and left axes transfer their ranges to top and right axes:
    connect(_Qh->xAxis, SIGNAL(rangeChanged(QCPRange)), _Qh->xAxis2, SLOT(setRange(QCPRange)));
    connect(_Qh->yAxis, SIGNAL(rangeChanged(QCPRange)), _Qh->yAxis2, SLOT(setRange(QCPRange)));
    _Qh->replot();
}





//_____________________________________________________________________________

void twoDhist_widget::mouseRelease(QMouseEvent * event)
{
    if (mRubberBand->isVisible())
    {
        const QRect & zoomRect = mRubberBand->geometry();
        int xp1, yp1, xp2, yp2;
        zoomRect.getCoords(&xp1, &yp1, &xp2, &yp2);
        auto x1 = _Qh->xAxis->pixelToCoord(xp1);
        auto x2 = _Qh->xAxis->pixelToCoord(xp2);
        auto y1 = _Qh->yAxis->pixelToCoord(yp1);
        auto y2 = _Qh->yAxis->pixelToCoord(yp2);

        _Qh->xAxis->setRange(x1, x2);
        _Qh->yAxis->setRange(y1, y2);

        mRubberBand->hide();
        _Qh->replot();
    }
    //QCustomPlot::mouseReleaseEvent(event);
}




//_____________________________________________________________________________

void twoDhist_widget::mouseMove(QMouseEvent * event)
{
    if (mRubberBand->isVisible())
    {
        mRubberBand->setGeometry(QRect(mOrigin, event->pos()).normalized());
    }
    //QCustomPlot::mouseMoveEvent(event);
}


//_____________________________________________________________________________

void twoDhist_widget::mousePress(QMouseEvent* e){
    //As well as plot interaction, clicking on a plot should make it the
    //'selected' plot (and hence, widget). This involves changing the background
    //colour of the frame, and updating the plot options box (which is global).


    //Do the 'selection'.
    if ((*_sel_Qh) != _Qh) make_selected();

    if (e->button() == Qt::RightButton){
        std::cout<<"Right click"<<std::endl;
        mOrigin = e->pos();
        mRubberBand->setGeometry(QRect(mOrigin, QSize()));
        mRubberBand->show();
    }

    else {
		//Decide on the type of interaction.
		std::stringstream ssx, ssy, ssz;
		if (_Qh->xAxis->selectedParts().testFlag(QCPAxis::spAxis))
			_Qh->axisRect()->setRangeDrag(_Qh->xAxis->orientation());

		else if (_Qh->yAxis->selectedParts().testFlag(QCPAxis::spAxis))
			_Qh->axisRect()->setRangeDrag(_Qh->yAxis->orientation());

		else {_Qh->axisRect()->setRangeDrag(Qt::Horizontal|Qt::Vertical);
			ssx << std::setprecision(4) << _Qh->xAxis->pixelToCoord(e->x());
			ssy << std::setprecision(4) << _Qh->yAxis->pixelToCoord(e->y());
			double z = _colormap->data()->data(_Qh->xAxis->pixelToCoord(e->x()), _Qh->yAxis->pixelToCoord(e->y()));
			ssz << std::setprecision(4) << z;
			_sel_x_lab->setText(QString(("x: " + ssx.str()).c_str()));
			_sel_y_lab->setText(QString(("y: " + ssy.str()).c_str()));
			_sel_z_lab->setText(QString(("z: " + ssz.str()).c_str()));
		}
    }
}





//_____________________________________________________________________________

void twoDhist_widget::make_selected(){
    (*_sel_widg)->setStyleSheet("background-color:white;");
    (*_sel_widg)->repaint();
    (*_sel_widg) = _plot_frame;
    (*_sel_hist_widg) = this;
    (*_sel_widg)->setStyleSheet("background-color:red;");
    (*_sel_widg)->repaint();
    (*_sel_Qh) = _Qh;
    (*_sel_ichip) = _ichip;
    update_plotops_statsbox();
    _logz_chbox->setEnabled(true);

    _sel_x_lab->setText(QString("x:"));
    _sel_y_lab->setText(QString("y:"));
    _sel_z_lab->setText(QString("z:"));


    if (_logged_x) _logx_chbox->setChecked(true);
    else _logx_chbox->setChecked(false);

    if (_logged_y) _logy_chbox->setChecked(true);
    else _logy_chbox->setChecked(false);

    if (_logged_z) _logz_chbox->setChecked(true);
    else _logz_chbox->setChecked(false);

    fill_fit_results();

}




//_____________________________________________________________________________

void twoDhist_widget::fill_fit_results(){
    std::string y;

    if (_fit_results != NULL){
        int ibintheta = _fit_results->GetBin(1);
        std::stringstream ss_theta, ss_c_shift, ss_SN;
        ss_theta << std::setprecision(3) << _fit_results->GetBinContent(ibintheta)*57.296;
        ss_c_shift << std::setprecision(3) << _fit_results->GetBinContent(ibintheta+2);
        ss_SN << std::setprecision(3) << _fit_results->GetBinContent(ibintheta+4);

        y = QString(0x3B8).toStdString() + ": " + ss_theta.str() + "     " + QString(0x394).toStdString()
                + "x: "  + ss_c_shift.str() + "     S/N: ~" + ss_SN.str();
    }

    else {
        y = " ";
    }

    _fit_results_box->setText(QString(y.c_str()));
}






//_____________________________________________________________________________

void twoDhist_widget::update_plotops_statsbox(){
    //Upon being selected, the plot options box needs to be updated with
    //relevant information about this plot. Start by making the string of text.


    std::stringstream ssN;
    ssN <<  std::setprecision(3) << _h->GetEntries();

    std::string s = std::string(_h->GetName()) + ", " + _h->GetTitle() + ", " + "TH2";
    _plot_ops_name_box->setText(QString(s.c_str()));
    std::string x = "N:     " + ssN.str();
    _plot_ops_statsbox->setText(QString(x.c_str()));

    std::string clicked_title = "Clicked position (plots units):";
    _fit_results_box->setText(" ");

    _clicked_pos_lab->setText(QString(clicked_title.c_str()));

}






//_____________________________________________________________________________

void twoDhist_widget::mouseWheel(){
    //Do the 'selection'.
    if ((*_sel_Qh) != _Qh) make_selected();

    //Controls zooming.
    if (_Qh->xAxis->selectedParts().testFlag(QCPAxis::spAxis))
        _Qh->axisRect()->setRangeZoom(_Qh->xAxis->orientation());
    else if (_Qh->yAxis->selectedParts().testFlag(QCPAxis::spAxis))
        _Qh->axisRect()->setRangeZoom(_Qh->yAxis->orientation());
    else
        _Qh->axisRect()->setRangeZoom(Qt::Horizontal|Qt::Vertical);
}







//_____________________________________________________________________________


void twoDhist_widget::selectionChanged(){
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

void twoDhist_widget::TH2F_to_QcustomPlot(){
    //The actual conversion. QcustomPlot requires a twoD map, which here,
    //are the heights of the bins of the TH2F.
    int nbinsx = _h->GetNbinsX();
    int nbinsy = _h->GetNbinsY();


    //Get the axis extremes.
    double xmin, xmax, ymin, ymax;
    xmin = _h->GetXaxis()->GetXmin();
    xmax = _h->GetXaxis()->GetXmax();
    ymin = _h->GetYaxis()->GetXmin();
    ymax = _h->GetYaxis()->GetXmax();

    _nonzero_xmin = xmin;
    _nonzero_ymin = ymin;
    _nonzero_xmax = xmax;
    _nonzero_ymax = ymax;
    //std::cout<<"Hitmap size:\t"<<xmin<<"\t"<<xmax<<"\t"<<ymin<<"\t"<<ymax<<"\t"<<nbinsx<<"\t"<<nbinsy<<std::endl;

    //Make a QCPColorMap:
    _colormap = new QCPColorMap(_Qh->xAxis, _Qh->yAxis);
    _Qh->addPlottable(_colormap);
    _colormap->data()->setSize(nbinsx, nbinsy); // we want the color map to have 100x100 data points
    double xBinWidth = _h->GetXaxis()->GetBinWidth(1);
    double yBinWidth = _h->GetYaxis()->GetBinWidth(1);
    _colormap->data()->setRange(QCPRange(xmin+0.5*xBinWidth, xmax-0.5*xBinWidth), QCPRange(ymin+0.5*yBinWidth, ymax-0.5*yBinWidth));
    //Assign data, by accessing the QCPColorMapData instance of the color map.
    for (int ix=1; ix<nbinsx+1; ++ix){
        for (int iy=1; iy<nbinsy+1; ++iy){
            int ibin = _h->GetBin(ix, iy);
            int xIndex, yIndex;
            double x, y;
            x = _h->GetXaxis()->GetBinLowEdge(ix);
            y = _h->GetYaxis()->GetBinLowEdge(iy);
            _colormap->data()->coordToCell(x, y, &xIndex, &yIndex);
            _colormap->data()->setCell(ix-1, iy-1, _h->GetBinContent(ibin));
        }
    }
    _Qh->xAxis->setTickLabelFont(QFont(font().family(), 10));
    _Qh->yAxis->setTickLabelFont(QFont(font().family(), 10));
    _Qh->xAxis->setLabel(_x_label);
    _Qh->xAxis->setLabelFont(QFont(font().family(), 11));

    _Qh->yAxis->setLabel(_y_label);
    _Qh->yAxis->setLabelFont(QFont(font().family(), 11));


    //Add a color scale:
    QCPColorScale *colorScale = new QCPColorScale(_Qh);
    _Qh->plotLayout()->addElement(0, 1, colorScale); // add it to the right of the main axis rect
    colorScale->setType(QCPAxis::atRight); // scale shall be vertical bar with tick/axis labels right (actually atRight is already the default)
    _colormap->setColorScale(colorScale); // associate the color map with the color scale
    //colorScale->axis()->setLabel("Magnetic Field Strength");


    colorScale->setBarWidth(15);
    colorScale->axis()->setPadding(2);
    //colorScale->axis()->setAutoTickCount(4);
    colorScale->axis()->setTickLabelFont(QFont(font().family(), 10));
    colorScale->axis()->setLabel(_z_label);
    colorScale->axis()->setLabelFont(QFont(font().family(), 11));

    //Set the color gradient of the color map to one of the presets:
    _colormap->setGradient(QCPColorGradient::gpHot);


    //Make sure the axis rect and color scale synchronize their bottom and top margins (so they line up):
    QCPMarginGroup *marginGroup = new QCPMarginGroup(_Qh);
    _Qh->axisRect()->setMarginGroup(QCP::msBottom|QCP::msTop, marginGroup);
    colorScale->setMarginGroup(QCP::msBottom|QCP::msTop, marginGroup);


    //Set the gap between the axis and colorbar.
    _Qh->axisRect()->setMinimumMargins(QMargins(3, 3, 3, 3)); // maybe only set right margin to 0,
    _Qh->yAxis2->setPadding(0);
    _Qh->yAxis->setTickLabelPadding(0);
    _Qh->plotLayout()->setColumnSpacing(4);


    //Rescale all axes.
    _Qh->rescaleAxes();
    _colormap->rescaleDataRange();
    _colormap->setInterpolate(false);
    _Qh->xAxis->setTickLengthOut(3);
    _Qh->yAxis->setTickLengthOut(3);
    _Qh->xAxis2->setTickLengthOut(3);
    _Qh->yAxis2->setTickLengthOut(3);
//    _Qh->xAxis->setTickLabels(false);
//    _Qh->yAxis->setTickLabels(false);

//    _Qh->xAxis->setAutoTickCount(3);
//    _Qh->yAxis->setAutoTickCount(3);
//    colorScale->axis()->setAutoTickCount(3);


    //Range and other axis esthetics.
    _Qh->axisRect()->setupFullAxesBox(true);
    //get_nonzero_range(xmin, xmax, ymin, ymax);
    _Qh->xAxis->setRange(xmin, xmax);
    _Qh->yAxis->setRange(ymin, ymax);
}






//-----------------------------------------------------------------------------

void twoDhist_widget::get_nonzero_range(double & xmin, double & xmax, double & ymin, double & ymax){
    double dummy;
    QCPColorMapData * d= _colormap->data();



    //xmin __________________________________

    for (int ix = 0; ix<d->keySize(); ix++){
        bool broken = false;
        for (int iy = 0; iy<d->valueSize(); iy++){
            if (d->cell(ix, iy) > 0.0){
                broken = true;
                break;
            }
        }
        if (broken){
            d->cellToCoord(ix, 0, &xmin, &dummy);
            break;
        }
    }



    //ymax __________________________________

    for (int iy = 0; iy<d->valueSize(); iy++){
        bool broken = false;
        for (int ix = 0; ix<d->keySize(); ix++){
            if (d->cell(ix, iy) > 0.0){
                broken = true;
                break;
            }
        }
        if (broken){
            d->cellToCoord(0, iy, &dummy, &ymax);
            break;
        }
    }




    //xmax__________________________________

    for (int ix = d->keySize(); ix>0; ix--){
        bool broken = false;
        for (int iy = 0; iy<d->valueSize(); iy++){
            if (d->cell(ix, iy) > 0.0){
                broken = true;
                break;
            }
        }
        if (broken){
            d->cellToCoord(ix, 0, &xmax, &dummy);
            break;
        }
    }



    //ymax__________________________________

    for (int iy = d->valueSize(); iy>0; iy--){
        bool broken = false;
        for (int ix = 0; ix<d->keySize(); ix++){
            if (d->cell(ix, iy) > 0.0){
                broken = true;
                break;
            }
        }
        if (broken){
            d->cellToCoord(0, iy, &dummy, &ymin);
            break;
        }
    }

    _nonzero_xmin = xmin;
    _nonzero_ymin = ymin;
    _nonzero_xmax = xmax;
    _nonzero_ymax = ymax;

}






//-----------------------------------------------------------------------------

void twoDhist_widget::add_stats_box(){
    //Add a small stats box on the plot.

    std::stringstream ss_up, ss_low;
    ss_low << _h->GetBinContent(_h->GetBin(0));
    ss_up << _h->GetBinContent(_h->GetBin(_h->GetNbinsX()));
    std::string Statsbox_text = "<" + ss_low.str() + "\n>" + ss_up.str();


    //Add to the plot.
    QCPItemText * Statsbox = new QCPItemText(_Qh);
    _Qh->addItem(Statsbox);


    //Set styles and position.
    Statsbox->position->setType(QCPItemPosition::ptAxisRectRatio);
    Statsbox->setPositionAlignment(Qt::AlignRight|Qt::AlignTop);
    Statsbox->position->setCoords(0.97, 0.03);
    Statsbox->setTextAlignment(Qt::AlignLeft);
    Statsbox->setFont(QFont(font().family(), 11));
    Statsbox->setPadding(QMargins(2, 2, 2, 2));
    Statsbox->setText(QString(Statsbox_text.c_str()));
    Statsbox->setPen(QPen(Qt::black));
    Statsbox->setBrush(QBrush(Qt::white));
    Statsbox->setSelectable(true);
}






//-----------------------------------------------------------------------------

void twoDhist_widget::toggle_axes_labels(){
    if (_Qh->xAxis->label() != _x_label) _Qh->xAxis->setLabel(_x_label);
    else _Qh->xAxis->setLabel(QString(""));

    if (_Qh->yAxis->label() != _y_label) _Qh->yAxis->setLabel(_y_label);
    else _Qh->yAxis->setLabel(QString(""));

    if (_colormap->colorScale()->axis()->label() != _z_label) _colormap->colorScale()->axis()->setLabel(_z_label);
    else _colormap->colorScale()->axis()->setLabel(QString(""));

    if (!_Qh->xAxis->tickLabels()){
        _Qh->xAxis->setTickLabels(true);
        _Qh->yAxis->setTickLabels(true);
    }

    else {
        _Qh->xAxis->setTickLabels(false);
        _Qh->yAxis->setTickLabels(false);
    }

    _Qh->replot();
}




//-----------------------------------------------------------------------------

void twoDhist_widget::show_axes_labels(){
    _Qh->xAxis->setLabel(_x_label);

    _Qh->yAxis->setLabel(_y_label);

    _colormap->colorScale()->axis()->setLabel(_z_label);


	_Qh->xAxis->setTickLabels(true);
	_Qh->yAxis->setTickLabels(true);

    _Qh->replot();
}






//-----------------------------------------------------------------------------

void twoDhist_widget::toggle_ref_plot(){
    std::cout<<"No reference provided."<<std::endl;
}
