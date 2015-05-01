#include "mpx3config.h"
#include <iterator>
#include <QFile>

#include <iostream>

using namespace std;


Mpx3Config::Mpx3Config()
{
	// number of devices connected
	_devicePresenceLayout.clear();
	_nDevicesPresent = 0;

}

SpidrController* Mpx3Config::establishConnection(){

	// number of devices connected
	_devicePresenceLayout.clear();
	_nDevicesPresent = 0;

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
	if(includeDacs){
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
}
