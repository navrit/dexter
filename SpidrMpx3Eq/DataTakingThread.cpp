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

    _stop = false;

    if ( ! isRunning() ) {
        start( HighestPriority );
    } else {
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

void setTriggerMode(SpidrController *spidrController, Mpx3Config *config)
{
    spidrController->setShutterTriggerConfig(config->getTriggerMode(),
        config->getTriggerLength_64(),
        config->getTriggerFreq_mHz(),
        config->getNTriggers(),
        0);
}

void DataTakingThread::stopReadout(SpidrController* spidrcontrol, Mpx3Config *config) {
    if (config->getOperationMode() == Mpx3Config::__operationMode_ContinuousRW) {
        spidrcontrol->stopContReadout();
    } else {
        spidrcontrol->stopAutoTrigger();
        if (_isExternalTrigger) {
            spidrcontrol->setShutterTriggerConfig(SHUTTERMODE_AUTO,
                config->getTriggerLength_64(),
                config->getTriggerFreq_mHz(),
                config->getNTriggers(),
                0);
        }
        if (_isSoftwareTrigger) {
            _mpx3gui->stopTriggerTimers();
            qDebug() << "[INFO]\tSoftware triggering stopped";
        }
    }
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
        qDebug() << "[ERROR]\tDevice not connected\n";
        return;
    }

    spidrcontrol->setLogLevel( 0 );

    SpidrDaq * spidrdaq = _mpx3gui->GetSpidrDaq();

    connect(this, SIGNAL(scoring_sig(int,int,int,int,int)),
            _vis,   SLOT( on_scoring(int,int,int,int,int)) );

    connect(this, SIGNAL(dataTakingFinished()), _vis, SLOT(dataTakingFinished()) );
    connect(_vis, SIGNAL(stop_data_taking_thread()), this, SLOT(on_stop_data_taking_thread()) ); // stop signal from qcstmglvis
    connect( this, &DataTakingThread::bufferFull,
             _vis, &QCstmGLVisualization::consumerBufferFull );

    connect(this,SIGNAL(sendingShutter()),_mpx3gui,SLOT(sendingShutter()));


    auto config = _mpx3gui->getConfig();
    QVector<int> activeDevices = config->getActiveDevices();

    /* Not necessary to track if it's colour mode of not
     * bool colourMode = false;
     */

    ///////////////////////////////////////////////////////////////////////////////////

    forever {
        // When abort execution. Triggered as the destructor is called.
        if ( _abort ) return;

        // Fetch new parameters
        // After a start or restart
        _mutex.lock();
        int opMode = config->getOperationMode();
        int contRWFreq = config->getContRWFreq();

        //! May wish to use 10000 for trigger mode testing
     //   int overhead = 200; // ms

        // dropFrames --> will use --> _vis->getDropFrames();
        //  since the user may want to activate/deactivate during data taking.
        bool bothCounters = config->getReadBothCounters();

        _mpx3gui->clear_data(false);
        _mutex.unlock();

        int chipMask = spidrdaq->chipMask;

        // Reset
        spidrcontrol->resetCounters();
        spidrdaq->resetLostCount();
        int nFramesReceived = 0, nFramesKept = 0, lostFrames = 0, lostPackets = 0;
        spidrdaq->releaseAll();
        _isSoftwareTrigger = false;
        _isExternalTrigger = false;

        if ( opMode == Mpx3Config::__operationMode_ContinuousRW ) {
            spidrcontrol->startContReadout( contRWFreq );
        } else if (opMode == Mpx3Config::__operationMode_SequentialRW) {
            switch (config->getTriggerMode()) {
            case SHUTTERMODE_AUTO:
                if (config->getTriggerPeriod() < LONG_PERIOD_US) {
                    setTriggerMode(spidrcontrol, config);
                    spidrcontrol->startAutoTrigger();
                    break;
                }
                [[fallthrough]];
            case SHUTTERMODE_SOFTWARE:
                // software trigger
                _isSoftwareTrigger = true;
                spidrcontrol->setShutterTriggerConfig(SHUTTERMODE_AUTO, 0, 1, 1, 0);
                emit sendingShutter();
                break;
            default:
                _isExternalTrigger = true;
                setTriggerMode(spidrcontrol, config);
                spidrcontrol->startAutoTrigger();
            }
        }

        int framesRequested = config->getNTriggers();
        if (framesRequested == 0) framesRequested = INT_MAX;

        // TODO: refine the time out
        unsigned long timeOutTime_ms = 500;

        bool daqRunning = true;
        while (nFramesReceived < framesRequested) {

            // 2) User stop condition
            if (_stop || _restart) {
                break;
            }

            if (daqRunning && (framesRequested - nFramesKept < spidrdaq->framesAvailable() - 5)) {
                // there's enough in the queue, stop acquisition
                stopReadout(spidrcontrol, config);
                daqRunning = false;
            }

            if (spidrdaq->hasFrame(timeOutTime_ms)) {

                // Read data
                FrameSet *fs = spidrdaq->getFrameSet();

                // The consumer thread is expecting N chips to be loaded
                // It can happen that the information from 1 or more chips
                // doesn't come through. This will put the consumer out of
                // sync since the descriptor can stay behind the readdescriptor.
                // In this case we will drop the frame.

                if (bothCounters && ! fs->hasBothCounters()) {
                    qDebug() << "[ERROR]\tFrameSet should have both counters but it hasn't";
                }

                if (fs->isComplete(chipMask) && ! (fs->pixelsLost() > 0 && _vis->getDropFrames())) {

                    foreach (int i, activeDevices) {
                        // retreive data for a given chip
                        _consumer->freeFrames->acquire();
                        _consumer->copydata(fs, i, false);
                        _consumer->usedFrames->release();
                    }
                    if (bothCounters)
                        foreach (int i, activeDevices) {
                            // retreive data for a given chip
                            _consumer->freeFrames->acquire();
                            _consumer->copydata(fs, i, true);
                            _consumer->usedFrames->release();
                        }
                    nFramesKept++;

                    _consumer->consume();
                }

                lostPackets += fs->pixelsLost();

                // Release frame
                spidrdaq->releaseFrame(fs);

                // Keep a local count of number of frames
                nFramesReceived++;
                // lost
                lostFrames += spidrdaq->framesLostCount();

                spidrdaq->resetLostCount();

                if( false) {
                    int externalCnt = 0;
                    spidrcontrol->getExtShutterCounter(&externalCnt);
                    //Debug() << "[Debug] ext count : " << externalCnt;
                    emit scoring_sig(externalCnt,
                                     nFramesKept,
                                     lostFrames,                  //
                                     lostPackets,                 // lost packets(ML605)/pixels(compactSPIDR)
                                     spidrdaq->framesCount());
                } else {
                    emit scoring_sig(nFramesReceived,
                                     nFramesKept,
                                     lostFrames,                  //
                                     lostPackets,                 // lost packets(ML605)/pixels(compactSPIDR)
                                     spidrdaq->framesCount());
                }
            }
        }
        if (daqRunning) {
            stopReadout(spidrcontrol, config);
        }
        msleep(50);
        spidrdaq->releaseAll();

        if ( ! _restart ) {  emit dataTakingFinished();}

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

    disconnect(this, SIGNAL(scoring_sig(int,int,int,int,int)),
               _vis,   SLOT( on_scoring(int,int,int,int,int)) );

    disconnect(this, SIGNAL(dataTakingFinished()), _vis, SLOT(dataTakingFinished()));
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


