#ifndef MPX3CONFIG_H
#define MPX3CONFIG_H
#include "mpx3defs.h"
#include "qcstmdacs.h"

#include <QObject>
#include <QHostAddress>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

#include "SpidrController.h"

#include <stdint.h>

class Mpx3Config: public QObject {
  Q_OBJECT
  //Spidr stuff
  SpidrController *controller = nullptr;
  bool isConnected = false;
  QHostAddress SpidrAddress;
  uint16_t port;
  //Operation stuff
  bool colourMode = false, decodeFrames = false;
  int OperationMode = -1, PixelDepth = -1, CsmSpm =-1, GainMode =-1, MaxPacketSize =-1, TriggerMode =-1, TriggerLength_us = -1, nTriggers = -1;

  QVector<int> _dacVals[MPX3RX_DAC_COUNT];

public:
  Mpx3Config();
  //void setIpAddress(QString ip, uint16_t port);
  bool fromJsonFile(QString filename, bool includeDacs = true);
  bool toJsonFile(QString filename, bool includeDacs = true);
  QString getIpAddress(){return SpidrAddress.toString();}
  SpidrController* getController(){return controller;}
  SpidrController* establishConnection();
  int getDacCount(){return _dacVals[0].length(); }
  int getDACValue(int chip, int dacIndex) { return _dacVals[dacIndex][chip]; }

  quint32 getIpAddressInt(){return SpidrAddress.toIPv4Address();}
  uint16_t getIpAddressPort(){return port;}
  bool getColourMode(){return colourMode;}
  bool getDecodeFrames(){return decodeFrames;}
  int getOperationMode(){return OperationMode;}
  int getPixelDepth(){return PixelDepth;}
  int getCsmSpm(){return CsmSpm;  }
  int getGainMode(){return GainMode;}
  int getMaxPacketSize(){return MaxPacketSize;}
  int getTriggerMode(){return TriggerMode;}
  int getTriggerLength(){return TriggerLength_us;}
  int getNTriggers(){return nTriggers;}
signals:
  void IpAdressChanged(QString);
  void portChanged(int);
  void colourModeChanged(bool);
  void decodeFramesChanged(bool);
  void operationModeChanged(int);
  void pixelDepthChanged(int);
  void csmSpmChanged(int);
  void gainModeChanged(int);
  void MaxPacketSizeChanged(int);
  void TriggerModeChanged(int);
  void TriggerLengthChanged(int);
  void nTriggersChanged(int);
public slots:
  void setIpAddress(QString ip){
    if(ip != this->getIpAddress()){
        SpidrAddress.setAddress(ip);
        emit IpAdressChanged(this->getIpAddress());
      }
  }
  void setPort(int newVal){if(newVal != port){port = newVal; emit portChanged(newVal);}}
  //void updateAddress();

  void setColourMode(bool mode){if(mode != colourMode){colourMode =mode; emit colourModeChanged(mode);}}
  //void updateColourMode();

  void setDecodeFrames(bool decode){if(decode != decodeFrames){decodeFrames = decode; emit decodeFramesChanged(decode);}}
  //void updateDecodeFrames();

  void setOperationMode(int newVal){if(newVal != OperationMode){OperationMode = newVal; emit operationModeChanged(newVal);}}
  //void updateOperationMode();

  void setPixelDepth(int newVal){if(newVal != PixelDepth){PixelDepth = newVal; emit pixelDepthChanged(newVal);}}
  //void updatePixelDepth();

  void setCsmSpm(int newVal){if(newVal != CsmSpm){CsmSpm = newVal; emit csmSpmChanged(newVal);}}
  //void updateCsmSpm();

  void setGainMode(int newVal){if(newVal != GainMode){GainMode = newVal; emit gainModeChanged(newVal);}}
  //void updateGainMode();

  void setMaxPacketSize(int newVal){if(newVal != MaxPacketSize){MaxPacketSize = newVal; emit MaxPacketSizeChanged(newVal);}}
  //void updateMaxPacketSize();

  void setTriggerMode(int newVal){if(newVal != TriggerMode){TriggerMode = newVal; emit TriggerModeChanged(newVal);}}
  //void updateTriggerMode();

  void setTriggerLength(int newVal){if(newVal != TriggerLength_us){TriggerLength_us = newVal; emit TriggerLengthChanged(newVal);}}
  //void updateTriggerLength();

  void setNTriggers(int newVal){if(newVal != nTriggers){nTriggers = newVal; emit nTriggersChanged(newVal);}}
  //void updateNTriggers();
};

#endif // MPX3CONFIG_H
