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
#define DEFAULT_IP  "192.168.1.10"
#define __default_matrixSizePerChip_X 	256
#define __default_matrixSizePerChip_Y 	256

class Mpx3Config: public QObject {

	Q_OBJECT
	//Spidr stuff
	SpidrController *controller = nullptr;
	bool connected = false;
	QHostAddress SpidrAddress;
	uint16_t port;
	//Operation stuff
	bool colourMode = false, decodeFrames = false;
	int OperationMode = -1, PixelDepth = -1, CsmSpm =-1, GainMode =-1, MaxPacketSize =-1, TriggerMode =-1, TriggerLength_us = -1, nTriggers = -1;
	QVector<int> _dacVals[MPX3RX_DAC_COUNT];

	typedef enum {
		__NOT_RESPONDING = 0,
		__CONTROLLER_OK,
		__DAC_OK
	} detector_response;

	QVector<detector_response> _responseChips;
	QVector<int> _activeChips;

public:
	Mpx3Config();
	//void setIpAddress(QString ip, uint16_t port);
	bool isConnected(){return connected;}
	bool fromJsonFile(QString filename, bool includeDacs = true);
	bool toJsonFile(QString filename, bool includeDacs = true);
	QString getIpAddress(){return SpidrAddress.toString();}
	SpidrController* getController(){return controller;}
	SpidrController* establishConnection();
	int getDacCount(){return _dacVals[0].length(); }
	int getDACValue(int chip, int dacIndex) { return _dacVals[dacIndex][chip]; }
	QVector<QPoint> getDevicePresenceLayout(){ return _devicePresenceLayout; };
	int getNDevicesPresent() { return _nDevicesPresent; }
	int getNDevicesSupported() { return _nDevicesSupported; };
	int getNActiveDevices(){return _activeChips.size();}
	QVector<int>  getActiveDevices(){return _activeChips;}
	int getIDIndex(int id){return _activeChips.indexOf(id);}

	void checkChipResponse(int devIndx, detector_response dr);
	bool detectorResponds(int devIndx);

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

private:
	// Layout of the matrix. Each QPoint is a chip connected with X,Y sizes.
	QVector<QPoint> _devicePresenceLayout;
	int _nDevicesPresent;
	int _nDevicesSupported;

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
		if(SpidrAddress.toString().length() == 0)
			SpidrAddress.setAddress(DEFAULT_IP);
		emit IpAdressChanged(this->getIpAddress());
		establishConnection();
	}
}
void setPort(int newVal){
	if(newVal != port){
		port = newVal;
		emit portChanged(newVal);
		//establishConnection();
	}
}

void setColourMode(bool mode){if(mode != colourMode){colourMode =mode; emit colourModeChanged(mode);updateColourMode();}}
void updateColourMode(){controller->setColourMode(0, colourMode);}

void setDecodeFrames(bool decode){if(decode != decodeFrames){decodeFrames = decode; emit decodeFramesChanged(decode);updateDecodeFrames();}}
void updateDecodeFrames(){}

void setOperationMode(int newVal){if(newVal != OperationMode){OperationMode = newVal; emit operationModeChanged(newVal);updateOperationMode();}}
void updateOperationMode(){}

void setPixelDepth(int newVal){if(newVal != PixelDepth){PixelDepth = newVal; emit pixelDepthChanged(newVal);updatePixelDepth();}}
void updatePixelDepth(){controller->setPixelDepth(0, PixelDepth);}

void setCsmSpm(int newVal){if(newVal != CsmSpm){CsmSpm = newVal; emit csmSpmChanged(newVal);updateCsmSpm();}}
void updateCsmSpm(){controller->setCsmSpm(0, CsmSpm);}

void setGainMode(int newVal){if(newVal != GainMode){GainMode = newVal; emit gainModeChanged(newVal);updateGainMode();}}
void updateGainMode(){controller->setGainMode(0, GainMode);}

void setMaxPacketSize(int newVal){if(newVal != MaxPacketSize){MaxPacketSize = newVal; emit MaxPacketSizeChanged(newVal);updateMaxPacketSize();}}
void updateMaxPacketSize(){controller->setMaxPacketSize(MaxPacketSize);}

void setTriggerMode(int newVal){if(newVal != TriggerMode){TriggerMode = newVal; emit TriggerModeChanged(newVal);updateTriggerMode();}}
void updateTriggerMode(){}

void setTriggerLength(int newVal){if(newVal != TriggerLength_us){TriggerLength_us = newVal; emit TriggerLengthChanged(newVal);updateTriggerLength();}}
void updateTriggerLength(){}

void setNTriggers(int newVal){if(newVal != nTriggers){nTriggers = newVal; emit nTriggersChanged(newVal);updateNTriggers();}}
void updateNTriggers(){}
};

#endif // MPX3CONFIG_H
