/**
 * John Idarraga <idarraga@cern.ch>
 * Nikhef, 2014.
 */

#include "DataTakingThread.h"
#include "qcstmglvisualization.h"
#include "ui_qcstmglvisualization.h"

#include "SpidrController.h"
#include "SpidrDaq.h"
#include "dataconsumerthread.h"

#include "mpx3eq_common.h"
#include "mpx3gui.h"
#include "ui_mpx3gui.h"

DataTakingThread::DataTakingThread(Mpx3GUI * mpx3gui, DataConsumerThread * consumer, QObject * parent)
    : QThread( parent )
{

    _restart = false;
    _abort = false;
    _idling = false;
    _stop = false;

    _vis = static_cast<QCstmGLVisualization*>( parent );
    _mpx3gui = mpx3gui;

    _consumer = consumer;

    _srcAddr = 0;
    _stop = false;
    _canDraw = true;
    rewindScoring();

}

DataTakingThread::~DataTakingThread() {

    _mutex.lock();
    _stop = false;
    _abort = true;          // will stop run as soon as possible
    _condition.wakeOne();   // wake up if sleeping
    _mutex.unlock();

    wait(); // wait 'til run has exited before the base class destructor is invoked

    qDebug() << "   DataTakingThread finished";

}

void DataTakingThread::takedata() {

    QMutexLocker locker( &_mutex );

    if ( ! isRunning() ) {
        _stop = false;
        start( HighestPriority );
    } else {
        _stop = false;
        _restart = true;
        _condition.wakeOne();
    }

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

    // New instance.  It belongs to this thread
    SpidrController * spidrcontrol = new SpidrController( ipaddr[3], ipaddr[2], ipaddr[1], ipaddr[0] );
    if ( !spidrcontrol || !spidrcontrol->isConnected() ) {
        qDebug() << "[ERR ] Device not connected !";
        return;
    }

    spidrcontrol->resetCounters();
    spidrcontrol->setLogLevel( 0 );
    //! Work around
    //! If we attempt a connection while the system is already sending data
    //! (this may happen if for instance the program was killed by the user,
    //!  or when it is close while a very long data taking has been lauched and
    //! the system failed to stop the data taking).  If this happens we ought
    //! to stop data taking, and give the system a bit of delay.
    //spidrcontrol->stopAutoTrigger();
    //spidrcontrol->stopContReadout();
    //Sleep( 100 );

    SpidrDaq * spidrdaq = _mpx3gui->GetSpidrDaq();

    connect(this, SIGNAL(scoring_sig(int,int,int,int,int,int,bool)),
            _vis,   SLOT( on_scoring(int,int,int,int,int,int,bool)) );

    connect(this, SIGNAL(data_taking_finished(int)), _vis, SLOT(data_taking_finished(int)));
    connect(_vis, SIGNAL(stop_data_taking_thread()), this, SLOT(on_stop_data_taking_thread())); // stop signal from qcstmglvis

    int timeOutTime = 0;
    int contRWFreq = 0;
    QVector<int> activeDevices = _mpx3gui->getConfig()->getActiveDevices();
    int nChips = activeDevices.size();
    int * framedata = nullptr;
    int opMode = Mpx3Config::__operationMode_SequentialRW;

    int size_in_bytes = -1;
    int nFramesReceived = 0, nFramesKept = 0;
    bool dropFrames = false;

    bool goalAchieved = false; //The compiler thinks this is unused, it is used...

    ///////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////

    forever {

        // When abort execution. Triggered as the destructor is called.
        if( _abort ) return;

        // Fetch new parameters
        // After a start or restart
        _mutex.lock();
        datataking_score_info score = _score;
        timeOutTime = _mpx3gui->getConfig()->getTriggerLength_ms()
                +  _mpx3gui->getConfig()->getTriggerDowntime_ms()
                + 100; // ms
        opMode = _mpx3gui->getConfig()->getOperationMode();
        contRWFreq = _mpx3gui->getConfig()->getContRWFreq();
        dropFrames = _vis->getDropFrames();
        //qDebug() << score.framesRequested << " | " << opMode << " | " << contRWFreq;
        _mutex.unlock();

        // Reset
        goalAchieved = false;
        spidrcontrol->resetCounters();
        spidrdaq->resetLostCount();

        if ( opMode == Mpx3Config::__operationMode_ContinuousRW ) {
            spidrcontrol->startContReadout( contRWFreq );
        } else {
            spidrcontrol->startAutoTrigger();
        }

        while ( spidrdaq->hasFrame( timeOutTime ) ) {

            // I need to keep extracting data even if the goal has been achieved
            for ( int i = 0 ; i < nChips ; i++ ) {

                // retreive data for a given chip
                framedata = spidrdaq->frameData(i, &size_in_bytes);

                // Copy data in the consumer using the Semaphores
                _consumer->freeFrames->acquire(); // Acquire space for 1 chip in one frame
                _consumer->copydata( framedata, size_in_bytes );
                _consumer->usedFrames->release();

            }

            // Awake the consumer thread if neccesary
            _consumer->consume();

            // Queue the data and keep going as fast as possible
            //if ( ! goalAchieved ) { }

            // Release frame
            spidrdaq->releaseFrame();

            // Keep a local count of number of frames
            nFramesReceived++;
            if ( ! ( spidrdaq->lostCount() > 0 && dropFrames ) ) nFramesKept++;

            // Reports
            emit scoring_sig(nFramesReceived,
                             nFramesKept,
                             spidrdaq->framesLostCount() / nChips,  //
                             spidrdaq->lostCount(),                 // lost packets(ML605)/pixels(compactSPIDR)
                             spidrdaq->framesCount(),               // ?
                             0,
                             spidrdaq->framesLostCount() % nChips   // Data misaligned
                             );

            /////////////////////////////////////////////
            // How to stop
            // 1) See Note 1 at the bottom
            if ( nFramesReceived == score.framesRequested ) {
                goalAchieved = true;
                if ( opMode == Mpx3Config::__operationMode_ContinuousRW ) {
                    spidrcontrol->stopContReadout();
                }
            }

            // 2) User stop condition
            if ( _stop ) break;

            // 3) User restart condition
            if ( _restart ) break;

        }

        // If the data taking was stopped
        if ( opMode == Mpx3Config::__operationMode_ContinuousRW ) {
            spidrcontrol->stopContReadout();
        } else {
            spidrcontrol->stopAutoTrigger();
        }
        // Clear
        //for ( int i = 0 ; i < nChips ; i++ ) {
        //    spidrdaq->clearFrameData( i );
        //}

        if ( ! _restart ) emit data_taking_finished( 0 );

        //qDebug() << "   lock DataTakingThread | goal:" << goalAchieved << " | frames:" << nFramesReceived ;

        // Rewind local variables
        nFramesReceived = 0;
        nFramesKept = 0;
        goalAchieved = false;

        // Put the thread to wait
        _mutex.lock();
        if ( !_restart ) {
            _idling = true;
            _condition.wait( &_mutex );
        }
        _idling = false;
        _restart = false;
        _mutex.unlock();
        //qDebug() << "   +++ unlock DataTakingThread";

    } // forever


    ///////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////
    disconnect(this, SIGNAL(scoring_sig(int,int,int,int,int,int,bool)),
               _vis,   SLOT( on_scoring(int,int,int,int,int,int,bool)) );

    disconnect(this, SIGNAL(data_taking_finished(int)), _vis, SLOT(data_taking_finished(int)));
    disconnect(_vis, SIGNAL(stop_data_taking_thread()), this, SLOT(on_stop_data_taking_thread())); // stop signal from qcstmglvis

    // In case the thread is reused
    _stop = false;

    // clear counters in SpidrDac
    spidrdaq->resetLostCount();
    delete spidrcontrol;

    // Note1.  When for instance taking data in ContinuousRW mode, in order to stop the
    //  operation one can only invoke stopAutoTrigger.  In the meanwhile a few extra frames
    //  could have been taken, that we don't need.  But it seems to be important to let
    //  spidrdaq finish evacuating frames.

}

void DataTakingThread::run2() {

    // Work an protect local variables
    _mutex.lock();
    //datataking_score_info score = this->_score;
    _mutex.unlock();

    // Open a new temporary connection to the spider to avoid collisions to the main one
    int ipaddr[4] = { 1, 1, 168, 192 };
    if( _srcAddr != 0 ) {
        ipaddr[3] = (_srcAddr >> 24) & 0xFF;
        ipaddr[2] = (_srcAddr >> 16) & 0xFF;
        ipaddr[1] = (_srcAddr >>  8) & 0xFF;
        ipaddr[0] = (_srcAddr >>  0) & 0xFF;
    }

    SpidrController * spidrcontrol = new SpidrController( ipaddr[3], ipaddr[2], ipaddr[1], ipaddr[0] );

    // +i
    spidrcontrol->resetCounters();

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

    if ( !spidrcontrol || !spidrcontrol->isConnected() ) {
        qDebug() << "[ERR ] Device not connected !";
        return;
    }

    SpidrDaq * spidrdaq = _mpx3gui->GetSpidrDaq();

    //connect(this, SIGNAL(reload_all_layers()), _vis, SLOT(reload_all_layers()));
    //connect(this, SIGNAL(reload_layer(int)), _vis, SLOT(reload_layer(int)));
    //connect(this, SIGNAL(data_taking_finished(int)), _vis, SLOT(data_taking_finished(int)));
    //connect(_vis, SIGNAL(stop_data_taking_thread()), this, SLOT(on_stop_data_taking_thread())); // stop signal from qcstmglvis
    //connect(_vis, SIGNAL(free_to_draw()), this, SLOT(on_free_to_draw()) );
    //connect(_vis, SIGNAL(busy_drawing()), this, SLOT(on_busy_drawing()) );

    //connect(this, SIGNAL(progress(int)), _vis, SLOT(progress_signal(int)));
    //connect(this, SIGNAL(lost_packets(int)), _vis, SLOT(lost_packets(int)) );
    //connect(this, SIGNAL(lost_frames(int)), _vis, SLOT(lost_frames(int)) );
    //connect(this, SIGNAL(data_misaligned(bool)), _vis, SLOT(data_misaligned(bool)) );
    //connect(this, SIGNAL(mpx3clock_stops(int)), _vis, SLOT(mpx3clock_stops(int)) );
    //connect(this, SIGNAL(fps_update(int)), _vis, SLOT(fps_update(int)) );
    //connect(this, SIGNAL(overflow_update(int)), _vis, SLOT(overflow_update(int)) );

    connect(this, &DataTakingThread::dataReady, _mpx3gui, &Mpx3GUI::dataReady);

    qDebug() << "[INFO] Acquiring ... ";

    // Get the list of id's for active devices
    QVector<int> activeDevices = _mpx3gui->getConfig()->getActiveDevices();
    int nChips = activeDevices.size();
    //KoreaHack int nChips = 4;
    int firstDevId = activeDevices[0];

    // Data structures for all 8 thresholds
    // counterL
    // I need to buffer the counterL in case the counterH is not correct.
    // Protect data and work on the copy

    _mutex.lock();
    QVector<int> ** th0 = nullptr;
    QVector<int> ** th2 = nullptr;
    QVector<int> ** th4 = nullptr;
    QVector<int> ** th6 = nullptr;
    th0 = new QVector<int>*[nChips];
    th2 = new QVector<int>*[nChips];
    th4 = new QVector<int>*[nChips];
    th6 = new QVector<int>*[nChips];
    for ( int i = 0 ; i < nChips ; i++ ) {
        th0[i] = nullptr;
        th2[i] = nullptr;
        th4[i] = nullptr;
        th6[i] = nullptr;
    }
    _th0 = th0;
    _th2 = th2;
    _th4 = th4;
    _th6 = th6;
    _mutex.unlock();


    // counterH
    // I don't need to buffer the high counters
    QVector<int> * th1 = nullptr;
    QVector<int> * th3 = nullptr;
    QVector<int> * th5 = nullptr;
    QVector<int> * th7 = nullptr;

    int nFramesReceived = 0, lastDrawn = 0;
    int nFramesKept = 0;
    int frameId = 0, prevFrameId = 0;

    int * framedata;

    //emit progress( nFramesReceived );
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

    if ( _mpx3gui->getConfig()->getOperationMode()
         == Mpx3Config::__operationMode_ContinuousRW ) {
        spidrcontrol->startContReadout( 10 );
    } else {
        spidrcontrol->startAutoTrigger();
    }

    // keep an eye on overflow
    unsigned int overflowCntr = 0;

    while ( spidrdaq->hasFrame( timeOutTime ) ) {

        overflowCntr = 0;

        frameId = spidrdaq->frameShutterCounter( 0 );

        // If bad flip happened I wait until the first counterL shows up
        /*
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
        */

        // If taking care of a counterL, start by rewinding the flags
        /*
        if ( !spidrdaq->isCounterhFrame( firstDevId ) ) {

            //qDebug() << "[INFO] Cleaning up";
            doReadFrames_L = true;
            doReadFrames_H = true;


            //_mutex.lock();
            //// Buffering for the counterL
            //for ( int i = 0 ; i < nChips ; i++ ) {
            //    if ( th0[i] ) { delete th0[i]; th0[i] = nullptr; }
            //    if ( th2[i] ) { delete th2[i]; th2[i] = nullptr; }
            //    if ( th4[i] ) { delete th4[i]; th4[i] = nullptr; }
            //    if ( th6[i] ) { delete th6[i]; th6[i] = nullptr; }
            //}
            //_mutex.unlock();


            // counterH is erased as soon as is used.
            // No need to buffer for that counterH
        }
        */

        size_in_bytes = -1;

        //int packetsLost = spidrdaq->lostCountFrame(); // The total number of lost packets/pixels detected in the current frame
        //int framesLost = spidrdaq->framesLostCount();
        // report the loss
        //emit lost_packets( spidrdaq->lostCount() );


        /*
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
*/

        // If in color mode, check flipping L,H -- L,H -- .... L,H
        /*
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
*/

        // If the frame is not good to read just keep going.
        // If both counters are to be read then drop both counterL and counterH.
        /*
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
        */

        int sizeReduced = 0; // data will be 4*sizeReduced
        QVector<int> dataTH0;
        QVector<int> dataTH2;
        QVector<int> dataTH4;
        QVector<int> dataTH6;

        for(int i = 0 ; i < activeDevices.size() ; i++) {

            // retreive data for a given chip
            framedata = spidrdaq->frameData(i, &size_in_bytes);
            int size = size_in_bytes/4;

            // In color mode the separation of thresholds needs to be done
            if ( _mpx3gui->getConfig()->getColourMode() ) {

                if ( !spidrdaq->isCounterhFrame(firstDevId) ) {

                    //cout << "low , frameId = " << nFramesReceived << endl;

                    size = size_in_bytes / __nThresholdsPerSpectroscopicPixel;
                    sizeReduced = size / __nThresholdsPerSpectroscopicPixel;    // 4 thresholds per 110um pixel

                    if ( ! th0[i] ) th0[i] = new QVector<int>(sizeReduced, 0);
                    if ( ! th2[i] ) th2[i] = new QVector<int>(sizeReduced, 0);
                    if ( ! th4[i] ) th4[i] = new QVector<int>(sizeReduced, 0);
                    if ( ! th6[i] ) th6[i] = new QVector<int>(sizeReduced, 0);

                    SeparateThresholds(i, framedata, size, th0[i], th2[i], th4[i], th6[i], sizeReduced);

                    if ( ! _mpx3gui->getConfig()->getReadBothCounters() ) {

                        for ( int j = 0 ; j < sizeReduced ; j++) {
                            dataTH0.append( th0[i]->at(j) ); // Append chip per chip index:i. Pixel index:j.
                            dataTH2.append( th2[i]->at(j) );
                            dataTH4.append( th4[i]->at(j) );
                            dataTH6.append( th6[i]->at(j) );
                        }
                    }

                }

                if ( spidrdaq->isCounterhFrame(firstDevId) && _mpx3gui->getConfig()->getReadBothCounters() ) {


                    int size = size_in_bytes / __nThresholdsPerSpectroscopicPixel;
                    int sizeReduced = size / __nThresholdsPerSpectroscopicPixel;    // 4 thresholds per 110um pixel

                    th1 = new QVector<int>(sizeReduced, 0);
                    th3 = new QVector<int>(sizeReduced, 0);
                    th5 = new QVector<int>(sizeReduced, 0);
                    th7 = new QVector<int>(sizeReduced, 0);

                    SeparateThresholds(i, framedata, size, th1, th3, th5, th7, sizeReduced);

                    /*
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
*/
                    // Get ready with the high thresholds
                    if ( th1 ) { delete th1; th1 = nullptr; }
                    if ( th3 ) { delete th3; th3 = nullptr; }
                    if ( th5 ) { delete th5; th5 = nullptr; }
                    if ( th7 ) { delete th7; th7 = nullptr; }

                }

            } else {
                for ( int j = 0 ; j < size ; j++) {
                    dataTH0.append( framedata[j] );
                }
            }

        }

        _mutex.lock();
        _incomingDataTH0.enqueue( dataTH0 ); emit dataReady( 0 );
        if ( _mpx3gui->getConfig()->getColourMode() ) {
            _incomingDataTH2.enqueue( dataTH2 ); emit dataReady( 2 );
            _incomingDataTH4.enqueue( dataTH4 ); emit dataReady( 4 );
            _incomingDataTH6.enqueue( dataTH6 ); emit dataReady( 6 );
        }
        _mutex.unlock();

        //qDebug() << _incomingDataTH0.size();

        // Keep a local count of number of frames
        nFramesReceived++;
        // And these are the frames actually kept
        nFramesKept++;
        // keep the frameId
        prevFrameId = frameId;
        // Release frame
        spidrdaq->releaseFrame();


        // Report to the gui
        //if ( _mpx3gui->getConfig()->getReadBothCounters() ) emit fps_update( nFramesReceived/2 );
        //else emit fps_update( nFramesReceived );

        //emit overflow_update( overflowCntr );

        // Get to draw if possible
        if ( _canDraw ) {

            // When reading both counters 1 full frame is made of 2 frames received
            //if ( _mpx3gui->getConfig()->getReadBothCounters() ) emit progress( nFramesKept/2 );
            //else  emit progress( nFramesKept );

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


    if ( _mpx3gui->getConfig()->getOperationMode()
         == Mpx3Config::__operationMode_ContinuousRW ) {
        spidrcontrol->stopContReadout();
    }

    if ( _stop ) { // if the data taking was stopped
        spidrcontrol->stopAutoTrigger();
    }

    // If number of triggers reached
    //if ( nFramesReceived == _mpx3gui->getConfig()->getNTriggers() ) break;

    // Force last draw if not reached
    if ( nFramesReceived != lastDrawn ) {

        //if ( _mpx3gui->getConfig()->getReadBothCounters() ) emit progress( nFramesKept/2 );
        //else  emit progress( nFramesKept );

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
             << " | lost frames : " << spidrdaq->framesLostCount() / nChips
             << " | lost packets(ML605)/pixels(compactSPIDR) : " << spidrdaq->lostCount()
             << " | frames count : " << spidrdaq->framesCount();

    // If framesLostCount() is not divisible by the number of chips, then the data is missaligned
    if ( spidrdaq->framesLostCount() % nChips != 0 ) {
        //emit data_misaligned( true );
        qDebug() << "[FATAL] Data is missaligned !!! | frames lots in all chips () = "
                 << spidrdaq->framesLostCount() << " |  can't divide by " << nChips ;
    }
    //emit lost_frames( spidrdaq->framesLostCount() / nChips );
    int val;

    // In case the MPX3 clock was stopped by the SPIDR (when running too fast). Address 0x10b8 look SPIDR register map.
    spidrcontrol->getSpidrReg( 0x10b8, &val );
    //emit mpx3clock_stops( val );


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

    //disconnect( this, SIGNAL(reload_all_layers()), _vis, SLOT(reload_all_layers()));
    //disconnect( this, SIGNAL(reload_layer(int)), _vis, SLOT(reload_layer(int)));
    //disconnect( this, SIGNAL(data_taking_finished(int)), _vis, SLOT(data_taking_finished(int)));
    //disconnect( this, SIGNAL(progress(int)), _vis, SLOT(progress_signal(int)));
    //disconnect( _vis, SIGNAL(stop_data_taking_thread()), this, SLOT(on_stop_data_taking_thread())); // stop signal from qcstmglvis
    //disconnect( _vis, SIGNAL(free_to_draw()), this, SLOT(on_free_to_draw()) );
    //disconnect( _vis, SIGNAL(busy_drawing()), this, SLOT(on_busy_drawing()) );

    //disconnect( this, SIGNAL(lost_packets(int)), _vis, SLOT(lost_packets(int)) );
    //disconnect( this, SIGNAL(fps_update(int)), _vis, SLOT(fps_update(int)) );
    //disconnect( this, SIGNAL(overflow_update(int)), _vis, SLOT(overflow_update(int)) );

    disconnect(this, &DataTakingThread::dataReady, _mpx3gui, &Mpx3GUI::dataReady);

    // In case I need the thread to go to sleep.  Not the case here.
    //_mutex.lock();
    //_condition.wait(&_mutex);
    //_mutex.unlock();

    qDebug() << "Thread finishing";

    // In case the thread is reused
    _stop = false;

    // clear counters in SpidrDac
    spidrdaq->resetLostCount();

    // Clear memory

    for ( int i = 0 ; i < nChips ; i++ ) {
        if ( th0[i] ) { delete th0[i]; th0[i] = nullptr; }
        if ( th2[i] ) { delete th2[i]; th2[i] = nullptr; }
        if ( th4[i] ) { delete th4[i]; th4[i] = nullptr; }
        if ( th6[i] ) { delete th6[i]; th6[i] = nullptr; }
    }
    if ( th0 ) delete [] th0;
    if ( th2 ) delete [] th2;
    if ( th4 ) delete [] th4;
    if ( th6 ) delete [] th6;

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

QVector<int> DataTakingThread::getData(int layer)
{
    if ( layer == 0 ) {
        return _incomingDataTH0.dequeue();
        //return _th0[index]->data();
    } else if ( layer == 2 ) {
        return _incomingDataTH2.dequeue();
        //return _th2[index]->data();
    } else if ( layer == 4 ) {
        return _incomingDataTH4.dequeue();
        //return _th4[index]->data();
    } else if ( layer == 6 ) {
        return _incomingDataTH6.dequeue();
        //return _th6[index]->data();
    }

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


//void DataTakingThread::SeparateThresholds( int /*id*/,
//                                           int * data,
//                                           int /*size*/,
//                                           QVector<int> * th0,
//                                           QVector<int> * th2,
//                                           QVector<int> * th4,
//                                           QVector<int> * th6,
//                                           int /*sizeReduced*/)
//{
//  int indxRed = 0;
//  for (int j = 0 ; j < __matrix_size_y ; j++) {
//    if( j & 1 ) {
//      indxRed -= __matrix_size_x/2;
//      for (int i = 0 ; i < __matrix_size_x  ; i++) {
//        if( i & 1 ) {
//          (*th4)[indxRed] = *data; // P3
//          ++indxRed;
//        } else {
//          (*th0)[indxRed] = *data; // P1
//        }
//        ++data;
//      }
//    } else {
//      for (int i = 0 ; i < __matrix_size_x  ; i++) {
//        if( i & 1 ) {
//          (*th6)[indxRed] = *data; // P4
//          ++indxRed;
//        } else {
//          (*th2)[indxRed] = *data; // P2
//        }
//        ++data;
//      }
//    }
//  }
//}

void DataTakingThread::SeparateThresholds(int /*id*/,
                                          int * data,
                                          int /*size*/,
                                          QVector<int> * th0,
                                          QVector<int> * th2,
                                          QVector<int> * th4,
                                          QVector<int> * th6,
                                          int /*sizeReduced*/) {

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
                //if ( indxRed %4 == 0 ) (*th0)[indxRed] = 1;
                //else (*th0)[indxRed] = 0;
            }
            if( (i % 2) == 1 && (j % 2) == 0) {
                (*th6)[indxRed] = data[indx]; // P4
            }
            if( (i % 2) == 1 && (j % 2) == 1) {
                (*th4)[indxRed] = data[indx]; // P3
            }


            //if( (i % 2) == 0 && (j % 2) == 0) {
            //    (*th6)[indxRed] = data[indx]; // P4
            //}
            //if( (i % 2) == 0 && (j % 2) == 1) {
            //    (*th4)[indxRed] = data[indx]; // P3
            //}
            //if( (i % 2) == 1 && (j % 2) == 0) {
            //    (*th2)[indxRed] = data[indx]; // P2
            //}
            //if( (i % 2) == 1 && (j % 2) == 1) {
            //    (*th0)[indxRed] = data[indx]; // P1
            //}


            if (i % 2 == 1) redi++;
            //if (i % 2 == 0) redi++;

        }

        if (j % 2 == 1) redj++;

    }

    //fs.close();
}


/*
for( int j = 0 ; j < size_in_bytes/4 ; j++ )
{
    pair<int,int> pix = XtoXY(j, 256);
    if( i == 0 ) {
        if ( pix.first == pix.second ) {

            framedata[j] = 1;
        } else {
            framedata[j] = 0;
        }
    } else if ( i == 1 ) {

        if ( pix.first != pix.second ) {
            framedata[j] = 1;
        } else {
            framedata[j] = 0;
        }
    } else if ( i == 2 ) {

        if ( pix.first == 128 ) {
            framedata[j] = 1;
        } else {
            framedata[j] = 0;
        }
    } else if ( i == 3 ) {

        if ( pix.second == 128 ) {
            framedata[j] = 1;
        } else {
            framedata[j] = 0;
        }
    }

}
*/
