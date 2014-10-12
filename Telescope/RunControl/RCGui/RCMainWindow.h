#ifndef RCMAINWINDOW_H
#define RCMAINWINDOW_H

#include <QMainWindow>
#include <QGridLayout>
#include <vector>
#include <sstream>
#include "../../DQM/GUI/qcustomplot.h"
#include <QPen>
#include <QTimer>
#include <QDateTime>
#include <QSpinBox>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fstream>
#include <iostream>
#include <arpa/inet.h>
#include <errno.h>
#include <thread>



//_____________________________________________________________________________

namespace Ui {
class RCMainWindow;
}


//_____________________________________________________________________________

class RCMainWindow : public QMainWindow {
    Q_OBJECT
public:
    // Members.
    Ui::RCMainWindow *ui; // Window.
    int m_nMaxServers;
    int m_tluSockfd;
    int m_numDaqConn;   // number of available DAQ connections according to config file
    int m_numDaqServers;   // number of DAQ servers responding
    std::vector<int> m_daqSockfd;
    struct sockaddr_in m_tluAddr;
    std::vector<struct sockaddr_in> m_daqAddr;
    QWidget * m_plots;
    QGridLayout * m_plotsLay;
    QCustomPlot * p_temp;
    QCustomPlot * p_file;
    QTimer * m_tempTimer;
    QTimer * m_fileTimer;
    int m_tempPlotFillRate;
    int m_filePlotFillRate;
    bool m_visiblePlotHolder;
    int m_nChips;
    std::vector<QPen> m_pens;
    float m_penWidth;
    int m_tempSampleNum;
    int m_fileSampleNum;
    float m_tempHistoryMins;
    float m_fileHistoryMins;
    std::vector<QSpinBox*> m_zBoxs;


    // Current status indicators.
    bool m_tluServerActive;
    std::vector<bool> m_daqServerActive;


    // Methods.
    explicit RCMainWindow(QWidget *parent = 0);
    ~RCMainWindow();
    void GUIOutput(std::string);
    bool getAddresses();
    bool connectTLU();
    bool connectDAQs();
    void sendTLUCommand(std::string);
    void sendDAQCommand(std::string);
    void setupTempPlot();
    void setupFilePlot();
    void addPlotHolder();
    void setupPens();
    std::string rad36(int);
    std::string zPosnsEncode();
    void setupZrecord();
    void updateZrecord(bool);
    void launchQMs(std::string);
    void getDAQmon(int, std::string *, bool *);
    std::vector<std::string> split_line(std::string, int);
    bool m_monitoring;


public slots:
    void on_b_init_clicked();
    void on_b_startRun_clicked();
    void on_b_endRun_clicked();
    void on_b_configure_clicked();
    void b_tempStart_clicked();
    void b_tempEnd_clicked();
    void b_fileStart_clicked();
    void b_fileEnd_clicked();
    void on_b_startMonitoring_clicked();
    void on_b_endMonitoring_clicked();
    void tempUpdate();
    void fileUpdate();
    void on_b_quit_clicked();
    void on_b_forceSaveRecords_clicked();
    void on_b_DQM_clicked();
    void on_b_FQM_clicked();
};


//_____________________________________________________________________________


#endif // RCMAINWINDOW_H
