#include "mpx3config.h"
#include <iterator>
#include <QFile>

#include <iostream>

using namespace std;


Mpx3Config::Mpx3Config()
{

}

/*void Mpx3Config::setIpAddress(QString ip, uint16_t port){
	SpidrAddress.setAddress(ip);
	this->port = port;
	quint32 ipaddr =  SpidrAddress.toIPv4Address();
	delete controller;
	controller = new SpidrController(((ipaddr>>24) & 0xFF), ((ipaddr>>16) & 0xFF), ((ipaddr>>8) & 0xFF), ((ipaddr>>0) & 0xFF), port);
	isConnected = controller->isConnected();
}*/

SpidrController* Mpx3Config::establishConnection(){
  quint32 ipaddr =  SpidrAddress.toIPv4Address();
  delete controller;
  controller = new SpidrController(((ipaddr>>24) & 0xFF), ((ipaddr>>16) & 0xFF), ((ipaddr>>8) & 0xFF), ((ipaddr>>0) & 0xFF), port);
  isConnected = controller->isConnected();
  return controller;
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
