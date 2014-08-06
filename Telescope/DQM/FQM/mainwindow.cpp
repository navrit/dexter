#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <iostream>
#include <sys/stat.h>
#include <sstream>
#include <dirent.h>
#include <fstream>
#include <unistd.h>
#include <sstream>

MainWindow::MainWindow(int runNumber) :
    QMainWindow(),
    ui(new Ui::MainWindow) {
    
    m_samplePeriod = 0.15; //s
    m_timeWidth = 120; 
    m_sampleNum = 0;
    m_nPlanes = 8;
    for (unsigned int i=0; i<m_nPlanes; i++) m_fileNames.push_back(std::vector<std::string>());
    ui->setupUi(this);
    _Qg = NULL;
    m_runNumber = runNumber;
    //_Qg->setAttribute(Qt::WA_PaintOnScreen);

    // Setup the plot (append later)
    _Qg = new QCustomPlot();
    for (int i=0; i<m_nPlanes; i++) {
        QPen p;
        p.setWidthF(2);
        p.setColor(QColor(sin(i*0.6)*100+100, sin(i*1.2 + 0.7)*100+100, sin(i*0.6 + 0.6)*100+100));
        std::stringstream ss; ss<<i;
        _Qg->addGraph();
        _Qg->graph()->setPen(p);
    }


    // Scatter points.
    for (int i=0; i<m_nPlanes; i++) {
        QPen p(QColor(sin(i*0.6)*100+100, sin(i*1.2 + 0.7)*100+100, sin(i*0.6 + 0.6)*100+100));
        p.setWidthF(2);
        QBrush b(QColor(sin(i*0.6)*100+100, sin(i*1.2 + 0.7)*100+100, sin(i*0.6 + 0.6)*100+100));
        std::stringstream ss; ss<<i;
        _Qg->addGraph();
        _Qg->graph()->setName(("Dev" + ss.str()).c_str());
        _Qg->graph()->setLineStyle(QCPGraph::lsNone);

        double scattersize = 9;
        if (i==0) _Qg->graph()->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCross, p, b, scattersize));
        if (i==1) _Qg->graph()->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssSquare, p, b, scattersize));
        if (i==2) _Qg->graph()->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCircle, p, b, scattersize));
        if (i==3) _Qg->graph()->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssPlus, p, b, scattersize));
        if (i==4) _Qg->graph()->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssDiamond, p, b, scattersize));
        if (i==5) _Qg->graph()->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssStar, p, b, scattersize));
        if (i==6) _Qg->graph()->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssTriangle, p, b, scattersize));
        if (i==7) _Qg->graph()->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssTriangleInverted, p, b, scattersize));
        if (i==8) _Qg->graph()->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssPeace, p, b, scattersize));
        if (i==9) _Qg->graph()->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCross, p, b, scattersize));
    }

    _Qg->xAxis->setTickLabelType(QCPAxis::ltDateTime);
    _Qg->xAxis->setDateTimeFormat("hh:mm:ss");
    //_Qg->axisRect()->setupFullAxesBox();
    _Qg->xAxis->setLabel("Sample Time since t_start");
    _Qg->yAxis->setLabel("File Size (Mbs)");
    _Qg->legend->setVisible(true);
    _Qg->legend->setBrush(QBrush(QColor(255, 255, 255, 100)));
    _Qg->axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignTop|Qt::AlignLeft);
    for (int i=0; i<m_nPlanes; i++) _Qg->legend->removeItem(0);

    _Qg->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectAxes |
                          QCP::iSelectLegend);
    
    
    
    _Qg->setNotAntialiasedElements(QCP::aeGrid);
    _Qg->setNotAntialiasedElements(QCP::aeSubGrid);
    _Qg->setNotAntialiasedElements(QCP::aeLegend);
    _Qg->setNotAntialiasedElements(QCP::aeFills);
    _Qg->setNotAntialiasedElements(QCP::aeZeroLine);
    _Qg->setNotAntialiasedElements(QCP::aePlottables);
    QFont font; 
    font.setStyleStrategy(QFont::NoAntialias); 
    _Qg->xAxis->setTickLabelFont(font); 
    _Qg->yAxis->setTickLabelFont(font); 
    _Qg->legend->setFont(font);
    
    
    
    on_b_reScan_clicked();
    timer_update();
    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(timer_update()));
    timer->start(1000*m_samplePeriod);


    delete ui->FQMholder->layout();
    QGridLayout * lay = new QGridLayout();
    lay->addWidget(_Qg);
    ui->FQMholder->setLayout(lay);
    this->resize(450,950);


    connect(_Qg, SIGNAL(mouseWheel(QWheelEvent*)), this, SLOT(mouseWheel()));
    connect(_Qg, SIGNAL(mousePress(QMouseEvent*)), this, SLOT(mousePress(QMouseEvent*)));
}


//_____________________________________________________________________________

MainWindow::~MainWindow() {
    delete ui;
}


//_____________________________________________________________________________

void MainWindow::timer_update() {
    if (!ui->c_pauseUpdate->isChecked()) {
        double key = m_sampleNum * m_samplePeriod;
        if (m_sampleNum%200 == 0 && m_sampleNum != 0) on_b_reScan_clicked();

        for (int i=0; i<m_nPlanes; i++) {
            if (m_sampleNum%50 == 0 && m_sampleNum != 0) {
                _Qg->graph(i+m_nPlanes)->removeDataBefore(key - m_timeWidth);
                _Qg->graph(i)->removeDataBefore(key - m_timeWidth);
            }

            double fileSize = getFileSize(i);
            _Qg->graph(i)->addData(key, fileSize);
            if (m_sampleNum%800 == 30) _Qg->graph(i+m_nPlanes)->addData(key, fileSize);
        }
        _Qg->xAxis->setAutoTickCount(4);
        _Qg->xAxis->setTickLabelRotation(30);
       
        _Qg->yAxis->rescale();
        double diff = _Qg->yAxis->range().upper - _Qg->yAxis->range().lower;
        _Qg->yAxis->setRange(_Qg->yAxis->range().upper + 0.1*diff, _Qg->yAxis->range().lower - 0.1*diff);
        _Qg->xAxis->rescale();

        if (m_sampleNum % 2==0) _Qg->replot();
    }
    m_sampleNum++;
}


//_____________________________________________________________________________

double MainWindow::getFileSize(int iPlane) {
    double x = 0.0;
    std::vector<std::string>::iterator iFile;
    for (iFile = m_fileNames[iPlane].begin(); iFile != m_fileNames[iPlane].end(); iFile++) {
        std::ifstream in((*iFile).c_str(), std::ifstream::ate | std::ifstream::binary);
        if (in) {
            x += (double)(in.tellg()/(1000000.0));
            in.close();
        }
        else std::cout<<"Failed to open file: "<<(*iFile)<<std::endl;
    }

    return x;
}


//_____________________________________________________________________________

void MainWindow::on_b_reScan_clicked() {
    std::cout<<"Scanning for new files."<<std::endl;
    for (unsigned int ichip=0; ichip<m_nPlanes; ichip++) {
        std::stringstream ssPlane; ssPlane<<ichip;
        std::stringstream ssRun; ssRun<<m_runNumber;
        m_fileNames[ichip].clear();
        std::string direcName = "/mnt/DATA/Dev" + ssPlane.str() + "/Run" + ssRun.str() + "/";
        DIR* dir = opendir(direcName.c_str());
        if (dir == NULL) {
            std::cout<<"Unable to open data directory: "<<direcName<<std::endl;
            exit (EXIT_FAILURE);
        }

        // Loop over entries.
        struct dirent* entry;
        while ((entry = readdir(dir)) != NULL) {
            int i = strlen(entry->d_name);
            std::string tempFileName = std::string(entry->d_name);
            if (i<7) continue;
            std::string stringCheck = ".dat";
            if (tempFileName.substr(i-4, i-1) == stringCheck) 
                m_fileNames[ichip].push_back(direcName + tempFileName);
        }
    }
}



//_____________________________________________________________________________

void MainWindow::mouseWheel(){
    if (!ui->c_pauseUpdate->isChecked()) {
        if (_Qg->xAxis->selectedParts().testFlag(QCPAxis::spAxis))
        _Qg->axisRect()->setRangeZoom(_Qg->xAxis->orientation());
        else if (_Qg->yAxis->selectedParts().testFlag(QCPAxis::spAxis))
        _Qg->axisRect()->setRangeZoom(_Qg->yAxis->orientation());
        else
        _Qg->axisRect()->setRangeZoom(Qt::Horizontal|Qt::Vertical);
    }
}


//_____________________________________________________________________________

void MainWindow::mousePress(QMouseEvent* e){
    if (!ui->c_pauseUpdate->isChecked()) {
        if (_Qg->xAxis->selectedParts().testFlag(QCPAxis::spAxis))
            _Qg->axisRect()->setRangeDrag(_Qg->xAxis->orientation());
        else if (_Qg->yAxis->selectedParts().testFlag(QCPAxis::spAxis))
            _Qg->axisRect()->setRangeDrag(_Qg->yAxis->orientation());
        else  _Qg->axisRect()->setRangeDrag(Qt::Horizontal|Qt::Vertical);
    }
}


//_____________________________________________________________________________

