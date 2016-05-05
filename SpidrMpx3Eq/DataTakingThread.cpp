/**
 * John Idarraga <idarraga@cern.ch>
 * Nikhef, 2014.
 */

#include "DataTakingThread.h"
#include "qcstmglvisualization.h"
#include "ui_qcstmglvisualization.h"

#include "SpidrController.h"
#include "SpidrDaq.h"

#include "mpx3eq_common.h"
#include "mpx3gui.h"
#include "ui_mpx3gui.h"

DataTakingThread::DataTakingThread(Mpx3GUI * mpx3gui, QCstmGLVisualization * dt) {

    _mpx3gui = mpx3gui;
    _vis = dt;
    _srcAddr = 0;
    _stop = false;
    _canDraw = true;

    rewindScoring();

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

    //! Work around
    //! If we attempt a connection while the system is already sending data
    //! (this may happen if for instance the program died for whatever reason,
    //!  or when it is close while a very long data taking has been lauched and
    //! the system failed to stop the data taking).  If this happens we ought
    //! to stop data taking, and give the system a bit of delay.
    spidrcontrol->stopAutoTrigger();
    Sleep( 100 );

    // 0 : DEBUG
    // 1 : INFO
    // 2 : WARNINGS, ERROR, FATAL
    //spidrcontrol->setLogLevel( 2 );
    spidrcontrol->setLogLevel( 0 );

    /*
    spidrcontrol->setShutterTriggerConfig (
            4,
            40000,
            200000,
            100
    );
    */

    if ( !spidrcontrol || !spidrcontrol->isConnected() ) {
        qDebug() << "[ERR ] Device not connected !";
        return;
    }

    SpidrDaq * spidrdaq = _mpx3gui->GetSpidrDaq();


    connect(this, SIGNAL(reload_all_layers()), _vis, SLOT(reload_all_layers()));
    connect(this, SIGNAL(reload_layer(int)), _vis, SLOT(reload_layer(int)));
    connect(this, SIGNAL(data_taking_finished(int)), _vis, SLOT(data_taking_finished(int)));
    connect(this, SIGNAL(progress(int)), _vis, SLOT(progress_signal(int)));
    connect(_vis, SIGNAL(stop_data_taking_thread()), this, SLOT(on_stop_data_taking_thread())); // stop signal from qcstmglvis
    connect(_vis, SIGNAL(free_to_draw()), this, SLOT(on_free_to_draw()) );
    connect(_vis, SIGNAL(busy_drawing()), this, SLOT(on_busy_drawing()) );

    connect(this, SIGNAL(lost_packets(int)), _vis, SLOT(lost_packets(int)) );
    connect(this, SIGNAL(fps_update(int)), _vis, SLOT(fps_update(int)) );
    connect(this, SIGNAL(overflow_update(int)), _vis, SLOT(overflow_update(int)) );


    qDebug() << "[INFO] Acquiring ... ";
    //_mpx3gui->GetUI()->startButton->setActive(false);

    // Get the list of id's for active devices
    QVector<int> activeDevices = _mpx3gui->getConfig()->getActiveDevices();
    int nChips = activeDevices.size();
    int firstDevId = activeDevices[0];

    // Data structures for all 8 thresholds
    // counterL
    // I need to buffer the counterL in case the counterH is not correct.
    QVector<int> ** th0 = new QVector<int>*[nChips]; // have as many pointers as chips
    QVector<int> ** th2 = new QVector<int>*[nChips];
    QVector<int> ** th4 = new QVector<int>*[nChips];
    QVector<int> ** th6 = new QVector<int>*[nChips];
    for ( int i = 0 ; i < nChips ; i++ ) {
        th0[i] = 0x0;
        th2[i] = 0x0;
        th4[i] = 0x0;
        th6[i] = 0x0;
    }
    // counterH
    // I don't need to buffer the high counters
    QVector<int> * th1 = 0x0;
    QVector<int> * th3 = 0x0;
    QVector<int> * th5 = 0x0;
    QVector<int> * th7 = 0x0;

    int nFramesReceived = 0, lastDrawn = 0;
    int nFramesKept = 0;
    int frameId = 0, prevFrameId = 0;

    int * framedata;

    emit progress( nFramesReceived );
    bool doReadFrames_L = true;
    bool doReadFrames_H = true;
    bool badFlipping = false;

    int size_in_bytes = -1;

    // Timeout
    int timeOutTime =
            _mpx3gui->getConfig()->getTriggerLength_ms()
            +  _mpx3gui->getConfig()->getTriggerDowntime_ms()
            + 100; // ms
    // TODO ! The extra 500ms is a combination of delay in the network plus
    // system overhead.  This should be predicted and not hard-coded. TODO !

    // Start the trigger as configured
    spidrcontrol->startAutoTrigger();

    // keep an eye on overflow
    unsigned int overflowCntr = 0;

    while ( spidrdaq->hasFrame( timeOutTime ) ) {

        overflowCntr = 0;

        frameId = spidrdaq->frameShutterCounter( 0 );

        // If bad flip happened I wait until the first counterL shows up
        if ( badFlipping ) {

            if ( !spidrdaq->isCounterhFrame(firstDevId) && frameId != prevFrameId ) {

                // Out of the bad flip condition
                qDebug() << "[INFO] recovering from bad flip ... ";
                badFlipping = false;

            } else {

                // Keep a local count of number of frames
                nFramesReceived++;
                // keep the frameId
                prevFrameId = frameId;
                // Release frame
                spidrdaq->releaseFrame();
                continue;
            }

        }

        // If taking care of a counterL, start by rewinding the flags
        if ( !spidrdaq->isCounterhFrame( firstDevId ) ) {

            //qDebug() << "[INFO] Cleaning up";
            doReadFrames_L = true;
            doReadFrames_H = true;

            // Buffering for the counterL
            for ( int i = 0 ; i < nChips ; i++ ) {

                if ( th0[i] ) { delete th0[i]; th0[i] = 0x0; }
                if ( th2[i] ) { delete th2[i]; th2[i] = 0x0; }
                if ( th4[i] ) { delete th4[i]; th4[i] = 0x0; }
                if ( th6[i] ) { delete th6[i]; th6[i] = 0x0; }

            }
            // counterH is erased as soon as is used.
            // No need to buffer for that counterH
        }

        size_in_bytes = -1;

        int packetsLost = spidrdaq->lostCountFrame(); // The total number of lost packets/pixels detected in the current frame
        // report the loss
        emit lost_packets( packetsLost );


        if ( _vis->GetUI()->dropFramesCheckBox->isChecked() ) {

            if ( packetsLost != 0 ) { // from any of the chips connected

                if ( _mpx3gui->getConfig()->getColourMode() ) {
                    if ( spidrdaq->isCounterhFrame(firstDevId) ) doReadFrames_H = false;
                    else doReadFrames_L = false;
                } else {
                    doReadFrames_L = false;
                }

            }

        }

        // If in color mode, check flipping L,H -- L,H -- .... L,H
        if ( _mpx3gui->getConfig()->getColourMode() && _mpx3gui->getConfig()->getReadBothCounters() ) {

            // the flip is wrong
            if ( (bool)(nFramesReceived%2) != spidrdaq->isCounterhFrame(firstDevId) ) {
                // Keep a local count of number of frames
                nFramesReceived++;
                // keep the frameId
                prevFrameId = frameId;

                cout << "       !!! Bad flipping !!!" << endl;

                badFlipping = true;

                // Release frame
                spidrdaq->releaseFrame();

                continue;
            }

        }

        // If the frame is not good to read just keep going.
        // If both counters are to be read then drop both counterL and counterH.
        if ( !doReadFrames_L || !doReadFrames_H ) {

            // Keep a local count of number of frames
            nFramesReceived++;
            // keep the frameId
            prevFrameId = frameId;

            //cout << "Bad frame L:" << doReadFrames_L << ", H:" << doReadFrames_H << endl;

            // Release frame
            spidrdaq->releaseFrame();

            // and keep going
            continue;
        }


        for(int i = 0 ; i < activeDevices.size() ; i++) {

            // retreive data for a given chip
            framedata = spidrdaq->frameData(i, &size_in_bytes);

            //cout << "chip id : " << activeDevices[i] << " | SpidrDaq::frameShutterCounter: " << spidrdaq->frameShutterCounter(i) << endl;
            // if ( size_in_bytes == 0 ) continue; // this may happen

            // In color mode the separation of thresholds needs to be done
            if ( _mpx3gui->getConfig()->getColourMode() ) {

                if ( !spidrdaq->isCounterhFrame(firstDevId) ) {

                    //cout << "low , frameId = " << nFramesReceived << endl;

                    int size = size_in_bytes / __nThresholdsPerSpectroscopicPixel;
                    int sizeReduced = size / __nThresholdsPerSpectroscopicPixel;    // 4 thresholds per 110um pixel

                    th0[i] = new QVector<int>(sizeReduced, 0);
                    th2[i] = new QVector<int>(sizeReduced, 0);
                    th4[i] = new QVector<int>(sizeReduced, 0);
                    th6[i] = new QVector<int>(sizeReduced, 0);

                    SeparateThresholds(i, framedata, size, th0[i], th2[i], th4[i], th6[i], sizeReduced);

                    // Send if if we are not reading counterH
                    // counterL
                    if ( ! _mpx3gui->getConfig()->getReadBothCounters() ) {
                        overflowCntr += _mpx3gui->addFrame(th0[i]->data(), i, 0);
                        //delete th0;

                        overflowCntr += _mpx3gui->addFrame(th2[i]->data(), i, 2);
                        //delete th2;

                        overflowCntr += _mpx3gui->addFrame(th4[i]->data(), i, 4);
                        //delete th4;

                        overflowCntr += _mpx3gui->addFrame(th6[i]->data(), i, 6);
                        //delete th6;
                    }

                }

                if ( spidrdaq->isCounterhFrame(firstDevId) && _mpx3gui->getConfig()->getReadBothCounters() ) {

                    //cout << "high , frameId = " << nFramesReceived << endl;


                    int size = size_in_bytes / __nThresholdsPerSpectroscopicPixel;
                    int sizeReduced = size / __nThresholdsPerSpectroscopicPixel;    // 4 thresholds per 110um pixel

                    th1 = new QVector<int>(sizeReduced, 0);
                    th3 = new QVector<int>(sizeReduced, 0);
                    th5 = new QVector<int>(sizeReduced, 0);
                    th7 = new QVector<int>(sizeReduced, 0);

                    SeparateThresholds(i, framedata, size, th1, th3, th5, th7, sizeReduced);

                    // Now send all the thresholds for the give chip.
                    // !!! WARNING: They have to enter in order !!!
                    // TH0
                    overflowCntr += _mpx3gui->addFrame(th0[i]->data(), i, 0);
                    //delete th0;

                    // TH1
                    overflowCntr += _mpx3gui->addFrame(th1->data(), i, 1);
                    //delete th1; th1 = 0x0;

                    // TH2
                    overflowCntr += _mpx3gui->addFrame(th2[i]->data(), i, 2);
                    //delete th2;

                    // TH3
                    overflowCntr += _mpx3gui->addFrame(th3->data(), i, 3);
                    //delete th3; th3 = 0x0;

                    // TH4
                    overflowCntr += _mpx3gui->addFrame(th4[i]->data(), i, 4);
                    //delete th4;

                    // TH5
                    overflowCntr += _mpx3gui->addFrame(th5->data(), i, 5);
                    //delete th5; th5 = 0x0;

                    // TH6
                    overflowCntr += _mpx3gui->addFrame(th6[i]->data(), i, 6);
                    //delete th6;

                    // TH7
                    overflowCntr += _mpx3gui->addFrame(th7->data(), i, 7);
                    //delete th7; th7 = 0x0;

                    // Get ready with the high thresholds
                    if ( th1 ) { delete th1; th1 = 0x0; }
                    if ( th3 ) { delete th3; th3 = 0x0; }
                    if ( th5 ) { delete th5; th5 = 0x0; }
                    if ( th7 ) { delete th7; th7 = 0x0; }

                }


            } else {
                overflowCntr += _mpx3gui->addFrame(framedata, i, 0);
            }

        }

        // Keep a local count of number of frames
        nFramesReceived++;
        // And these are the frames actually kept
        nFramesKept++;
        // keep the frameId
        prevFrameId = frameId;
        // Release frame
        spidrdaq->releaseFrame();


        // Report to the gui
        if ( _mpx3gui->getConfig()->getReadBothCounters() ) emit fps_update( nFramesReceived/2 );
        else emit fps_update( nFramesReceived );

        emit overflow_update( overflowCntr );

        // Get to draw if possible
        if ( _canDraw ) {

            // When reading both counters 1 full frame is made of 2 frames received
            if ( _mpx3gui->getConfig()->getReadBothCounters() ) emit progress( nFramesKept/2 );
            else  emit progress( nFramesKept );

            if( _mpx3gui->getConfig()->getColourMode() ) {
                emit reload_all_layers();
            } else {
                emit reload_layer(0);
            }

            lastDrawn = nFramesReceived;
        }

        // If called to Stop
        if ( _stop ) break;

        // Fake a stop if the number of frames to be taken has been reached
        if ( _score.missingToCompleteJob == nFramesKept ) {
            // If this happened
            break;
        }
    }

    if ( _stop ) { // if the data taking was stopped
        spidrcontrol->stopAutoTrigger();
    }

    // If number of triggers reached
    //if ( nFramesReceived == _mpx3gui->getConfig()->getNTriggers() ) break;

    // Force last draw if not reached
    if ( nFramesReceived != lastDrawn ) {

        if ( _mpx3gui->getConfig()->getReadBothCounters() ) emit progress( nFramesKept/2 );
        else  emit progress( nFramesKept );

        /*
        if( _mpx3gui->getConfig()->getColourMode() ) {
            emit reload_all_layers();
        } else {
            emit reload_layer(0);
        }
        */

    }

    // Keep some scoring info for later
    _score.framesKept = nFramesKept;

    qDebug() << "[INFO] received : " << nFramesReceived
             << " | frames kept : " << nFramesKept
             << " | lost frames : " << spidrdaq->framesLostCount()
             << " | lost packets(ML605)/pixels(compactSPIDR) : " << spidrdaq->lostCount()
             << " | frames count : " << spidrdaq->framesCount();

    //for ( int i = 0 ; i < activeDevices.size() ; i++ ) {
    //    cout << "devId = " << i << " | packetsReceivedCount = " << spidrdaq->packetsReceivedCount( i ) << endl;
    //    cout << "devId = " << i << " | packetSize = " << spidrdaq->packetSize( i ) << endl;
    //}

    // When the process is finished the thread sends a message
    //  to inform QCstmGLVisualization that it's done.
    // QCstmGLVisualization could be having a hard time trying
    //  to keep up with the drawing.  At that moment something
    //  needs to happens to avoid blocking.
    emit data_taking_finished( 0 );

    disconnect( this, SIGNAL(reload_all_layers()), _vis, SLOT(reload_all_layers()));
    disconnect( this, SIGNAL(reload_layer(int)), _vis, SLOT(reload_layer(int)));
    disconnect( this, SIGNAL(data_taking_finished(int)), _vis, SLOT(data_taking_finished(int)));
    disconnect( this, SIGNAL(progress(int)), _vis, SLOT(progress_signal(int)));
    disconnect( _vis, SIGNAL(stop_data_taking_thread()), this, SLOT(on_stop_data_taking_thread())); // stop signal from qcstmglvis
    disconnect( _vis, SIGNAL(free_to_draw()), this, SLOT(on_free_to_draw()) );
    disconnect( _vis, SIGNAL(busy_drawing()), this, SLOT(on_busy_drawing()) );

    disconnect( this, SIGNAL(lost_packets(int)), _vis, SLOT(lost_packets(int)) );
    disconnect( this, SIGNAL(fps_update(int)), _vis, SLOT(fps_update(int)) );
    disconnect( this, SIGNAL(overflow_update(int)), _vis, SLOT(overflow_update(int)) );

    // In case the thread is reused
    _stop = false;

    // clear counters in SpidrDac
    spidrdaq->resetLostCount();

    delete spidrcontrol;

}

pair<int, int> DataTakingThread::XtoXY(int X, int dimX){
    return make_pair(X % dimX, X/dimX);
}

bool DataTakingThread::ThereIsAFalse(vector<bool> v) {

    vector<bool>::iterator i  = v.begin();
    vector<bool>::iterator iE = v.end();

    for ( ; i != iE ; i++ ) {
        if ( (*i) == false ) return true;
    }
    return false;
}

void DataTakingThread::rewindScoring() {
    _score.framesKept = 0;
    _score.framesReceived = 0;
    _score.framesRequested = 0;
    _score.missingToCompleteJob = 0;
    _score.tocomplete = false;
}

int DataTakingThread::calcScoreDifference() {

    // counting down
    _score.missingToCompleteJob -= _score.framesKept;

    // counting up
    _score.framesReceived += _score.framesKept;

    if ( _score.missingToCompleteJob != 0 ) _score.tocomplete = true;
    else _score.tocomplete = false;

    return _score.missingToCompleteJob;
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

void DataTakingThread::SeparateThresholds(int /*id*/, int * data, int /*size*/, QVector<int> * th0, QVector<int> * th2, QVector<int> * th4, QVector<int> * th6, int /*sizeReduced*/) {

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

    // test
    //QString name = "frame_";
    //name += QString::number( id, 'd', 0 );
    //name += ".txt";
    //std::fstream fs ( name.toStdString().c_str(), std::fstream::out);
    //int cntr = 0;

    int indx = 0, indxRed = 0, redi = 0, redj = 0;
    //int c0 = 0, c2 = 0, c4 = 0, c6 = 0;

    for (int j = 0 ; j < __matrix_size_y ; j++) {
        //for (int j = __matrix_size_y-1 ; j >= 0 ; j--) {

        redi = 0;
        for (int i = 0 ; i < __matrix_size_x  ; i++) {
            //for (int i = __matrix_size_x-1 ; i >= 0  ; i--) {

            indx = XYtoX( i, j, __matrix_size_x);
            indxRed = XYtoX( redi, redj, __matrix_size_x / 2); // This index should go up to 128*128

            // test
            //fs << data[indx] << " ";
            //cntr++;
            //if( cntr%256 == 0 ) fs << endl;

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

    //fs.close();
}
