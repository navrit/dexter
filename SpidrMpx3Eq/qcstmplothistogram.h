#ifndef QCSTMPLOTHISTOGRAM_H
#define QCSTMPLOTHISTOGRAM_H

#include <QObject>
#include <QWidget>
#include <QResizeEvent>
#include <float.h>
#include "qcustomplot.h"

#include "histogram.h"

#define __range_min_whenLog  1.0

class QCstmPlotHistogram : public QCustomPlot
{
    Q_OBJECT

public:
    //explicit QCstmPlotHistogram(QWidget* &parent);
    explicit QCstmPlotHistogram(QWidget * parent);
    ~QCstmPlotHistogram();

    QMap<int, QPair<int, Histogram*>> m_mapping;
    int m_binCount = 1;
    int m_currentHist = -1;
    bool clicked = false;
    double xClicked = 0, xReleased =0;
    //QVector<QVector<double>> xHist, yHist;
    QCPItemStraightLine *lowClamp = nullptr, *highClamp = nullptr;
    qreal _lastDraggPoint = 0.;
    QCPAxis::ScaleType _scaleTypeHistogram = QCPAxis::stLinear;

    void mousePressEvent(QMouseEvent *event){//TODO: check if clicked inside graph.
        QCustomPlot::mousePressEvent(event);
        if(event->button() == Qt::RightButton)
        {
            clicked = true;
            xClicked = xAxis->pixelToCoord(event->x());
            minClampChanged(xClicked);
        }
    }

    void mouseDoubleClickEvent(QMouseEvent *event);

    void mouseMoveEvent(QMouseEvent *event){ //TODO: don't update actual range, only markers. Update range on release.
        QCustomPlot::mouseMoveEvent(event);
        if(clicked)
            maxClampChanged(xAxis->pixelToCoord(event->x()));
    }


    void mouseReleaseEvent(QMouseEvent *event);
    void rebin();
    int generateGraph();

public:

    void setHistogram(int threshold, int* data, int size);
    void setHistogram(int threshold, QVector<int> data);

    void addHistogram(int threshold, int* data, int size);
    void addHistogram(int threshold, QVector<int> data);

    void setPlot(int index, Histogram *hist);

    int getMin(int threshold){
        if (threshold < 0) return 0;
        return m_mapping[threshold].second->getMin();
    }
    int getMax(int threshold){
        if (threshold < 0) return 0;
        return m_mapping[threshold].second->getMax();
    }
    int getCurrentThreshold();

    unsigned getYMaxCount(int threshold){
        if (threshold < 0) return 0;
        return m_mapping[threshold].second->getYMaxCount();
    }
    unsigned getBin(int threshold, int level){
        if (threshold < 0) return 0;
        return m_mapping[threshold].second->at(level);
    }
    unsigned getTotal(int threshold);
    double getRangeMinWhenLog(){return __range_min_whenLog; }

    void clear();
    int getHistogramCount(){return m_mapping.size();}
    Histogram* getHistogram(int threshold){
        if (threshold < 0) return nullptr;
        return m_mapping[threshold].second;
    }
    void scaleToInterest();
signals:
    void rangeChanged(QCPRange newRange);
    void new_range_dragged(QCPRange NewRange);

public slots:
    void set_scale_full(int threshold);
    void set_scale_percentile(int threshold, double lower, double upper);
    void setActive(int index);
    void changeRange(QCPRange newRange);
    void minClampChanged(double min);
    void maxClampChanged(double max);
    void changeBinCount(int binCount){m_binCount = binCount;}
    void on_scaleTypeChanged(QCPAxis::ScaleType st);

    //void rebinHistograms(int binSize)
};

#endif // QCSTMPLOTHISTOGRAM_H
