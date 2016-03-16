#include "qcstmplothistogram.h"

#include <QMap>
#include <QPair>

//QCstmPlotHistogram::QCstmPlotHistogram(QWidget*& parent)
//    : QCustomPlot ( parent )
QCstmPlotHistogram::QCstmPlotHistogram(QWidget * parent)
    : QCustomPlot ( parent )
{
    //setMouseTracking(false);
    this->setParent( parent );
    this->addLayer("back");
    this->addLayer("front");
    this->addLayer("overlay");
    //hist = 0;
    //this->addGraph();
    setInteractions(QCP::iRangeDrag|QCP::iRangeZoom);
    axisRect()->setupFullAxesBox(true);
    xAxis->setLabel("signal");
    yAxis->setLabel("count");
    //this->graph(0)->setLineStyle(QCPGraph::lsStepCenter);
    //this->graph(0)->setPen(QPen(Qt::black));
    lowClamp = new QCPItemStraightLine(this);    highClamp = new QCPItemStraightLine(this);
    lowClamp->setLayer("overlay"); highClamp->setLayer("overlay");
    lowClamp->setPen(QPen(Qt::blue)); highClamp->setPen(QPen(Qt::blue));
    lowClamp->point1->setCoords(-DBL_MAX,0); lowClamp->point2->setCoords(-DBL_MAX,1);
    highClamp->point1->setCoords(DBL_MAX,0); highClamp->point2->setCoords(DBL_MAX,1);
    this->addItem(lowClamp); this->addItem(highClamp);


    // Being aware of changes in the scale type for histogram
    connect( this->yAxis, SIGNAL(scaleTypeChanged(QCPAxis::ScaleType)),
             this, SLOT(on_scaleTypeChanged(QCPAxis::ScaleType)) );
}

QCstmPlotHistogram::~QCstmPlotHistogram()
{
    this->clear();
    //delete hist;
}

/**
 * @brief QCstmPlotHistogram::mouseDoubleClickEvent.  This method
 *  will recover the proper strecht in Y (full image visible in the
 *  Y-range) if the user wants it.
 *
 * @param event
 */
void QCstmPlotHistogram::mouseDoubleClickEvent(QMouseEvent *event)
{
    QCustomPlot::mousePressEvent(event);
    if(event->button() == Qt::LeftButton)
    {

        // see that there are histograms
        if ( m_currentHist == -1 ) return;

        // Get the max
        int ymax = getYMaxCount( getCurrentThreshold() );

        // Change range
        if ( _scaleTypeHistogram == QCPAxis::stLinear ) {
            this->yAxis->setRangeLower( 0 );
        } else {
            this->yAxis->setRangeLower( __range_min_whenLog );
        }

        this->yAxis->setRangeUpper( ymax );

        // replot
        this->replot( QCustomPlot::rpQueued);

    }
}

void QCstmPlotHistogram::setHistogram(int threshold, QVector<int> data){
    if ( threshold < 0 ) return;
    setHistogram(threshold, data.data(), data.size());
}

void QCstmPlotHistogram::setHistogram(int threshold, int *data, int size){
    if ( threshold < 0 ) return;
    int index;
    if(m_mapping.contains(threshold) ){
        index =m_mapping[threshold].first;
        delete m_mapping[threshold].second;
    }
    else
        index = generateGraph();

    //QElapsedTimer timer;
    //timer.start();
    //hist->setWidth(m_binSize);
    int min = INT_MAX, max = INT_MIN;
    for(int i = 0; i < size; i++){
        if(data[i] < min)
            min = data[i];
        if(data[i] > max)
            max = data[i];
    }

    // This could happen
    if ( min == INT_MIN ) min = 0;

    int binWidth = (max-min)/m_binCount == 0? 1 : (max-min)/m_binCount;
    Histogram *hist = new Histogram(min, max, binWidth);
    hist->addRange(data, size);
    //qDebug() << "Histogram creation took" << timer.elapsed() << "milliseconds";

    m_mapping[threshold] = qMakePair(index, hist);
    setPlot(index, hist);
}

/*void QCstmPlotHistogram::addHistogram(int threshold, QVector<int> data){
  addHistogram(threshold, data.data(), data.size());
}

void QCstmPlotHistogram::addHistogram(int threshold, int *data, int size){
  if(!m_mapping.contains(threshold))
    return setHistogram(threshold, data, size);
  m_mapping[threshold].second += Histogram(data, size);
  setPlot(m_mapping[threshold].first, m_mapping[threshold].second);
}*/

int QCstmPlotHistogram::generateGraph(){
    QCPGraph *newGraph = addGraph();
    newGraph->setPen(QPen(Qt::gray));
    newGraph->setLayer("back");
    newGraph->setLineStyle(QCPGraph::lsStepRight);
    return graphCount()-1;
}

void QCstmPlotHistogram::setPlot(int index, Histogram *hist){
    //QElapsedTimer timer;

    //timer.start();
    QCPGraph *graph = this->graph(index);
    graph->clearData();
    int i;
    int sample, oldSample = hist->atIndex(0)-1;
    for( i = 0; i < hist->size(); i++){
        sample = hist->atIndex(i);
        if(sample != oldSample){
            graph->addData(i*hist->getWidth()+hist->getMin(), ((double)sample));
            oldSample = sample;
        }
    }
    graph->addData(i*hist->getWidth()+hist->getMin(), ((double)hist->atIndex(i-1)));
    graph->rescaleAxes();
    replot(QCustomPlot::rpQueued);
    //qDebug() << "Histogram plot took " << timer.elapsed() << "milliseconds";
}

int QCstmPlotHistogram::getCurrentThreshold()
{

    QList<int> keys = m_mapping.keys();
    if ( ! keys.empty() ) return keys[m_currentHist];

    return 0;
}

void QCstmPlotHistogram::scaleToInterest(){
    double x0 = lowClamp->point1->coords().x();
    double x1 = highClamp->point1->coords().x();
    double delta = x1-x0;
    this->xAxis->setRange(x0-delta*0.1, x1+delta*0.1);
    this->replot(QCustomPlot::rpQueued);
}

/*unsigned QCstmPlotHistogram::getTotal(int threshold){
  Histogram *hist = m_mapping[threshold].second;
  unsigned sum = 0;
  for(int i = hist->getMin(); i <= hist->getMax(); i++ )
    sum += hist->at(i);
  return sum;
}*/

void QCstmPlotHistogram::rebin(){
    /*for(int i = 0; i < m_mapping.size();i++)
    setPlot(m_mapping.values()[i].first, m_mapping.values()[i].second);*/
}


void QCstmPlotHistogram::setActive(int index){
    if(this->graphCount() == 0)
        return;
    if(-1 == index)
        index = this->graphCount()-1;
    if(m_currentHist >= 0){
        this->graph(m_currentHist)->setPen(QPen(Qt::gray));
        this->graph(m_currentHist)->setLayer("back");
    }
    m_currentHist = index;
    this->graph(m_currentHist)->setPen(QPen(Qt::red));
    this->graph(m_currentHist)->setLayer("front");
    //this->graph(currentHist)->setLayer()
    this->graph(m_currentHist)->rescaleAxes();
    replot(QCustomPlot::rpQueued);
}

void QCstmPlotHistogram::clear(){
    this->clearGraphs();
    for(auto it = m_mapping.begin(); it != m_mapping.end(); it++)
        delete it.value().second;
    m_mapping.clear();
    m_currentHist = -1;
    this->replot(QCustomPlot::rpQueued);
}

void QCstmPlotHistogram::changeRange(QCPRange newRange){

    minClampChanged(newRange.lower);
    maxClampChanged(newRange.upper);

    emit rangeChanged(newRange);
}

void QCstmPlotHistogram::minClampChanged(double min){
    lowClamp->point1->setCoords(min,0); lowClamp->point2->setCoords(min,1);
    replot(QCustomPlot::rpQueued);
}
void QCstmPlotHistogram::maxClampChanged(double max){

    highClamp->point1->setCoords(max,0); highClamp->point2->setCoords(max,1);
    // It is neccesary to set the RefreshPriority to rpQueued
    //  here because this get's too slow.
    // TODO ! May be to see later with gamma-ray
    //  why this replot triggers signals in the heatmap (QCstmGLPlot)
    replot(QCustomPlot::rpQueued);
}

void QCstmPlotHistogram::set_scale_full(int threshold){
    if ( threshold < 0 ) return;
    int index = m_mapping[threshold].first;
    if(this->graphCount() == 0)
        return;
    this->changeRange(QCPRange(this->graph(index)->data()->begin().key(), (this->graph(index)->data()->end()-1).key()));
    this->replot(QCustomPlot::rpQueued);
}

void QCstmPlotHistogram::set_scale_percentile(int threshold, double lowerPercentile, double upperPercentile){
    if ( threshold < 0 ) return;
    if(this->graphCount() == 0)
        return;
    Histogram *hist = m_mapping[threshold].second;
    uint64_t total = 0, partialSum  = 0;
    for(int i = 0; i < hist->size(); i++)
        total += hist->atIndex(i);
    uint64_t minBound = lowerPercentile*total, maxBound = upperPercentile*total;
    double lowerBound, upperBound;
    int index = 0;
    do{
        partialSum += hist->atIndex(index++);
    }while(partialSum < (unsigned)minBound);
    lowerBound = hist->getMin()+(index-1)*hist->getWidth();
    while(partialSum < (unsigned)maxBound)
        partialSum += hist->atIndex(index++);
    upperBound = hist->getMin()+(index)*hist->getWidth();;
    this->changeRange(QCPRange(lowerBound, upperBound));
}

void QCstmPlotHistogram::mouseReleaseEvent(QMouseEvent *event){
    QCustomPlot::mouseReleaseEvent(event);
    if((event->button() == Qt::RightButton) && clicked)
    {
        clicked = false;
        xReleased = xAxis->pixelToCoord(event->x());
        maxClampChanged(xReleased);
        if(xReleased < xClicked){
            double tmp = xReleased;
            xReleased = xClicked;
            xClicked = tmp;
        };
        //this->changeRange(QCPRange(xClicked, xReleased));
        emit new_range_dragged(QCPRange(xClicked, xReleased));
    }
}

void QCstmPlotHistogram::on_scaleTypeChanged(QCPAxis::ScaleType st)
{
    _scaleTypeHistogram = st;
}
