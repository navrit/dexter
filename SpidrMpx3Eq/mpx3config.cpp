#include "mpx3config.h"
#include <iterator>
#include <iostream>

#include <QFile>

#include "SpidrController.h"
#include "SpidrDaq.h"


using namespace std;


Mpx3Config::Mpx3Config()
{
  // number of devices connected
  _devicePresenceLayout.clear();
  _nDevicesPresent = 0;
  _trigPeriod_ms = 0;
}

void Mpx3Config::SendConfiguration(){

	// Configure the chips
	int nDevSupported = getNDevicesSupported();
	for(int i = 0 ; i < nDevSupported ; i++) {
		if ( detectorResponds( i ) ) {
			Configuration( false, i );
		}
	}

}

void Mpx3Config::Configuration(bool reset, int deviceIndex) {

	cout << "[INFO] Configuring chip " << deviceIndex;

	SpidrController * spidrcontrol = _mpx3gui->GetSpidrController();
	SpidrDaq * spidrdaq = _mpx3gui->GetSpidrDaq();

	// Reset pixel configuration
	if ( reset ) spidrcontrol->resetPixelConfig();

	// All adjustment bits to zero
	//SetAllAdjustmentBits(0x0, 0x0);

	// OMR
	//spidrcontrol->setPolarity( true );		// Holes collection
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
	spidrcontrol->setPixelDepth( deviceIndex, getPixelDepth() );
	spidrdaq->setPixelDepth( getPixelDepth() );
	spidrcontrol->setMaxPacketSize( getMaxPacketSize() );

	// Write OMR ... i shouldn't call this here
	//_spidrcontrol->writeOmr( 0 );

	// Trigger config
	int trig_mode      = getTriggerMode();     // Auto-trigger mode = 4
	int trig_length_us = getTriggerLength();  // This time shouldn't be longer than the period defined by trig_freq_hz

	//int trig_freq_mhz   = (int) 1000 * ( 1. / (2.*((double)trig_length_us/1000000)) );   // Make the period double the trig_len
	int trig_freq_mhz   = (int) 1000 * ( 1. / (1.1*((double)trig_length_us/1000000)) );   // Make the period double the trig_len
	cout << " | configured freq is " << trig_freq_mhz << "mHz";

	// Get the trigger period for information.  This is NOT the trigger length !
	_trigPeriod_ms = (int) (1E6 * (1./(double)trig_freq_mhz));
	int nr_of_triggers = getNTriggers();    // This is the number of shutter open i get

	spidrcontrol->setShutterTriggerConfig( trig_mode, trig_length_us,
			trig_freq_mhz, nr_of_triggers );

	cout << endl;
}

/*
void QCstmGLVisualization::Configuration(bool reset, int deviceIndex) {//TODO: should be part of parent?

	SpidrController * spidrcontrol = _mpx3gui->GetSpidrController();
	SpidrDaq * spidrdaq = _mpx3gui->GetSpidrDaq();

	int nTriggers = _mpx3gui->getConfig()->getNTriggers();

	// Reset pixel configuration
	if ( reset ) spidrcontrol->resetPixelConfig();

	// All adjustment bits to zero
	//SetAllAdjustmentBits(0x0, 0x0);

	// OMR
	//spidrcontrol->setPolarity( true );		// Holes collection
	//_spidrcontrol->setDiscCsmSpm( 0 );		// DiscL used
	//_spidrcontrol->setInternalTestPulse( true ); // Internal tests pulse

	spidrcontrol->setColourMode( deviceIndex, _mpx3gui->getConfig()->getColourMode() ); // false 	// Fine Pitch
	spidrcontrol->setCsmSpm( deviceIndex, _mpx3gui->getConfig()->getCsmSpm() ); // 0 );				// Single Pixel mode

	// Particular for Equalization
	//spidrcontrol->setEqThreshH( deviceIndex, true );
	//spidrcontrol->setDiscCsmSpm( deviceIndex, 0 );		// In Eq mode using 0: Selects DiscL, 1: Selects DiscH
	//_spidrcontrol->setGainMode( 1 );

	// Gain ?!
	// 00: SHGM  0
	// 10: HGM   2
	// 01: LGM   1
	// 11: SLGM  3
	spidrcontrol->setGainMode( deviceIndex, _mpx3gui->getConfig()->getGainMode() ); // 2 );

	// Other OMR
	spidrdaq->setDecodeFrames(  _mpx3gui->getConfig()->getDecodeFrames() ); //  true );
	spidrcontrol->setPixelDepth( deviceIndex,  _mpx3gui->getConfig()->getPixelDepth() );
	spidrdaq->setPixelDepth( _mpx3gui->getConfig()->getPixelDepth() );
	spidrcontrol->setMaxPacketSize( _mpx3gui->getConfig()->getMaxPacketSize() );

	// Write OMR ... i shouldn't call this here
	//_spidrcontrol->writeOmr( 0 );

	// Trigger config
	int trig_mode      = _mpx3gui->getConfig()->getTriggerMode();     // Auto-trigger mode = 4
	int trig_length_us = _mpx3gui->getConfig()->getTriggerLength();  // This time shouldn't be longer than the period defined by trig_freq_hz
	//int trig_freq_hz   = (int) ( 1. / (2.*((double)trig_length_us/1000000.)) );   // Make the period double the trig_len
	int trig_freq_hz   = (int) ( 1. / (((double)trig_length_us/1000000.)) );   // Make the period double the trig_len


	cout << "[INFO] Configured freq is " << trig_freq_hz << "Hz" << endl;
	int nr_of_triggers = _mpx3gui->getConfig()->getNTriggers();    // This is the number of shutter open i get
	//int trig_pulse_count;
	spidrcontrol->setShutterTriggerConfig( trig_mode, trig_length_us,
			trig_freq_hz, nr_of_triggers );

}
*/

SpidrController* Mpx3Config::establishConnection(){

  // number of devices connected
  _devicePresenceLayout.clear();
  _nDevicesPresent = 0;
  _activeChips.clear();
  quint32 ipaddr =  SpidrAddress.toIPv4Address();
  delete controller;
  controller = new SpidrController(((ipaddr>>24) & 0xFF), ((ipaddr>>16) & 0xFF), ((ipaddr>>8) & 0xFF), ((ipaddr>>0) & 0xFF), port);
  connected = controller->isConnected();
  // number of device that the system can support
  controller->getDeviceCount(&_nDevicesSupported);
  cout << "[INFO] Number of devices supported: " << _nDevicesSupported << endl;

  // Response
  _responseChips = QVector<detector_response>( _nDevicesSupported );

  // FIXME
  // For the moment assume matrixes of 256*256
  for(int i = 0 ; i < _nDevicesSupported ; i++) {
      int id = 0;
      controller->getDeviceId(i, &id);

      cout << "--- Device [" << i << "] ------------------ " << endl;

      if ( id != 0 ) {

          cout << "    id : " << id << " | ";
          _devicePresenceLayout.push_back( QPoint(__default_matrixSizePerChip_X, __default_matrixSizePerChip_Y) );
          _nDevicesPresent++;
          // If connected check response
          checkChipResponse( i, __CONTROLLER_OK );
          if(detectorResponds(i))
            _activeChips.push_back(i);
        } else {
          cout << "     	NOT RESPONDING !";
          _devicePresenceLayout.push_back( QPoint(0, 0) );
          // If not connected tag it immediately
          _responseChips[i] = __NOT_RESPONDING;
        }

      cout << endl;
    }

  return controller;
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

void Mpx3Config::checkChipResponse(int devIndx, detector_response dr) {

  if ( dr == __CONTROLLER_OK ) { // Check if the detector responds ok to the Controller

      // For instance try to read a DAC
      int dac_val = 0;

      if ( ! controller->setDac( devIndx, MPX3RX_DAC_TABLE[0].code, dac_val ) ) {
          cout << "chip response failed : "  << controller->errorString();
          _responseChips[devIndx] = __NOT_RESPONDING;
        } else {
          cout << "Response OK";

          _responseChips[devIndx] = __CONTROLLER_OK;
        }

    }

}

bool Mpx3Config::detectorResponds(int devIndx) {

	if( devIndx >= _nDevicesSupported || devIndx < 0) {
		cout << "[ERR ] device index out of range: " << devIndx << endl;
		return false;
	}

	if ( _responseChips[devIndx] > __NOT_RESPONDING ) return true;

  return false;
}

bool Mpx3Config::fromJsonFile(QString filename, bool includeDacs){

  cout << "[INFO] reading the configuration from the Json file: " << filename.toStdString() << endl;

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
  QJsonObject JSobjectParent, objIp, objDetector;
  QJsonArray objDacsArray;
  objIp.insert("SpidrControllerIp", SpidrAddress.toString());
  objIp.insert("SpidrControllerPort", this->port);

  objDetector.insert("OperationMode", this->OperationMode);
  objDetector.insert("PixelDepth", this->PixelDepth);
  objDetector.insert("CsmSpm", this->CsmSpm);
  objDetector.insert("GainMode", this->GainMode);
  objDetector.insert("MaxPacketSize", this->MaxPacketSize);
  objDetector.insert("TriggerMode", this->TriggerMode);
  objDetector.insert("TriggerLength_us", this->TriggerLength_us);
  objDetector.insert("nTriggers", this->nTriggers);
  objDetector.insert("ColourMode", this->colourMode);
  objDetector.insert("DecodeFrames", this->decodeFrames);

  JSobjectParent.insert("IPConfig", objIp);
  JSobjectParent.insert("DetectorConfig", objDetector);
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
