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

void DataTakingThread::ConnectToHardware() {

    SpidrController * spidrcontrol = _mpx3gui->GetSpidrController();

    // I need to do this here and not when already running the thread
    // Get the IP source address (SPIDR network interface) from the already connected SPIDR module.
    if ( spidrcontrol ) { spidrcontrol->getIpAddrSrc( 0, &_srcAddr ); }
    else { _srcAddr = 0; }

}

void DataTakingThread::run() {

    typedef std::chrono::high_resolution_clock Time;
    typedef std::chrono::nanoseconds ns;

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

    connect(this, SIGNAL(data_taking_finished(int)), _vis, SLOT(data_taking_finished(int)) );
    connect(_vis, SIGNAL(stop_data_taking_thread()), this, SLOT(on_stop_data_taking_thread()) ); // stop signal from qcstmglvis
    connect( this, &DataTakingThread::bufferFull,
             _vis, &QCstmGLVisualization::consumerBufferFull );

    unsigned long timeOutTime = 0;
    int contRWFreq = 0;
    QVector<int> activeDevices = _mpx3gui->getConfig()->getActiveDevices();
    int nChips = activeDevices.size();
    int * framedata = nullptr;
    int opMode = Mpx3Config::__operationMode_SequentialRW;

    int size_in_bytes = -1;
    int nFramesReceived = 0, nFramesKept = 0, lostFrames = 0, lostPackets = 0;
    bool emergencyStop = false;
    bool bothCounters = false;
    int totalFramesExpected = 0;
    uint64_t cntrLH = 0; //! Odd is Counter Low, even is Counter High

    unsigned int oneFrameChipCntr = 0;
    uint halfSemaphoreSize = 0;
    bool reachLimitStop = false;

    ///////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////

    forever {

        // When abort execution. Triggered as the destructor is called.
        if ( _abort ) return;

        // Fetch new parameters
        // After a start or restart
        _mutex.lock();
        datataking_score_info score = _score;
        opMode = _mpx3gui->getConfig()->getOperationMode();
        contRWFreq = _mpx3gui->getConfig()->getContRWFreq();

        //! May wish to use 10000 for trigger mode testing
        int overhead = 20; // ms

        if ( opMode == Mpx3Config::__operationMode_ContinuousRW ) {
            timeOutTime = uint((1/contRWFreq) //! Eg. 100 Hz --> 1/100 s = 10 ms
                                + overhead);
        } else {
            timeOutTime = uint(_mpx3gui->getConfig()->getTriggerLength_ms()
                                +  _mpx3gui->getConfig()->getTriggerDowntime_ms()
                                + overhead);
        }

        // dropFrames --> will use --> _vis->getDropFrames();
        //  since the user may want to activate/deactivate during data taking.
        halfSemaphoreSize = uint(_consumer->getSemaphoreSize()/2.);
        bothCounters = _mpx3gui->getConfig()->getReadBothCounters();
        //qDebug() << score.framesRequested << " | " << opMode << " | " << contRWFreq;
        totalFramesExpected = score.framesRequested;
        if ( bothCounters ) totalFramesExpected *= 2;
        //qDebug() << "total = " << totalFramesExpected;
        _mutex.unlock();

        // Reset
        spidrcontrol->resetCounters();
        spidrdaq->resetLostCount();
        nFramesReceived = 0; nFramesKept = 0; lostFrames = 0; lostPackets = 0;
        emergencyStop = false;
        reachLimitStop = false;
        cntrLH = 1;

        if ( opMode == Mpx3Config::__operationMode_ContinuousRW ) {
            spidrcontrol->startContReadout( contRWFreq );
        } else {
            spidrcontrol->startAutoTrigger();
        }

        // Ask SpidrDaq for frames
        while ( spidrdaq->hasFrame( timeOutTime ) ) {

            // When using both counters keep track of which Counter we are reading now
            // cntrLH%2 == 0  Cntr0 (THL0)
            // cntrLH%2 == 1  Cntr1 (THL1)

            if (bothCounters) {
                cntrLH++;
//                if ( cntrLH%2 == 0) {
//                    qDebug() << "Counter LOW" << cntrLH;
//                } else {
//                    qDebug() << "Counter HIGH" << cntrLH;
//                }
            }


            // 2) User stop condition
            if ( _stop ||  _restart  ) {
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
                continue;
            }

            //! T0 - Start of while loop to here : < 0.1% time
            //! --------------------------

            // An emergency stop when the consumer thread can't keep up
            if ( _consumer->freeFrames->available() < halfSemaphoreSize ) {
                qDebug() << " ... try to stop ";
                if ( ! emergencyStop ) {
                    emit bufferFull( 0 );
                    if ( opMode == Mpx3Config::__operationMode_ContinuousRW ) {
                        spidrcontrol->stopContReadout();
                    } else if ( opMode == Mpx3Config::__operationMode_SequentialRW ) {
                        spidrcontrol->stopAutoTrigger();
                    }
                    _consumer->consume();
                }
                emergencyStop = true;
            }
            //! T0-T1 < 0.1% time
            //! --------------------------

            /*QVector<int> v1;
            QVector<int> v2;
            QVector<int> v3;
            QVector<int> v4;*/

            // Read data
            oneFrameChipCntr = 0;          
            for ( int i = 0 ; i < nChips ; i++ ) {
                // retreive data for a given chip
                //auto t1 = Time::now();
                framedata = spidrdaq->frameData(i, &size_in_bytes); //! < 0.1% time
                //clearToCopy = true;
                //auto t2 = Time::now(); v1.append( std::chrono::duration_cast<ns>(t2 - t1).count());

                _consumer->freeFrames->acquire(); //! < 0.1% time
                //auto t3 = Time::now(); v2.append( std::chrono::duration_cast<ns>(t3 - t2).count());

                _consumer->copydata( framedata, size_in_bytes ); //! 99+% time
                //auto t4 = Time::now(); v3.append( std::chrono::duration_cast<ns>(t4 - t3).count());

                _consumer->usedFrames->release(); //! < 0.1% time
                //auto t5 = Time::now(); v4.append( std::chrono::duration_cast<ns>(t5 - t4).count());

                oneFrameChipCntr++;
            }
            //! T1-T2  93-98% time
            //! --------------------------

            // Release frame
            spidrdaq->releaseFrame();

            // The consumer thread is expecting N chips to be loaded
            // It can happen that the information from 1 or more chips
            // doesn't come through. This will put the consumer out of
            // sync since the descriptor can stay behind the readdescriptor.
            // In this case we will drop the frame.

            // Another reason to rewind is when there are lost packets
            // and the user has picked to drop the frames with lost packets

            lostPackets += spidrdaq->lostCount();

            if ( (oneFrameChipCntr != nChips) || (lostPackets > 0 && _vis->getDropFrames()) ) {
                for ( unsigned int i = 0 ; i < oneFrameChipCntr ; i++ ) {
                    //qDebug() << "Rewind >> " << i; // 0,1,2,3
                    // Free the resources and rewind descriptor
                    _consumer->usedFrames->acquire();
                    _consumer->rewindcopydata(size_in_bytes);
                    _consumer->freeFrames->release();
                }
                //qDebug() << " !!! REWIND !!! [" << oneFrameChipCntr << "]";
            }
            //! T2-T3  ~0.4-2.2% time
            //! --------------------------

            // If we are working with 2 counters, go get
            // the second frame associated before it continues.
//            qDebug() << "bothCounters & cntrLH : " << bothCounters << cntrLH << "\t Counter Low" << bool(cntrLH%2 == 0);
            if ( bothCounters && (cntrLH%2 == 0) ) continue;

            // Awake the consumer thread
            _consumer->consume();

            //! T3-T4  ~0.1-0.2% time
            //! --------------------------

            // Keep a local count of number of frames
            nFramesReceived++;
            // Keep a track of frames actually kept
            if ( ! ( spidrdaq->lostCount() > 0 && _vis->getDropFrames() ) ) nFramesKept++;
            // lost
            lostFrames += spidrdaq->framesLostCount() / nChips;

            emit scoring_sig(nFramesReceived,
                             nFramesKept,
                             lostFrames,                  //
                             lostPackets,                 // lost packets(ML605)/pixels(compactSPIDR)
                             spidrdaq->framesCount(),               // ?
                             0,
                             spidrdaq->framesLostCount() % nChips   // Data misaligned
                             );

            spidrdaq->resetLostCount();

            //! T4-T5  ~1-5% time
            //! --------------------------

            // How to stop in ContRW
            // 1) See Note 1 at the bottom
            //  note than score.framesRequested=0 when asking for infinite frames
            if ( (nFramesReceived + lostFrames == score.framesRequested) && score.framesRequested != 0 ) {
                if ( opMode == Mpx3Config::__operationMode_ContinuousRW ) {
                    spidrcontrol->stopContReadout();
                }
                reachLimitStop = true;
            }

            //! T5-T6 < 0.1% time
            //! --------------------------

            /*for (int i=0; i< v1.size(); i++){
              double sum = v1[i] + v2[i] + v3[i] + v4[i];
              qDebug() << v1[i]/sum*100 << "%  "
                       << v2[i]/sum*100 << "%  "
                       << v3[i]/sum*100 << "%  "
                       << v4[i]/sum*100 << "%";
            }*/
        }

        _consumer->consume();

        if ( ! _restart ) emit data_taking_finished( 0 );

        // Final report
        emit scoring_sig(nFramesReceived,
                         nFramesKept,
                         lostFrames,  //
                         spidrdaq->lostCount(),                 // lost packets(ML605)/pixels(compactSPIDR)
                         spidrdaq->framesCount(),               // ?
                         0,
                         spidrdaq->framesLostCount() % nChips   // Data misaligned
                         );

        // If this was an emergency stop
        // Have the consumer free resources
        if ( emergencyStop ) {
            _consumer->freeResources();
        }

        // Put the thread to wait
        //qDebug() << "   ... lock DataTakingThread";
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
    _restart = false;
    _abort = false;
    _idling = false;
    _stop = false;

    // clear counters in SpidrDac
    spidrdaq->resetLostCount();
    delete spidrcontrol;

    // Note1.  When for instance taking data in ContinuousRW mode, in order to stop the
    //  operation one can only invoke stopAutoTrigger.  In the meanwhile a few extra frames
    //  could have been taken, that we don't need.  But it seems to be important to let
    //  spidrdaq finish evacuating frames. Once this is done the last frame seems to contain artifacts.
    //  Read it out but don't schedule it for processing.

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

/*
void DataTakingThread::on_busy_drawing() {
    _canDraw = false;
}

void DataTakingThread::on_free_to_draw() {
    _canDraw = true;
}
*/

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
