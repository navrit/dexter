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

#include <chrono>

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

    rewindScoring();
}

DataTakingThread::~DataTakingThread() {

    _mutex.lock();
    _stop = false;
    _abort = true;          // will stop run as soon as possible
    _condition.wakeOne();   // wake up if sleeping
    _mutex.unlock();

    wait(); // wait 'til run has exited before the base class destructor is invoked

    //qDebug() << "   DataTakingThread finished";

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

void DataTakingThread::setExternalTrigger(bool external)
{
    _isExternalTrigger = external;
}

void DataTakingThread::ConnectToHardware() {

    SpidrController * spidrcontrol = _mpx3gui->GetSpidrController();

    // I need to do this here and not when already running the thread
    // Get the IP source address (SPIDR network interface) from the already connected SPIDR module.
    if ( spidrcontrol ) { spidrcontrol->getIpAddrSrc( 0, &_srcAddr ); }
    else { _srcAddr = 0; }

}

void DataTakingThread::run() {

    //typedef std::chrono::high_resolution_clock Time;
    //typedef std::chrono::nanoseconds ns;

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
        qDebug() << "[ERROR]\tDevice not connected\n";
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

    connect(this, SIGNAL(data_taking_finished(int)), _vis, SLOT(data_taking_finished(int)) );
    connect(_vis, SIGNAL(stop_data_taking_thread()), this, SLOT(on_stop_data_taking_thread()) ); // stop signal from qcstmglvis
    connect( this, &DataTakingThread::bufferFull,
             _vis, &QCstmGLVisualization::consumerBufferFull );

    connect(this,SIGNAL(sendingShutter()),_mpx3gui,SLOT(sendingShutter()));



    unsigned long timeOutTime = 0;
    int contRWFreq = 0;
    QVector<int> activeDevices = _mpx3gui->getConfig()->getActiveDevices();
    int nChips = activeDevices.size();
    int opMode = Mpx3Config::__operationMode_SequentialRW;

    int nFramesReceived = 0, nFramesKept = 0, lostFrames = 0, lostPackets = 0;
    bool emergencyStop = false;
    /* Not necessary to track if it's colour mode of not
     * bool colourMode = false;
     */

    uint halfSemaphoreSize = 0;
    bool reachLimitStop = false;

    ///////////////////////////////////////////////////////////////////////////////////

    forever {
        _mpx3gui->getConfigMonitoring()->returnLastTriggerMode2(spidrcontrol);
        // When abort execution. Triggered as the destructor is called.
        if ( _abort ) return;

        // Fetch new parameters
        // After a start or restart
        _mutex.lock();
        datataking_score_info score = _score;
        opMode = _mpx3gui->getConfig()->getOperationMode();
        contRWFreq = _mpx3gui->getConfig()->getContRWFreq();

        //! May wish to use 10000 for trigger mode testing
     //   int overhead = 200; // ms

//        if ( opMode == Mpx3Config::__operationMode_ContinuousRW ) {
//            timeOutTime = uint((1000/contRWFreq) //! Eg. 100 Hz --> 1/100 s = 10 ms
//                                + overhead);
//        } else {
//            timeOutTime = uint(_mpx3gui->getConfig()->getTriggerLength_ms()
//                                +  _mpx3gui->getConfig()->getTriggerDowntime_ms()
//                                + overhead);
//        }

        // dropFrames --> will use --> _vis->getDropFrames();
        //  since the user may want to activate/deactivate during data taking.
        halfSemaphoreSize = uint(_consumer->getSemaphoreSize()/2.);
        bool bothCounters = _mpx3gui->getConfig()->getReadBothCounters();

        _mpx3gui->clear_data(false);
        _mutex.unlock();

        // Reset
        spidrcontrol->resetCounters();
        spidrdaq->resetLostCount();
        nFramesReceived = 0; nFramesKept = 0; lostFrames = 0; lostPackets = 0;
        emergencyStop = false;
        reachLimitStop = false;

        spidrdaq->releaseAll();

            bool stopTimers = true;
            if ( opMode == Mpx3Config::__operationMode_ContinuousRW ) {
                spidrcontrol->startContReadout( contRWFreq );
                stopTimers = false;
            } else if(opMode == Mpx3Config::__operationMode_SequentialRW && (_mpx3gui->getConfig()->getTriggerLength_ms_64() + _mpx3gui->getConfig()->getTriggerDowntime_ms_64() <= LONG_PERIOD_MS)&&!_isExternalTrigger){
                spidrcontrol->startAutoTrigger();
                stopTimers = false;
            }




        bool _break = false;
        timeOutTime = 500;
        if(opMode == Mpx3Config::__operationMode_SequentialRW)
            emit sendingShutter();
        while(!_break && !_stop){
        // Ask SpidrDaq for frames
            while ( spidrdaq->hasFrame(timeOutTime)) {

                // 2) User stop condition
                if ( _stop ||  _restart  ) {
                    _break = true;
                    if(_isExternalTrigger){
                        _mpx3gui->getConfigMonitoring()->protectTriggerMode(spidrcontrol);
                        break;
                    }

                    if ( opMode == Mpx3Config::__operationMode_ContinuousRW ) {
                        spidrcontrol->stopContReadout();
                    } else { //if ( opMode == Mpx3Config::__operationMode_SequentialRW ) {
                        spidrcontrol->stopAutoTrigger();
                    }
                }

                // Three stopping conditions where I still need to let the SpidrDaq::hasFrame loop to finish
                // 1) If trying to stop a ContRW
                // 2) User stop
                // 3) Restart
                if ( reachLimitStop || _stop || _restart ) {
                    spidrdaq->releaseFrame();
                    _consumer->consume();
                    _break = true;
                    continue;
                }

                //! T0 - Start of while loop to here : < 0.1% time
                //! --------------------------

                // An emergency stop when the consumer thread can't keep up
                if ( _consumer->freeFrames->available() < halfSemaphoreSize ) {
                    qDebug() << "[WARNING]\tConsumer trying to stop ";
                    if ( !emergencyStop ) {
                        emit bufferFull( 0 );
                        if ( opMode == Mpx3Config::__operationMode_ContinuousRW ) {
                            spidrcontrol->stopContReadout();
                        } else if ( opMode == Mpx3Config::__operationMode_SequentialRW ) {
                            spidrcontrol->stopAutoTrigger();
                        }
                        _consumer->consume();
                    }
                    emergencyStop = true;
                    _break = true;
                }
                //! T0-T1 < 0.1% time
                //! --------------------------

                // Read data
                FrameSet *fs = spidrdaq->getFrameSet();

                // The consumer thread is expecting N chips to be loaded
                // It can happen that the information from 1 or more chips
                // doesn't come through. This will put the consumer out of
                // sync since the descriptor can stay behind the readdescriptor.
                // In this case we will drop the frame.

                if (fs->isComplete() && ! (fs->pixelsLost() > 0 && _vis->getDropFrames())) {

                    for ( int i = 0 ; i < nChips ; i++ ) {
                        // retreive data for a given chip
                        //clearToCopy = true;
                        _consumer->freeFrames->acquire(); //! < 0.1% time
                        _consumer->copydata(fs, i, false); //! 99+% time
                        _consumer->usedFrames->release(); //! < 0.1% time
                    }
                    if (bothCounters)
                        for ( int i = 0 ; i < nChips ; i++ ) {
                            // retreive data for a given chip
                            //clearToCopy = true;
                            _consumer->freeFrames->acquire(); //! < 0.1% time
                            _consumer->copydata(fs, i, true); //! 99+% time
                            _consumer->usedFrames->release(); //! < 0.1% time
                        }
                    // Keep a track of frames actually kept
                    nFramesKept++;
                }

                //! T1-T2  93-98% time
                //! --------------------------

                lostPackets += fs->pixelsLost();

                // Release frame
                spidrdaq->releaseFrame(fs);

                //! T2-T3  ~0.4-2.2% time
                //! --------------------------

                // Awake the consumer thread
                _consumer->consume();

                //! T3-T4  ~0.1-0.2% time
                //! --------------------------

                // Keep a local count of number of frames
                nFramesReceived++;
                // lost
                lostFrames += spidrdaq->framesLostCount();

                if(!_isExternalTrigger)
                {
                    emit scoring_sig(nFramesReceived,
                                     nFramesKept,
                                     lostFrames,                  //
                                     lostPackets,                 // lost packets(ML605)/pixels(compactSPIDR)
                                     spidrdaq->framesCount(),     // ?
                                     0,
                                     0							  // Data misaligned?
                                     );
                }

                spidrdaq->resetLostCount();

                //! T4-T5  ~1-5% time
                //! --------------------------

                // How to stop in ContRW
                // 1) See Note 1 at the bottom
                //  note than score.framesRequested=0 when asking for infinite frames
                if ( (nFramesReceived >= score.framesRequested) && score.framesRequested > 0 && !_isExternalTrigger ) {
                    _break = true;
                    if ( opMode == Mpx3Config::__operationMode_ContinuousRW ) {
                        spidrcontrol->stopContReadout();
                    }
                    else{
                        spidrcontrol->stopAutoTrigger();
                        break;
                    }
                    reachLimitStop = true;
                }

                //! T5-T6 < 0.1% time
                //! --------------------------

                if( _isExternalTrigger ) {
                    int externalCnt = 0;
                    spidrcontrol->getExtShutterCounter(&externalCnt);
                    //Debug() << "[Debug] ext count : " << externalCnt;
                    emit scoring_sig(externalCnt,
                                     externalCnt,
                                     lostFrames,                  //
                                     lostPackets,                 // lost packets(ML605)/pixels(compactSPIDR)
                                     spidrdaq->framesCount(),               // ?
                                     0,
                                     0										// Data misaligned?
                                     );

                    if (externalCnt >= _mpx3gui->getConfig()->getNTriggers() && _mpx3gui->getConfig()->getNTriggers() != 0 ){
                        _mpx3gui->getConfigMonitoring()->protectTriggerMode(spidrcontrol);
                        _break = true;
                        break;
                    }
                }
            }
        }
        if(stopTimers){
            spidrcontrol->stopAutoTrigger();
            _mpx3gui->stopTriggerTimers();
            qDebug() << "[Info]\t Software triggering stopped.";
        }

        _mpx3gui->getConfigMonitoring()->protectTriggerMode(spidrcontrol);
        _consumer->consume();

        if ( ! _restart ) {  emit data_taking_finished( 0 );}

        // If this was an emergency stop
        // Have the consumer free resources
        if ( emergencyStop ) {
            _consumer->freeResources();
        }

        _mutex.lock();
        if ( !_restart ) {
            _idling = true;
            _condition.wait( &_mutex );
        }
        _idling = false;
        _restart = false;

        _mutex.unlock();

    } // forever

    ///////////////////////////////////////////////////////////////////////////////////

    disconnect(this, SIGNAL(scoring_sig(int,int,int,int,int,int,bool)),
               _vis,   SLOT( on_scoring(int,int,int,int,int,int,bool)) );

    disconnect(this, SIGNAL(data_taking_finished(int)), _vis, SLOT(data_taking_finished(int)));
    disconnect(_vis, SIGNAL(stop_data_taking_thread()), this, SLOT(on_stop_data_taking_thread())); // stop signal from qcstmglvis

    disconnect(this,SIGNAL(sendingShutter()),_mpx3gui,SLOT(sendingShutter()));
    // In case the thread is reused
    _restart = false;
    _abort = false;
    _idling = false;
    _stop = false;

    // clear counters in SpidrDaq
    spidrdaq->resetLostCount();
    delete spidrcontrol;

    // Note1.  When for instance taking data in ContinuousRW mode, in order to stop the
    //  operation one can only invoke stopAutoTrigger.  In the meanwhile a few extra frames
    //  could have been taken, that we don't need.  But it seems to be important to let
    //  spidrdaq finish evacuating frames. Once this is done the last frame seems to contain artifacts.
    //  Read it out but don't schedule it for processing.

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

void DataTakingThread::on_stop_data_taking_thread() {

    // Used to properly stop the data taking thread
    _stop = true;

}


