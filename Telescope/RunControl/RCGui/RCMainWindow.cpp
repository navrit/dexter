#include "RCMainWindow.h"
#include "ui_RCMainWindow.h"


//_____________________________________________________________________________

RCMainWindow::RCMainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::RCMainWindow),
    m_nMaxServers(8),
    m_tluSockfd(0),
    m_numDaqConn(0),
    m_numDaqServers(0),
    m_tluServerActive(false){

	//this->setStyleSheet("QGroupBox{border:2px solid lightgray;border-radius:5px;margin-top: 1ex;} QGroupBox::title{subcontrol-origin: margin;subcontrol-position:top centre;padding:0 3px;}");

    // Constructor.
    ui->setupUi(this);  
//    this->setStyleSheet("QTextBrowser{ font-size:11px }");
    ui->l_logo->setStyleSheet("QWidget{ font-size:25px }");
    ui->t_header->clear();
    //ui->t_header->setStyleSheet("QWidget{font-size:12px}");
    ui->t_header->append("Default header text.");

    m_nChips = 9;
    m_penWidth = 2.5;
    m_tempSampleNum = 0;
    m_fileSampleNum = 0;
    m_tempHistoryMins = 0.5;
    m_fileHistoryMins = 1;

    // More default values.
    for (int i=0; i<m_nMaxServers; i++) {
      struct sockaddr_in ith_daqAddr;
      m_daqAddr.push_back(ith_daqAddr);
      m_daqServerActive.push_back(false);
      m_daqSockfd.push_back(0);
    }

    p_temp = NULL;
    p_file = NULL;
    m_tempTimer = NULL;
    m_fileTimer = NULL;
    m_visiblePlotHolder = false;


    // GUI default settings.
    ui->b_runNumber->setValue(2000);
    ui->t_header->setReadOnly(false);

    m_tempPlotFillRate = 500;
    m_filePlotFillRate = 100;

    int w = 480;
    ui->b_actions->setMaximumWidth(w);
    ui->b_settings->setMaximumWidth(w);
    ui->b_output->setMaximumWidth(w);
    ui->b_monitoring->setMaximumWidth(w);
    ui->l_logo->setMaximumWidth(w);

    setupPens();
    m_zBoxs.push_back(ui->b_z0);
    m_zBoxs.push_back(ui->b_z1);
    m_zBoxs.push_back(ui->b_z2);
    m_zBoxs.push_back(ui->b_z3);
    m_zBoxs.push_back(ui->b_z4);
    m_zBoxs.push_back(ui->b_z5);
    m_zBoxs.push_back(ui->b_z6);
    m_zBoxs.push_back(ui->b_z7);
    m_zBoxs.push_back(ui->b_z8);
    m_zBoxs.push_back(ui->b_z9);

    setupZrecord();
    m_monitoring = false;
}


//_____________________________________________________________________________

void RCMainWindow::on_b_forceSaveRecords_clicked() {
	updateZrecord(false);
}


//_____________________________________________________________________________

void RCMainWindow::updateZrecord(bool increment) {
	GUIOutput("[Note] updating z record");
	std::ofstream myfile ("/home/tpx3/DQM_files/rcZrecord.txt", std::ios::out);
	if (myfile.is_open()) {
		std::stringstream ssRun;
		if (increment) ssRun << ui->b_runNumber->value() + 1;
		else ssRun << ui->b_runNumber->value();
		std::string sOut = ssRun.str() + "\n";
		myfile << sOut;

		for (int i=0; i<10; i++){
			if (m_zBoxs[i]->value() != -1) {
				std::stringstream ss;
				ss << m_zBoxs[i]->value();
				std::string sOut = ss.str() + "\n";
				myfile << sOut;
			}
		}
	}
	else GUIOutput("[Warning] Unable to open rc record file for writing - will continue");
}


//_____________________________________________________________________________

void RCMainWindow::setupZrecord() {
	std::string line;
	std::ifstream myfile ("/home/tpx3/DQM_files/rcZrecord.txt");
	int iline = 0;
	bool success = false;
	if (myfile.is_open()) {
		success = true;
		while (getline (myfile,line)) {
			if (iline == 0) ui->b_runNumber->setValue(atoi(line.c_str()));
			else m_zBoxs[iline-1]->setValue(atoi(line.c_str()));
			iline++;
		}
		myfile.close();
	}
    else GUIOutput("Unable to open z position record - will continue with guess and update at run start");
	if (success) for (int i = iline-1; i<10; i++) m_zBoxs[i]->setValue(-1);
}


//_____________________________________________________________________________

void RCMainWindow::setupPens() {
    m_pens.push_back(QPen(Qt::darkCyan));
    m_pens.push_back(QPen(Qt::black));
    m_pens.push_back(QPen(Qt::red));
    m_pens.push_back(QPen(Qt::green));
    m_pens.push_back(QPen(Qt::blue));

    m_pens.push_back(QPen(Qt::darkCyan));
    m_pens[m_pens.size()-1].setStyle(Qt::DashLine);
    m_pens.push_back(QPen(Qt::black));
    m_pens[m_pens.size()-1].setStyle(Qt::DashLine);
    m_pens.push_back(QPen(Qt::red));
    m_pens[m_pens.size()-1].setStyle(Qt::DashLine);
    m_pens.push_back(QPen(Qt::green));
    m_pens[m_pens.size()-1].setStyle(Qt::DashLine);
    m_pens.push_back(QPen(Qt::blue));
    m_pens[m_pens.size()-1].setStyle(Qt::DashLine);

    for (int i=0; i<m_nChips; i++) m_pens[i].setWidth(m_penWidth);
}


//_____________________________________________________________________________

void RCMainWindow::addPlotHolder() {
    m_plots = new QWidget(this);
    m_plotsLay = new QGridLayout();
    m_plots->setLayout(m_plotsLay);
    QGridLayout * l = (QGridLayout*) ui->centralWidget->layout();
    l->addWidget(m_plots, 0, l->columnCount(), l->rowCount(), l->columnCount());
    m_visiblePlotHolder = true;
    this->resize(1060,1000);
}


//_____________________________________________________________________________

RCMainWindow::~RCMainWindow() {
    // Destructor.
    delete ui;
}

//_____________________________________________________________________________

void RCMainWindow::on_b_init_clicked() {
	QApplication::setOverrideCursor(Qt::WaitCursor);
	QApplication::processEvents();

    GUIOutput("[Note] initializing run contol");
    ui->b_init->setEnabled(false);

    bool success = true;

    success = getAddresses();
    success = connectTLU();
    success = connectDAQs();

    if (success) {
		ui->b_startRun->setEnabled(true);
		ui->b_endRun->setEnabled(true);
		ui->b_configure->setEnabled(true);
		ui->b_startMonitoring->setEnabled(true);
		ui->b_endMonitoring->setEnabled(true);
		ui->t_header->setEnabled(true);
		ui->b_runNumber->setEnabled(true);
    }

    GUIOutput("[Note] init of run control complete");
    QApplication::restoreOverrideCursor();
}


//_____________________________________________________________________________

bool RCMainWindow::connectTLU() {
    int retval;
    if (m_tluSockfd > 0) {
        GUIOutput("[Note] connecting to TLU server");
        retval = ::connect(m_tluSockfd, (struct sockaddr *) &m_tluAddr, sizeof(m_tluAddr));
        if (retval == 0) {
            GUIOutput("[Note] connection to TLU server established");
            m_tluServerActive = true;
        }
        else {
            std::stringstream errnoSS;
            errnoSS << errno;
            GUIOutput("[Error] connection to TLU server failed (errno " + errnoSS.str() + ")");
            GUIOutput("[Error] Please quit RunControl client");
            m_tluServerActive = false;
            return false;
        }
    }
    return true;
}


//_____________________________________________________________________________

bool RCMainWindow::connectDAQs() {
    int retval;
    for (int i=0; i<m_numDaqConn; i++) {
        if (m_daqSockfd[i] > 0) {
            std::stringstream iSS;
            iSS << i;
            std::stringstream ss;
            ss << inet_ntoa(m_daqAddr[i].sin_addr);
            GUIOutput("[Note] connecting to DAQ server " + iSS.str() + ": " + ss.str());
            retval = ::connect(m_daqSockfd[i], (struct sockaddr *) &m_daqAddr[i], sizeof(m_daqAddr[i]));
            if (retval == 0) {
                GUIOutput("[Note] connection to DAQ server " + iSS.str() + " established");
                m_daqServerActive[i] = true;
                m_numDaqServers++;
            }
            else {
                std::stringstream errnoSS;
                errnoSS << errno;
                GUIOutput("[Warning] connection to DAQ server " + iSS.str() + " failed (errno " + errnoSS.str() + ")");
                m_daqServerActive[i] = false;
                return false;
            }
        }
    }

    if (m_numDaqServers > 0) {
        if (m_numDaqServers != m_numDaqConn) {
            GUIOutput("[Warning] not all DAQ servers are running");
            GUIOutput("[Note] continuing with remaining DAQ servers");
        }
    }

    else {
        GUIOutput("[Error] no DAQ servers running");
        GUIOutput("[Error] Please quit RunControl client");
        return false;
    }
    return true;
}


//_____________________________________________________________________________

bool RCMainWindow::getAddresses() {
    int portnumber;
    int size = 128;
    char line[size];
    char ipno_str[size];
    char name[size];
    FILE * fp = fopen("/localstore/TPX3/SPIDR/software/trunk/Telescope/RunControl/ServerAddressTable.txt","r");


    // Check opened.b_include
    if (fp == NULL) {
        GUIOutput("Unable to read file.");
        return false;
    }


    // Read.
    while (fgets(line, size , fp)) {
        if (line[0] != '#') {
            sscanf(line, "%s %d %s", ipno_str, &portnumber, name);
            std::stringstream ipnoSS;
            ipnoSS << ipno_str;


            // TLU case.
            if (strstr( name, "TLU")) {
                m_tluSockfd = socket(AF_INET, SOCK_STREAM, 0);
                if (m_tluSockfd < 0) {
                    GUIOutput("[Error] failed to create TLU socket");
                    return false;
                }

                GUIOutput("[Note] creating TLU socket at ip number " + ipnoSS.str());
                m_tluAddr.sin_family = AF_INET;
                m_tluAddr.sin_port = htons(portnumber);
                inet_aton(ipno_str, &(m_tluAddr.sin_addr));
            }


            // DAQ case.
            if (strstr(name, "DAQ")) {
                m_daqSockfd[m_numDaqConn] = socket(AF_INET, SOCK_STREAM, 0);
                std::stringstream numDaqConnSS;
                numDaqConnSS << m_numDaqConn;
                if (m_daqSockfd[m_numDaqConn] < 0) {
                    GUIOutput("[Error] failed to create DAQ socket " + numDaqConnSS.str());
                    return false;
                }
                GUIOutput("[Note] creating DAQ socket " + numDaqConnSS.str() + " at ip number " + ipnoSS.str());
                m_daqAddr[m_numDaqConn].sin_family = AF_INET;
                m_daqAddr[m_numDaqConn].sin_port = htons(portnumber);
                inet_aton(ipno_str, &m_daqAddr[m_numDaqConn].sin_addr);
                m_numDaqConn++;
            }
        }
    }

    return true;
}


//_____________________________________________________________________________

void RCMainWindow::GUIOutput(std::string s) {
    ui->t_output->append(QString(s.c_str()));
    std::cout<<s<<std::endl;
}


//_____________________________________________________________________________

void RCMainWindow::on_b_startRun_clicked() {
	ui->b_startMonitoring->setEnabled(true);
	QApplication::setOverrideCursor(Qt::WaitCursor);
	QApplication::processEvents();
	updateZrecord(true);

	ui->b_startRun->setEnabled(false);
    ui->b_configure->setEnabled(false);
    m_monitoring = true;
	ui->b_startMonitoring->setEnabled(false);
    b_fileStart_clicked();
    b_tempStart_clicked();


    GUIOutput("[Note] RunControl: begin of start_run command");
    std::stringstream ssRunNum;
    ssRunNum << ui->b_runNumber->value();
    GUIOutput("[Note] Command sent:");
    std::string command = "start_run " + ssRunNum.str() + " " + ui->t_header->toPlainText().toStdString();
    if (ui->b_includeZposns->isChecked()) {
    	command += " zs";
    	command += zPosnsEncode();
    }
    GUIOutput(command);


    if (strlen(command.c_str()) < 128) {
		sendTLUCommand("setVeto");
		sendDAQCommand(command);
		sendTLUCommand("pulseT0");
		sendTLUCommand("resetVeto");
		GUIOutput("[Note] RunControl end of start_run command");
    }

    else {
    	std::stringstream ssx;
    	ssx << strlen(command.c_str());
    	GUIOutput("[Warning] header size too large: " + ssx.str() + " characters vs 128");
    	GUIOutput("Please shorten (e.g. by not including zPosns), and try again");
    	ui->b_startRun->setEnabled(true);
    	ui->b_configure->setEnabled(true);
    }

    QApplication::restoreOverrideCursor();
}


//_____________________________________________________________________________

std::string RCMainWindow::zPosnsEncode() {
	// in hexatridecimal - valid to 1296mm.
	std::string zPosns;
	for (int i=0; i<9; i++) {
		std::string zPosn;
		int v = m_zBoxs[i]->value();
		if (v==-1) continue;
		zPosn = rad36(m_zBoxs[i]->value());
		if (strlen(zPosn.c_str()) == 1) zPosn.insert(0, "0");
		zPosns += zPosn;
	}
	return zPosns;
}


//_____________________________________________________________________________

std::string RCMainWindow::rad36(int k){
    std::string retval;
    int v = k;
    if (k==0) return "0";

    while(v > 0){
        unsigned m = v%36;
        if(m <= 9) retval.insert(0,1,'0'+m);
        else retval.insert(0,1,'A'+m-10);
        v /= 36;
    }
    if (k>1296) retval += ",";
    return retval;
}

//_____________________________________________________________________________

void RCMainWindow::on_b_configure_clicked() {
	QApplication::setOverrideCursor(Qt::WaitCursor);
	QApplication::processEvents();

    GUIOutput("[Note] RunControl: begin of configure command");

    ui->b_startRun->setEnabled(false);
    ui->b_configure->setEnabled(false);

    sendTLUCommand("setVeto");
    sendDAQCommand("configure");

    ui->b_startRun->setEnabled(true);
    ui->b_configure->setEnabled(true);

    GUIOutput("[Note] RunControl: end of configure command");
    QApplication::restoreOverrideCursor();
}


//_____________________________________________________________________________

void RCMainWindow::on_b_endRun_clicked() {
	QApplication::setOverrideCursor(Qt::WaitCursor);
	QApplication::processEvents();

	m_monitoring = false;
	sendDAQCommand("stop_mon");
    //b_fileEnd_clicked();
    b_tempEnd_clicked();

    GUIOutput("[Note] RunControl: begin of stop_run command");

    sendTLUCommand("setVeto");
    sendDAQCommand("stop_run");

    ui->b_startRun->setEnabled(true);
    ui->b_configure->setEnabled(true);

    GUIOutput("[Note] RunControl: end of stop_run command");
    ui->b_runNumber->setValue(ui->b_runNumber->value() + 1);
    QApplication::restoreOverrideCursor();
    ui->b_startMonitoring->setEnabled(true);
}

//_____________________________________________________________________________

void RCMainWindow::sendTLUCommand(std::string commandStr) {
    int buffersize = 1024;
    int msglen;
    char buffer[buffersize];

    if (m_tluServerActive) {
        send(m_tluSockfd, commandStr.c_str(), strlen(commandStr.c_str()), 0);
        msglen = recv(m_tluSockfd, buffer, buffersize, 0);
        buffer[msglen] = '\0';
        GUIOutput("[Note] Message received from TLU: " + std::string(buffer));

        if (strstr(buffer, "OK")) GUIOutput("[Note]" + commandStr + " successful");
        else GUIOutput(commandStr + " FAILED");
    }
    else GUIOutput("No TLU server active");
}


//_____________________________________________________________________________

void RCMainWindow::sendDAQCommand(std::string commandStr) {
    int buffersize = 1024;
    int msglen;
    char buffer[buffersize];   // message buffer to/from servers

    for (int i=0 ; i<m_numDaqConn ; i++ )
        if (m_daqServerActive[i]) send(m_daqSockfd[i], commandStr.c_str(), strlen(commandStr.c_str()), 0);

    usleep(500);

    GUIOutput("[Note] RunControl is waiting for reply to command: " + commandStr);
    if (commandStr == "stop_mon") {
    	GUIOutput("[Note] stop_mon skips checking return values from DAQ terminals.");
    	return;
    }
    for (int i=0 ; i<m_numDaqConn ; i++ ) {
        if (m_daqServerActive[i]) {
            msglen = recv(m_daqSockfd[i], buffer, buffersize, 0);
            buffer[msglen] = '\0';
            GUIOutput("[Note] message received from DAQ: " + std::string(buffer));
        }
    }

    GUIOutput("[Note] leaving command");
}


//_____________________________________________________________________________

void RCMainWindow::b_tempStart_clicked() {
    if (m_tempTimer != NULL) delete m_tempTimer;
    if (p_temp != NULL) delete p_temp;
    setupTempPlot();
    m_plotsLay->addWidget(p_temp, 1, 0, 1, 1);

    m_tempSampleNum = 0;

    // Timer.
    m_tempTimer = new QTimer(this);
    connect(m_tempTimer, SIGNAL(timeout()), this, SLOT(tempUpdate()));
    m_tempTimer->start(m_tempPlotFillRate);
}


//_____________________________________________________________________________

void RCMainWindow::setupTempPlot() {
    if (!m_visiblePlotHolder) addPlotHolder();
    if (p_temp != NULL) {
        p_temp->clearGraphs();
        p_temp->clearItems();
    }

    p_temp = new QCustomPlot(this);
    p_temp->yAxis->setLabel("Temperature (C)");
    p_temp->xAxis->setLabel("Time Past");
    p_temp->setMinimumWidth(350);
    p_temp->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    p_temp->xAxis->setTickLabelType(QCPAxis::ltDateTime);
    p_temp->xAxis->setDateTimeFormat("hh:mm:ss");

    for (int i=0; i<m_nChips; i++) {
        p_temp->addGraph();
        std::stringstream ss;
        ss<<i;
        p_temp->graph()->setName(QString(("Chip" + ss.str()).c_str()));
        p_temp->graph()->setPen(m_pens[i]);
    }

    //p_temp->legend->setVisible(true);
    p_temp->axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignTop|Qt::AlignLeft);
    p_temp->xAxis->setAutoTickCount(5);
    p_temp->xAxis->setTickLabelRotation(30);
}


//_____________________________________________________________________________

void RCMainWindow::setupFilePlot() {
    if (!m_visiblePlotHolder) addPlotHolder();
    if (p_file != NULL) {
        p_file->clearGraphs();
        p_file->clearItems();
    }
    p_file = new QCustomPlot(this);
    p_file->yAxis->setLabel("Data taken (million packets)");
    p_file->xAxis->setLabel("Time Past");
    p_file->setMinimumWidth(600);
    p_file->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    p_file->xAxis->setTickLabelType(QCPAxis::ltDateTime);
    p_file->xAxis->setDateTimeFormat("hh:mm:ss");

    for (int i=0; i<m_nChips; i++) {
        p_file->addGraph();
        std::stringstream ss;
        ss<<i;
        p_file->graph()->setName(QString(("Server_" + ss.str()).c_str()));
        p_file->graph()->setPen(m_pens[i]);
    }

    p_file->legend->setVisible(true);
    p_file->axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignTop|Qt::AlignLeft);
    p_file->xAxis->setAutoTickCount(5);
    p_file->xAxis->setTickLabelRotation(30);
}

//_____________________________________________________________________________

void RCMainWindow::tempUpdate() {
	m_tempTimer->stop();
	if (!m_monitoring) return;
    std::string s;
    time_t  timev;
    time(&timev);
    double key = timev;
    double keyLow = key - m_fileHistoryMins*60;

    for (int i=0 ; i<m_numDaqConn ; i++ ) {
        if (m_daqServerActive[i]) {
        	bool busy = true;
        	std::thread t(&RCMainWindow::getDAQmon, this, i, &s, &busy);
        	while (busy) QCoreApplication::processEvents();
        	t.join();
        	std::stringstream lineStream;
        	lineStream << s;
        	std::string a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14;
        	lineStream >> a1 >> a2 >> a3 >> a4 >> a5 >> a6 >> a7 >> a8>> a9 >> a10 >> a11 >> a12 >> a13 >> a14;
        	std::cout<<s<<std::endl;

			p_temp->graph(2*i)->addData(key, ::atof(a5.c_str()));
			p_temp->graph(2*i+1)->addData(key, ::atof(a8.c_str()));
			p_temp->graph(2*i)->removeDataBefore(keyLow);
			p_temp->graph(2*i+1)->removeDataBefore(keyLow);

			p_file->graph(2*i)->addData(key, ::atof(a11.c_str())*1e-6);
			p_file->graph(2*i+1)->addData(key, ::atof(a14.c_str())*1e-6);
			p_file->graph(2*i)->removeDataBefore(keyLow);
			p_file->graph(2*i+1)->removeDataBefore(keyLow);
        }
    }
    p_temp->xAxis->setRange(keyLow, key);
    p_temp->yAxis->rescale();
	p_temp->yAxis->setRange(20, 100);
	p_temp->replot();

	p_file->xAxis->setRange(keyLow, key);
	p_file->yAxis->rescale();
	p_file->replot();

	m_tempSampleNum++;
	m_tempTimer->start(m_tempPlotFillRate);
}

//_____________________________________________________________________________

void RCMainWindow::getDAQmon(int i, std::string * s, bool * busy) {
	int buffersize = 1024;
    int msglen;
    char buffer[buffersize];   // message buffer to/from servers

	msglen = recv(m_daqSockfd[i], buffer, buffersize, 0);
    buffer[msglen] = '\0';
	(*s) = std::string(buffer);
	(*busy) = false;
}


//_____________________________________________________________________________

void RCMainWindow::fileUpdate() {
    double key = m_fileSampleNum * (m_filePlotFillRate/1000.);
    double keyLow = key - m_fileHistoryMins*60;
    for (int i=0; i<m_nChips; i++) p_file->graph(i)->addData(key, i);
    p_file->xAxis->setRange(keyLow, key);
    p_file->yAxis->rescale();
    p_file->replot();
    m_fileSampleNum++;
}


//_____________________________________________________________________________

void RCMainWindow::b_tempEnd_clicked() {
    disconnect(m_tempTimer, SIGNAL(timeout()), this, SLOT(tempUpdate()));
    m_tempTimer->stop();
}

//_____________________________________________________________________________

void RCMainWindow::on_b_endMonitoring_clicked() {
	QApplication::setOverrideCursor(Qt::WaitCursor);
	QApplication::processEvents();
	m_monitoring = false;
	m_tempTimer->stop();
	sendDAQCommand("stop_mon");
    //b_fileEnd_clicked();
    b_tempEnd_clicked();
    ui->b_startMonitoring->setEnabled(true);
    ui->b_configure->setEnabled(true);
    ui->b_startRun->setEnabled(true);
    QApplication::restoreOverrideCursor();
}


//_____________________________________________________________________________

void RCMainWindow::on_b_startMonitoring_clicked() {
	QApplication::setOverrideCursor(Qt::WaitCursor);
	QApplication::processEvents();
	ui->b_startRun->setEnabled(false);
	ui->b_configure->setEnabled(false);
	m_monitoring = true;
	sendDAQCommand("start_mon");
	ui->b_startMonitoring->setEnabled(false);
    b_fileStart_clicked();
    b_tempStart_clicked();
    QApplication::restoreOverrideCursor();
}


//_____________________________________________________________________________

void RCMainWindow::b_fileStart_clicked() {
    if (m_fileTimer != NULL) delete m_fileTimer;
    if (p_file != NULL) delete p_file;
    setupFilePlot();
    m_plotsLay->addWidget(p_file, 0, 0, 1, 1);


    m_fileSampleNum = 0;
    // Timer.
    m_fileTimer = new QTimer(this);
    connect(m_fileTimer, SIGNAL(timeout()), this, SLOT(fileUpdate()));
    //m_fileTimer->start(m_filePlotFillRate);
}


//_____________________________________________________________________________

void RCMainWindow::b_fileEnd_clicked() {
    disconnect(m_fileTimer, SIGNAL(timeout()), this, SLOT(fileUpdate()));
    //m_fileTimer->stop();
}



//_____________________________________________________________________________

void RCMainWindow::on_b_quit_clicked() {
	updateZrecord(false);
    this->close();
}


//_____________________________________________________________________________

void RCMainWindow::on_b_DQM_clicked() {
	std::thread t_DQM(&RCMainWindow::launchQMs, this, "/localstore/TPX3/SPIDR/software/trunk/Telescope/DQM/GUI/PreQM ");
	t_DQM.detach();
}

//_____________________________________________________________________________

void RCMainWindow::launchQMs(std::string command) {
	std::stringstream ssRun;
	ssRun << ui->b_runNumber->value();
	command += ssRun.str();
	GUIOutput("[Note] external command: " + command);
	system(command.c_str());
}


//_____________________________________________________________________________

void RCMainWindow::on_b_FQM_clicked() {
	std::thread t_FQM(&RCMainWindow::launchQMs, this, "/localstore/TPX3/SPIDR/software/trunk/Telescope/DQM/FQM/FQM ");
	t_FQM.detach();
}


//_____________________________________________________________________________

std::vector<std::string> RCMainWindow::split_line(std::string line, int spacing){
	//function splits a string by its spaces.
	std::vector<std::string> line_bits;
	//cycle through the line, looking for spaces. Never starts with a space.
	//bool hitNextNumber = false;
	for (unsigned int i=0; i<line.size(); i++){
		if (line[i] == ' ') {
			if (line[i-1] != ' ') {
				line_bits.push_back(line.substr(0, i));
				line = line.substr(i, line.size()); //skip the space.
				i=0;
			}
		}
	}
	line_bits.push_back(line); //the last one.

	return line_bits;
}
