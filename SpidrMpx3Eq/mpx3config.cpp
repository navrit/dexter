#include "mpx3config.h"
#include "qcstmconfigmonitoring.h"
#include "ui_qcstmconfigmonitoring.h"
#include "mpx3dacsdescr.h"
#include <iterator>
#include <iostream>

#include <QFile>
#include <QDebug>

#include "SpidrController.h"
#include "SpidrDaq.h"


using namespace std;


Mpx3Config::Mpx3Config()
{
    // number of devices connected
    _devicePresenceLayout.clear();
    _nDevicesPresent = 0;
    _trigPeriod_ms = 0;

    controller = nullptr;

}

void Mpx3Config::SendConfiguration(){

    // Configure the chips
    int nDevSupported = getNDevicesSupported();
    for ( int i = 0 ; i < nDevSupported ; i++ ) {
        if ( detectorResponds( i ) ) {
            Configuration( false, i );
        }
    }

}

void Mpx3Config::Configuration(bool reset, int deviceIndex) {

    SpidrController * spidrcontrol = _mpx3gui->GetSpidrController();
    SpidrDaq * spidrdaq = _mpx3gui->GetSpidrDaq();

    // Reset pixel configuration
    if ( reset ) spidrcontrol->resetPixelConfig();

    // All adjustment bits to zero
    //SetAllAdjustmentBits(0x0, 0x0);

    // Operation mode
    if ( OperationMode == __operationMode_SequentialRW ) {
        spidrcontrol->setContRdWr( deviceIndex, false );
    } else if ( OperationMode == __operationMode_ContinuousRW ) {
        spidrcontrol->setContRdWr( deviceIndex, true );
    } else {
        spidrcontrol->setContRdWr( deviceIndex, false );
    }

    // OMR
    spidrcontrol->setPolarity( deviceIndex, getPolarity() );		// true: Holes collection
    //cout << " | polarity: " << getPolarity();
    //_spidrcontrol->setDiscCsmSpm( 0 );		// DiscL used
    //_spidrcontrol->setInternalTestPulse( true ); // Internal tests pulse

    // Not an equalization
    spidrcontrol->setEqThreshH( deviceIndex, false );

    spidrcontrol->setColourMode( deviceIndex, getColourMode() ); // false 	// Fine Pitch
    spidrcontrol->setCsmSpm( deviceIndex, getCsmSpm() ); // 0 );				// Single Pixel mode

    // Particular for Equalization
    //spidrcontrol->setEqThreshH( deviceIndex, true );
    //spidrcontrol->setDiscCsmSpm( deviceIndex, 0 );		// In Eq mode using 0: Selects DiscL, 1: Selects DiscH
    //_spidrcontrol->setGainMode( 1 );

    // Gain ?!
    // 00: SHGM  0
    // 10: HGM   2
    // 01: LGM   1
    // 11: SLGM  3
    spidrcontrol->setGainMode( deviceIndex, getGainMode() );

    // Other OMR
    spidrdaq->setDecodeFrames(  getDecodeFrames() ); //  true );
    //qDebug() << " (" << getPixelDepth() << ":" << getReadBothCounters() << ") ";
    spidrcontrol->setPixelDepth( deviceIndex, getPixelDepth(), getReadBothCounters() ); // third parameter : true = read two counters
    spidrdaq->setPixelDepth( getPixelDepth() );
    spidrcontrol->setMaxPacketSize( getMaxPacketSize() );

    // Write OMR ... i shouldn't call this here
    //_spidrcontrol->writeOmr( 0 );

    // Trigger config
    int trig_mode      = getTriggerMode();       // Auto-trigger mode = 4
    int trig_length_us = getTriggerLength();     // This time shouldn't be longer than the period defined by trig_freq_hz
    int trig_deadtime_us = getTriggerDowntime();

    //int trig_freq_mhz   = (int) 1000 * ( 1. / (2.*((double)trig_length_us/1000000)) );   // Make the period double the trig_len
    //int trig_freq_mhz   = (int) 1000 * ( 1. / (1.1*((double)trig_length_us/1000000)) );   //
    int trig_freq_mhz   = (int) 1000 * ( 1. / ((double)( trig_length_us + trig_deadtime_us )/1000000) );
    //int trig_freq_hz   = (int) ( 1. / ((double)( trig_length_us + trig_deadtime_us )/1000000) );

    //cout << " | configured freq is " << trig_freq_mhz << "mHz";

    // Get the trigger period for information.  This is NOT the trigger length !
    _trigPeriod_ms = (int) (1E6 * (1./(double)trig_freq_mhz));
    //_trigPeriod_ms /= 1000;
    int nr_of_triggers = getNTriggers();    // This is the number of shutter open i get

    qDebug() << "[CONF] id:" << deviceIndex
             << "| trig_mode:" << trig_mode
             << "| trig_length_us:" << trig_length_us
             << "| trig_deadtime_us: " << trig_deadtime_us
             << "| trig_freq_mhz:" <<  trig_freq_mhz
             << "| nr_of_triggers:" << nr_of_triggers;

    // Send off the trigger settings
    spidrcontrol->setShutterTriggerConfig (
                trig_mode,
                trig_length_us,
                trig_freq_mhz,
                nr_of_triggers
                );

    //cout << endl;
}


void Mpx3Config::Configuration(bool reset, int deviceIndex, scan_config_parameters extrapars) {
    cout << "[INFO] Configuring chip " << deviceIndex;

    SpidrController * spidrcontrol = _mpx3gui->GetSpidrController();
    SpidrDaq * spidrdaq = _mpx3gui->GetSpidrDaq();

    // Reset pixel configuration
    if ( reset ) spidrcontrol->resetPixelConfig();

    // Operation mode
    if ( OperationMode == __operationMode_SequentialRW ) {
        spidrcontrol->setContRdWr( deviceIndex, false );
    } else if ( OperationMode == __operationMode_ContinuousRW ) {
        spidrcontrol->setContRdWr( deviceIndex, true );
    } else {
        spidrcontrol->setContRdWr( deviceIndex, false );
    }

    // All adjustment bits to zero
    //SetAllAdjustmentBits(0x0, 0x0);

    // OMR
    //spidrcontrol->setPolarity( true );		// Holes collection
    //_spidrcontrol->setDiscCsmSpm( 0 );		// DiscL used
    //_spidrcontrol->setInternalTestPulse( true ); // Internal tests pulse

    // Not an equalization
    spidrcontrol->setEqThreshH( deviceIndex, extrapars.equalizationBit );

    spidrcontrol->setColourMode( deviceIndex, getColourMode() ); // false 	// Fine Pitch
    spidrcontrol->setCsmSpm( deviceIndex, getCsmSpm() ); // 0 );				// Single Pixel mode

    // Particular for Equalization
    //spidrcontrol->setEqThreshH( deviceIndex, true );
    spidrcontrol->setDiscCsmSpm( deviceIndex, extrapars.DiscCsmSpm );		// In Eq mode using 0: Selects DiscL, 1: Selects DiscH
    //_spidrcontrol->setGainMode( 1 );

    // Gain ?!
    // 00: SHGM  0
    // 10: HGM   2
    // 01: LGM   1
    // 11: SLGM  3
    spidrcontrol->setGainMode( deviceIndex, getGainMode() );

    // Other OMR
    spidrdaq->setDecodeFrames(  getDecodeFrames() ); //  true );
    spidrcontrol->setPixelDepth( deviceIndex, getPixelDepth(), getReadBothCounters() ); // third parameter : true = read two counters
    spidrdaq->setPixelDepth( getPixelDepth() );
    spidrcontrol->setMaxPacketSize( getMaxPacketSize() );

    // Write OMR ... i shouldn't call this here
    //_spidrcontrol->writeOmr( 0 );

    // Trigger config
    int trig_mode      = getTriggerMode();     // Auto-trigger mode = 4
    int trig_length_us = getTriggerLength();  // This time shouldn't be longer than the period defined by trig_freq_hz

    //int trig_freq_mhz   = (int) 1000 * ( 1. / (2.*((double)trig_length_us/1000000)) );   // Make the period double the trig_len
    int trig_freq_mhz   = (int) 1000 * ( 1. / (1.1*((double)trig_length_us/1000000)) );   // Make the period double the trig_len
    //int trig_freq_hz   = (int) ( 1. / (1.1*((double)trig_length_us/1000000)) );   // Make the period double the trig_len

    // Get the trigger period for information.  This is NOT the trigger length !
    _trigPeriod_ms = (int) (1E6 * (1./(double)trig_freq_mhz));
    //_trigPeriod_ms /= 1000;
    //int nr_of_triggers = getNTriggers();    // This is the number of shutter open i get

    qDebug() << "[CONF] id:" << deviceIndex
             << "| trig_mode:" << trig_mode
             << "| trig_length_us:" << trig_length_us
             << "| trig_freq_mhz:" <<  trig_freq_mhz
             << "| nr_of_triggers:" << extrapars.nTriggers;

    spidrcontrol->setShutterTriggerConfig( trig_mode, trig_length_us,
                                           trig_freq_mhz, extrapars.nTriggers );

}

SpidrController * Mpx3Config::establishConnection(){

    // number of devices connected
    _devicePresenceLayout.clear();
    _nDevicesPresent = 0;
    _activeChips.clear();

    // Addres from Json config file
    quint32 ipaddr =  SpidrAddress.toIPv4Address();
    //cout << SpidrAddress.toString().toStdString() << endl;

    // If previously connected
    if ( controller ) delete controller;
    controller = new SpidrController(((ipaddr>>24) & 0xFF), ((ipaddr>>16) & 0xFF), ((ipaddr>>8) & 0xFF), ((ipaddr>>0) & 0xFF), port);
    connected = controller->isConnected();

    // Connection failed
    if ( ! connected ) return controller;

    // number of device that the system can support
    controller->getDeviceCount(&_nDevicesSupported);

    //! Work around
    //! If we attempt a connection while the system is already sending data
    //! (this may happen if for instance the program died for whatever reason,
    //!  or when it is close while a very long data taking has been lauched and
    //! the system failed to stop the data taking).  If this happens we ought
    //! to stop data taking, and give the system a bit of delay.
    controller->stopAutoTrigger();
    Sleep( 100 );

    // Response
    _responseChips = QVector<detector_response>( _nDevicesSupported );

    // Run a simple reponse check by reaching a DAC setting
    for(int i = 0 ; i < _nDevicesSupported ; i++) {

        int id = 0;
        controller->getDeviceId(i, &id);

        QDebug dbg(QtInfoMsg);
        dbg << "--- Device [" << i << "] ---";

        if ( id != 0 ) {

            dbg << "Id :" << id << "|";
            _devicePresenceLayout.push_back( QPoint(__default_matrixSizePerChip_X, __default_matrixSizePerChip_Y) );
            _nDevicesPresent++;

            // If connected check response
            checkChipResponse( i, __CONTROLLER_OK ); // this fills a vector

            // And then I just consult that vector
            if( detectorResponds(i) ) {
                _activeChips.push_back(i);
                dbg << "OK";
            }

        } else {

            dbg << "NOT RESPONDING !";
            _devicePresenceLayout.push_back( QPoint(0, 0) );
            // If not connected tag it immediately
            _responseChips[i] = __NOT_RESPONDING;

        }

        // the CR for the QDebug object is emmited when the object is destroyed

    }

    return controller;
}

void Mpx3Config::destroyController()
{
    if ( controller != nullptr ) {
        delete controller;
        controller = nullptr;
    }
}

/*
 * If there is only one device connected, no matter what the devIndx is, the data is always at DataBuffer 0
 * Otherwise, for instance if only devices 0 and 2 are connected.  The data will be found in 0,1.
 * This member computes that transform using the detected status of the chips.
 */
int Mpx3Config::getDataBufferId(int devIndx) {

    // If there is only one device connected, no matter what the devIndx is, the data is always at DataBuffer 0
    if ( getNDevicesPresent() == 1 ) return 0;

    // Otherwise, for instance if only devices 0 and 2 are connected.  The data will be found in 0,1
    int dataBufferId = -1;
    for ( int i = 0 ; i < _nDevicesSupported ; i++ ) {
        if ( _responseChips[i] != __NOT_RESPONDING ) dataBufferId++;
        if ( devIndx == i ) return dataBufferId;
    }

    return -1;
}

unsigned int Mpx3Config::getPixelDepthFromIndex(int indx) {

    int size = sizeof(__pixelDepthMap) / sizeof(const unsigned int);
    if ( indx >= size ) return __pixelDepthMap[ __pixelDepth12BitsIndex ]; // 12 bits

    return __pixelDepthMap[indx];
}

void Mpx3Config::checkChipResponse(int devIndx, detector_response dr) {

    if ( dr == __CONTROLLER_OK ) { // Check if the detector responds ok to the Controller

        // For instance try to read a DAC
        int dac_val = 0;

        if ( ! controller->setDac( devIndx, MPX3RX_DAC_TABLE[0].code, dac_val ) ) {
            //cout << "chip response failed : "  << controller->errorString();
            _responseChips[devIndx] = __NOT_RESPONDING;
        } else {
            //cout << "Response OK";
            _responseChips[devIndx] = __CONTROLLER_OK;
        }

    }

}

void Mpx3Config::setTriggerDowntime(int newVal) {

    // There's a minimum setting for downtime
    if ( newVal < __min_trigger_deadtime_ms ) newVal = __min_trigger_deadtime_ms;

    if ( newVal != TriggerDowntime_us ) {
        TriggerDowntime_us = newVal; emit TriggerDowntimeChanged(newVal);
        //updateTriggerLength();
    }
    SendConfiguration();
}

// This is connected to QAbstractSpinBox::editingFinished() which takes no argument.
// Pick the value from the spin-box directly.
void Mpx3Config::setTriggerDowntime() {

    int newVal = _mpx3gui->getConfigMonitoring()->getUI()->triggerDowntimeSpinner->value();
    setTriggerDowntime( newVal );

}


void Mpx3Config::setPolarityByString(QString itemS, int indx) {


    bool polarityChange = Polarity;

    if( itemS.contains("pos", Qt::CaseInsensitive) ) polarityChange = true; // positive, holes collection
    else if ( itemS.contains("neg", Qt::CaseInsensitive) ) polarityChange = false; // negative
    else polarityChange = true; //default

    // Now find the index to update the ComboBox
    // if indx == -1 (default in this member) it means that we are coming from
    //  a place where we don't know the index, like the json config.  In this case
    //  find the right index.

    if ( indx == -1 ) {
        // See how the polarity list was registered
        QComboBox * cb = _mpx3gui->getConfigMonitoring()->getUI()->polarityComboBox;
        indx = cb->findText( itemS, Qt::MatchContains);
    }

    if ( polarityChange != Polarity ) {
        Polarity = polarityChange;
        emit polarityChanged(indx);
        //updateGainMode();
        cout << "Polarity : " << itemS.toStdString() << " | item : " << indx << endl;

    }
    SendConfiguration();

}

void Mpx3Config::setPolarity(int newVal) {

    // See how the polarity list was registered
    QComboBox * cb = _mpx3gui->getConfigMonitoring()->getUI()->polarityComboBox;
    // Pick the right item
    QString itemS = cb->itemText(newVal);
    // And set it by string
    setPolarityByString( itemS, newVal );

}

bool Mpx3Config::detectorResponds(int devIndx) {

    if( devIndx >= _nDevicesSupported || devIndx < 0) {
        qDebug() << "[ERR ] device index out of range: " << devIndx;
        return false;
    }

    if ( _responseChips[devIndx] > __NOT_RESPONDING ) return true;

    return false;
}

bool Mpx3Config::fromJsonFile(QString filename, bool includeDacs){

    qDebug() << "[INFO] reading the configuration from the Json file: " << filename.toStdString().c_str();

    QFile loadFile(filename);
    if(!loadFile.open(QIODevice::ReadOnly)){
        printf("Couldn't open configuration file %s\n", filename.toStdString().c_str());
        return false;
    }
    QByteArray binaryData = loadFile.readAll();
    QJsonObject JSobjectParent = QJsonDocument::fromJson(binaryData).object();
    QJsonObject::iterator it, itParent;
    itParent = JSobjectParent.find("IPConfig");
    if(itParent != JSobjectParent.end()){
        QJsonObject JSobject = itParent.value().toObject();
        it = JSobject.find("SpidrControllerIp");
        if(it != JSobject.end())
            setIpAddress(it.value().toString());
        it = JSobject.find("SpidrControllerPort");
        if(it != JSobject.end())
            setPort(it.value().toInt());
    }
    itParent = JSobjectParent.find("DetectorConfig");
    if(itParent != JSobjectParent.end()){

        QJsonObject JSobject = itParent.value().toObject();

        it = JSobject.find("OperationMode");
        if(it != JSobject.end())
            setOperationMode(it.value().toInt());

        it = JSobject.find("PixelDepth");
        if(it != JSobject.end())
            setPixelDepth(it.value().toInt());

        // The user set polarity with the keywords: "pos":positive, "neg":negative
        // Here we convert it to what is needed by the internal functions
        it = JSobject.find("Polarity");
        if(it != JSobject.end()) {
            QString itemS = it.value().toString();
            setPolarityByString( itemS );
        }

        it = JSobject.find("CsmSpm");
        if(it != JSobject.end())
            setCsmSpm(it.value().toInt());

        it = JSobject.find("GainMode");
        if(it != JSobject.end())
            setGainMode(it.value().toInt());

        it = JSobject.find("MaxPacketSize");
        if(it != JSobject.end())
            setMaxPacketSize(it.value().toInt());

        it = JSobject.find("TriggerMode");
        if(it != JSobject.end())
            setTriggerMode(it.value().toInt());

        it = JSobject.find("TriggerLength_us");
        if(it != JSobject.end())
            setTriggerLength(it.value().toInt());

        it = JSobject.find("TriggerDeadtime_us");
        if(it != JSobject.end())
            setTriggerDowntime(it.value().toInt());

        it = JSobject.find("nTriggers");
        if(it != JSobject.end())
            setNTriggers(it.value().toInt());

        it = JSobject.find("ColourMode");
        if(it != JSobject.end())
            setColourMode(it.value().toBool());

        it = JSobject.find("DecodeFrames");
        if(it != JSobject.end())
            setDecodeFrames(it.value().toBool());
    }

    itParent = JSobjectParent.find("StepperConfig");
    if ( itParent != JSobjectParent.end() ) {

        QJsonObject JSobject = itParent.value().toObject();

        it = JSobject.find("Acceleration");
        double t1 = it.value().toDouble();
        if(it != JSobject.end())
            setStepperConfigAcceleration(it.value().toDouble());

        it = JSobject.find("Speed");
        if(it != JSobject.end())
            setStepperConfigSpeed(it.value().toDouble());

        it = JSobject.find("UseCalib");
        if(it != JSobject.end())
            setStepperConfigUseCalib(it.value().toBool());

        it = JSobject.find("CalibPos0");
        if(it != JSobject.end())
            setStepperConfigCalibPos0(it.value().toDouble());

        it = JSobject.find("CalibAngle0");
        if(it != JSobject.end())
            setStepperConfigCalibAngle0(it.value().toDouble());

        it = JSobject.find("CalibPos1");
        if(it != JSobject.end())
            setStepperConfigCalibPos1(it.value().toDouble());

        it = JSobject.find("CalibAngle1");
        if(it != JSobject.end())
            setStepperConfigCalibAngle1(it.value().toDouble());

    }

    if(includeDacs){
        for(int i = 0 ; i < MPX3RX_DAC_COUNT; i++)
            _dacVals[i].clear();
        QJsonArray dacsArray;
        itParent = JSobjectParent.find("DACs");
        if(itParent != JSobjectParent.end()){
            dacsArray = itParent.value().toArray();
            foreach (const QJsonValue & value, dacsArray) {
                QJsonObject obj = value.toObject();
                for(int i = 0 ; i < MPX3RX_DAC_COUNT; i++) {
                    _dacVals[i].push_back( obj[MPX3RX_DAC_TABLE[i].name].toInt() );
                    //cout << obj[MPX3RX_DAC_TABLE[i].name].toInt() << endl;
                }
            }
        }
    }

    return true;
}

bool Mpx3Config::toJsonFile(QString filename, bool includeDacs){
    QFile loadFile(filename);
    if(!loadFile.open(QIODevice::WriteOnly)){
        printf("Couldn't open configuration file %s\n", filename.toStdString().c_str());
        return false;
    }
    QJsonObject JSobjectParent, objIp, objDetector, objStepper;
    QJsonArray objDacsArray;
    objIp.insert("SpidrControllerIp", SpidrAddress.toString());
    objIp.insert("SpidrControllerPort", this->port);

    objDetector.insert("OperationMode", this->OperationMode);
    objDetector.insert("PixelDepth", this->PixelDepth);
    objDetector.insert("Polarity", this->Polarity);
    objDetector.insert("CsmSpm", this->CsmSpm);
    objDetector.insert("GainMode", this->GainMode);
    objDetector.insert("MaxPacketSize", this->MaxPacketSize);
    objDetector.insert("TriggerMode", this->TriggerMode);
    objDetector.insert("TriggerLength_us", this->TriggerLength_us);
    objDetector.insert("TriggerDeadtime_us", this->TriggerDowntime_us);
    objDetector.insert("nTriggers", this->nTriggers);
    objDetector.insert("ColourMode", this->colourMode);
    objDetector.insert("DecodeFrames", this->decodeFrames);

    objStepper.insert("Acceleration", this->stepperAcceleration);
    objStepper.insert("Speed", this->stepperSpeed);
    objStepper.insert("UseCalib", this->stepperUseCalib);
    objStepper.insert("CalibPos0", this->stepperCalibPos0);
    objStepper.insert("CalibAngle0", this->stepperCalibAngle0);
    objStepper.insert("CalibPos1", this->stepperCalibPos1);
    objStepper.insert("CalibAngle1", this->stepperCalibAngle1);

    JSobjectParent.insert("IPConfig", objIp);
    JSobjectParent.insert("DetectorConfig", objDetector);
    JSobjectParent.insert("StepperConfig", objStepper);

    if(includeDacs){
        for(int j = 0; j < this->getDacCount(); j++){
            QJsonObject obj;
            for(int i = 0 ; i < MPX3RX_DAC_COUNT; i++)
                obj.insert(MPX3RX_DAC_TABLE[i].name, _dacVals[i][j]);
            objDacsArray.insert(j, obj);
        }
        JSobjectParent.insert("DACs", objDacsArray);
    }
    QJsonDocument doc;
    doc.setObject(JSobjectParent);
    loadFile.write(doc.toJson());

    return true;
}

void  Mpx3Config::setStepperConfigCalib(QStandardItem * item) {

    int row = item->row();
    int col = item->column();

    // Check value integrity. Needs to convert to a double.
    QVariant val = item->data(Qt::DisplayRole);
    double dval = 0.0;
    QString posS;
    if ( val.canConvert<QString>() ) {

        posS = val.toString();
        cout << posS.toStdString() << endl;
        bool valok = false;
        double dval = posS.toDouble( &valok );
        cout << dval << endl;

        if( ! valok ) return;

    }

    // If the value is ok check where it goes

    if        ( row == 0 && col == 0 ) {
        setStepperConfigCalibPos0( dval );
    } else if ( row == 0 && col == 1 ) {
        setStepperConfigCalibAngle0( dval );
    } else if ( row == 1 && col == 0 ) {
        setStepperConfigCalibPos1( dval );
    } else if ( row == 1 && col == 1 ) {
        setStepperConfigCalibAngle1( dval );
    }

}


