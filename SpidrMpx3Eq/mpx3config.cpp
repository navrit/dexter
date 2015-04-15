#include "mpx3config.h"
#include <iterator>
#include <QFile>

#include <iostream>

using namespace std;


Mpx3Config::Mpx3Config()
{

}

void Mpx3Config::setIpAddress(QString ip, uint16_t port){
	SpidrAddress.setAddress(ip);
	this->port = port;
	quint32 ipaddr =  SpidrAddress.toIPv4Address();
	delete controller;
	controller = new SpidrController(((ipaddr>>24) & 0xFF), ((ipaddr>>16) & 0xFF), ((ipaddr>>8) & 0xFF), ((ipaddr>>0) & 0xFF), port);
	isConnected = controller->isConnected();
}

bool Mpx3Config::fromJsonFile(QString filename){

	cout << "[INFO] reading the configuration from the Json file: " << filename.toStdString() << endl;

	QFile loadFile(filename);
	if(!loadFile.open(QIODevice::ReadOnly)){
		printf("Couldn't open configuration file %s\n", filename.toStdString().c_str());
		return true;
	}
	QByteArray binaryData = loadFile.readAll();
	QJsonObject JSobjectParent = QJsonDocument::fromJson(binaryData).object();
	QJsonObject::iterator it, itParent;
	itParent = JSobjectParent.find("IPConfig");
	if(itParent != JSobjectParent.end()){
		QJsonObject JSobject = itParent.value().toObject();
		it = JSobject.find("SpidrControllerIp");
		if(it != JSobject.end())
			SpidrAddress.setAddress(it.value().toString());
		it = JSobject.find("SpidrControllerPort");
		if(it != JSobject.end())
			port = it.value().toInt();
		emit IpAdressChanged(this->getIpAddress());
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
		if(it != JSobject.end()){
			printf("Read %d for decodeFrames!\n", it.value().toBool());
			setDecodeFrames(it.value().toBool());
		}
	}
	QJsonArray dacsArray;
	itParent = JSobjectParent.find("DACs");
	if(itParent != JSobjectParent.end()){
		QJsonObject JSobject = itParent.value().toObject();
		foreach (const QJsonValue & value, dacsArray) {
			QJsonObject obj = value.toObject();
			for(int i = 0 ; i < MPX3RX_DAC_COUNT; i++) {
				_dacVals[i].push_back( JSobject[MPX3RX_DAC_TABLE[i].name].toInt() );
			}
		}
	}


	return true;
}
