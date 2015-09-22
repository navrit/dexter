/**
 * John Idarraga <idarraga@cern.ch>
 * Nikhef, 2014.
 */

#include "DataTakingThread.h"
#include "qcstmglvisualization.h"
#include "ui_qcstmglvisualization.h"

#include "SpidrController.h"
#include "SpidrDaq.h"

#include "mpx3gui.h"
#include "ui_mpx3gui.h"

DataTakingThread::DataTakingThread(Mpx3GUI * mpx3gui, QCstmGLVisualization * dt) {

	_mpx3gui = mpx3gui;
	_vis = dt;
	_srcAddr = 0;
	_stop = false;
	_canDraw = true;

}

void DataTakingThread::ConnectToHardware() {

	SpidrController * spidrcontrol = _mpx3gui->GetSpidrController();

	// I need to do this here and not when already running the thread
	// Get the IP source address (SPIDR network interface) from the already connected SPIDR module.
	if ( spidrcontrol ) { spidrcontrol->getIpAddrSrc( 0, &_srcAddr ); }
	else { _srcAddr = 0; }

}

void DataTakingThread::run() {

	// Open a new temporary connection to the spider to avoid collisions to the main one
	int ipaddr[4] = { 1, 1, 168, 192 };
	if( _srcAddr != 0 ) {
		ipaddr[3] = (_srcAddr >> 24) & 0xFF;
		ipaddr[2] = (_srcAddr >> 16) & 0xFF;
		ipaddr[1] = (_srcAddr >>  8) & 0xFF;
		ipaddr[0] = (_srcAddr >>  0) & 0xFF;
	}

	SpidrController * spidrcontrol = new SpidrController( ipaddr[3], ipaddr[2], ipaddr[1], ipaddr[0] );

	if ( !spidrcontrol || !spidrcontrol->isConnected() ) {
		cout << "[ERR ] Device not connected !" << endl;
		return;
	}

	SpidrDaq * spidrdaq = _mpx3gui->GetSpidrDaq();

	connect(this, SIGNAL(reload_all_layers()), _vis, SLOT(on_reload_all_layers()));
	connect(this, SIGNAL(reload_layer(int)), _vis, SLOT(on_reload_layer(int)));
	connect(this, SIGNAL(data_taking_finished(int)), _vis, SLOT(on_data_taking_finished(int)));
	connect(this, SIGNAL(progress(int)), _vis, SLOT(on_progress_signal(int)));
	connect(_vis, SIGNAL(stop_data_taking_thread()), this, SLOT(on_stop_data_taking_thread())); // stop signal from qcstmglvis
	connect(_vis, SIGNAL(free_to_draw()), this, SLOT(on_free_to_draw()) );
	connect(_vis, SIGNAL(busy_drawing()), this, SLOT(on_busy_drawing()) );

	cout << "Acquiring ... ";
	//_mpx3gui->GetUI()->startButton->setActive(false);

	// Start the trigger as configured
	spidrcontrol->startAutoTrigger();

	int nFramesReceived = 0, lastDrawn = 0;
	int * framedata;
	emit progress( nFramesReceived );
	bool doReadFrames = true;
	unsigned int cntrBothCounters = 0;

	while ( spidrdaq->hasFrame( (_mpx3gui->getConfig()->getTriggerLength()/1000) + 20  ) ) { // 20ms over the trigger length timeout

		int size_in_bytes = -1;
		QVector<int> activeDevices = _mpx3gui->getConfig()->getActiveDevices();

		doReadFrames = true;
		if ( _vis->GetUI()->dropFramesCheckBox->isChecked() ) {
			if ( spidrdaq->packetsLostCountFrame() != 0 ) { // from any of the chips connected
				doReadFrames = false;
			}
		}

		if ( doReadFrames ) {
			for(int i = 0 ; i < activeDevices.size() ; i++) {

				framedata = spidrdaq->frameData(i, &size_in_bytes);

				//cout << "data[" << i << "] chip id : " << activeDevices[i] << endl;

				if ( size_in_bytes == 0 ) continue; // this may happen

				// In color mode the separation of thresholds needs to be done
				if( _mpx3gui->getConfig()->getColourMode() ) {

					if ( ( cntrBothCounters%2 == 0 && _mpx3gui->getConfig()->getReadBothCounters() ) ||  !_mpx3gui->getConfig()->getReadBothCounters() ) {
						//cout << "low : " << _mpx3gui->getConfig()->getReadBothCounters() << "," << cntrBothCounters << endl;
						int size = size_in_bytes / 4;
						int sizeReduced = size / 4;    // 4 thresholds per 110um pixel

						QVector<int> *th0 = new QVector<int>(sizeReduced,0);
						QVector<int> *th2 = new QVector<int>(sizeReduced,0);
						QVector<int> *th4 = new QVector<int>(sizeReduced,0);
						QVector<int> *th6 = new QVector<int>(sizeReduced,0);

						SeparateThresholds(framedata, size, th0, th2, th4, th6, sizeReduced);

						_mpx3gui->addFrame(th0->data(), i, 0);
						delete th0;

						_mpx3gui->addFrame(th2->data(), i, 2);
						delete th2;

						_mpx3gui->addFrame(th4->data(), i, 4);
						delete th4;

						_mpx3gui->addFrame(th6->data(), i, 6);
						delete th6;
					}

					if ( cntrBothCounters%2 == 1 && _mpx3gui->getConfig()->getReadBothCounters() ) {

						cout << "high" << endl;

						int size = size_in_bytes / 4;
						int sizeReduced = size / 4;    // 4 thresholds per 110um pixel

						QVector<int> *th1 = new QVector<int>(sizeReduced,0);
						QVector<int> *th3 = new QVector<int>(sizeReduced,0);
						QVector<int> *th5 = new QVector<int>(sizeReduced,0);
						QVector<int> *th7 = new QVector<int>(sizeReduced,0);

						SeparateThresholds(framedata, size, th1, th3, th5, th7, sizeReduced);

						_mpx3gui->addFrame(th1->data(), i, 1);
						delete th1;

						_mpx3gui->addFrame(th3->data(), i, 3);
						delete th3;

						_mpx3gui->addFrame(th5->data(), i, 5);
						delete th5;

						_mpx3gui->addFrame(th7->data(), i, 7);
						delete th7;
					}


				} else {
					_mpx3gui->addFrame(framedata, i, 0);
				}

			}
		}
		//_mpx3gui->getDataset()->addHistory();

		nFramesReceived++;
		// Release frame
		spidrdaq->releaseFrame();
		cntrBothCounters++;

		// Get to draw if possible
		if ( _canDraw ) {

			emit progress( nFramesReceived );

			if( _mpx3gui->getConfig()->getColourMode() ) {
				emit reload_all_layers();
			} else {
				emit reload_layer(0);
			}

			lastDrawn = nFramesReceived;
		}

		// If number of triggers reached
		if ( nFramesReceived == _mpx3gui->getConfig()->getNTriggers() ) break;

		// If called to Stop
		if ( _stop ) break;

	}

	// Force last draw if not reached
	if ( nFramesReceived != lastDrawn ) {
		emit progress( nFramesReceived );
		if( _mpx3gui->getConfig()->getColourMode() ) {
			emit reload_all_layers();
		} else {
			emit reload_layer(0);
		}
	}

	cout << "local counter : " << cntrBothCounters << endl;

	cout << "received " << nFramesReceived
			<< " | lost frames : " << spidrdaq->framesLostCount()
			<< " | lost packets : " << spidrdaq->packetsLostCount()
			<< endl;

	QVector<int> activeDevices = _mpx3gui->getConfig()->getActiveDevices();
	for(int i = 0 ; i < activeDevices.size() ; i++) {
		cout << "devId = " << i << " | packetsReceivedCount = " << spidrdaq->packetsReceivedCount( i ) << endl;
		cout << "devId = " << i << " | packetSize = " << spidrdaq->packetSize( i ) << endl;
	}
	// When the process is finished the thread sends a message
	//  to inform QCstmGLVisualization that it's done.
	// QCstmGLVisualization could be having a hard time trying
	//  to keep up with the drawing.  At that moment something
	//  needs to happens to avoid blocking.
	emit data_taking_finished( nFramesReceived );

	disconnect(this, SIGNAL(reload_all_layers()), _vis, SLOT(on_reload_all_layers()));
	disconnect(this, SIGNAL(reload_layer(int)), _vis, SLOT(on_reload_layer(int)));
	disconnect(this, SIGNAL(data_taking_finished(int)), _vis, SLOT(on_data_taking_finished(int)));
	disconnect(this, SIGNAL(progress(int)), _vis, SLOT(on_progress_signal(int)));
	disconnect(_vis, SIGNAL(stop_data_taking_thread()), this, SLOT(on_stop_data_taking_thread())); // stop signal from qcstmglvis
	disconnect(_vis, SIGNAL(free_to_draw()), this, SLOT(on_free_to_draw()) );
	disconnect(_vis, SIGNAL(busy_drawing()), this, SLOT(on_busy_drawing()) );

	// In case the thread is reused
	_stop = false;

	// clear counters in SpidrDac
	// spidrdaq->frame

	delete spidrcontrol;
}

pair<int, int> DataTakingThread::XtoXY(int X, int dimX){
	return make_pair(X % dimX, X/dimX);
}

void DataTakingThread::on_busy_drawing() {
	_canDraw = false;
}

void DataTakingThread::on_free_to_draw() {
	_canDraw = true;
}

void DataTakingThread::on_stop_data_taking_thread() {

	// Used to properly stop the data taking thread
	_stop = true;

}

void DataTakingThread::SeparateThresholds(int * data, int size, QVector<int> * th0, QVector<int> * th2, QVector<int> * th4, QVector<int> * th6, int sizeReduced) {

	// Layout of 110um pixel
	//  -------------   ---------------------
	//  | P3  |  P1 |   | thl 4,5 | thl 0,1 |
	//	-------------   ---------------------
	//  | P4  |  P2 |   | thl 6,7 | thl 2,3 |
	//  -------------   ---------------------
	//  Where:
	//  	P1 --> TH0, TH1
	//		P2 --> TH2, TH3
	//		P3 --> TH4, TH5
	//		P4 --> TH6, TH7


	int indx = 0, indxRed = 0, redi = 0, redj = 0;
	int c0 = 0, c2 = 0, c4 = 0, c6 = 0;

	for (int j = 0 ; j < __matrix_size_y ; j++) {
		//for (int j = __matrix_size_y-1 ; j >= 0 ; j--) {

		redi = 0;
		for (int i = 0 ; i < __matrix_size_x  ; i++) {
			//for (int i = __matrix_size_x-1 ; i >= 0  ; i--) {

			indx = XYtoX( i, j, __matrix_size_x);
			indxRed = XYtoX( redi, redj, __matrix_size_x / 2); // This index should go up to 128*128

			if( (i % 2) == 0 && (j % 2) == 0) {
				(*th2)[indxRed] = data[indx]; // P2
			}
			if( (i % 2) == 0 && (j % 2) == 1) {
				(*th0)[indxRed] = data[indx]; // P1
			}
			if( (i % 2) == 1 && (j % 2) == 0) {
				(*th6)[indxRed] = data[indx]; // P4
			}
			if( (i % 2) == 1 && (j % 2) == 1) {
				(*th4)[indxRed] = data[indx]; // P3
			}

			/*
			if( (i % 2) == 0 && (j % 2) == 0) {
				(*th6)[indxRed] = data[indx]; // P4
			}
			if( (i % 2) == 0 && (j % 2) == 1) {
				(*th4)[indxRed] = data[indx]; // P3
			}
			if( (i % 2) == 1 && (j % 2) == 0) {
				(*th2)[indxRed] = data[indx]; // P2
			}
			if( (i % 2) == 1 && (j % 2) == 1) {
				(*th0)[indxRed] = data[indx]; // P1
			}
			 */

			if (i % 2 == 1) redi++;
			//if (i % 2 == 0) redi++;

		}

		if (j % 2 == 1) redj++;

	}


}
