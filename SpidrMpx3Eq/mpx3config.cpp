#include "mpx3config.h"
#include <iterator>
#include <QFile>

Mpx3Config::Mpx3Config()
{

}

void Mpx3Config::fromJsonFile(QString filename){
  QFile loadFile(filename);
  if(!loadFile.open(QIODevice::ReadOnly)){
       printf("Couldn't open configuration file %s\n", filename.toStdString().c_str());
       return;
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
  }
  itParent = JSobjectParent.find("DetectorConfig");
  if(itParent != JSobjectParent.end()){
      QJsonObject JSobject = itParent.value().toObject();
      it = JSobject.find("OperationMode");
      if(it != JSobject.end())
        OperationMode = it.value().toInt();
      it = JSobject.find("PixelDepth");
      if(it != JSobject.end())
        PixelDepth = it.value().toInt();
      it = JSobject.find("CsmSpm");
      if(it != JSobject.end())
        CsmSpm = it.value().toInt();
      it = JSobject.find("GainMode");
      if(it != JSobject.end())
        GainMode = it.value().toInt();
      it = JSobject.find("MaxPacketSize");
      if(it != JSobject.end())
        MaxPacketSize = it.value().toInt();
      it = JSobject.find("TriggerMode");
      if(it != JSobject.end())
        TriggerMode = it.value().toInt();
      it = JSobject.find("TriggerLength_us");
      if(it != JSobject.end())
        TriggerLength_us = it.value().toInt();
      it = JSobject.find("nTriggers");
      if(it != JSobject.end())
        nTriggers = it.value().toInt();
      it = JSobject.find("colourMode");
      if(it != JSobject.end())
        colourMode = it.value().toBool();
      it = JSobject.find("decodeFrames");
      if(it != JSobject.end())
        decodeFrames = it.value().toBool();
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


}
