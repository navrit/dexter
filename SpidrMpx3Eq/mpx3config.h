#ifndef MPX3CONFIG_H
#define MPX3CONFIG_H
#include "mpx3defs.h"
#include "qcstmdacs.h"
//#include "mpx3gui.h"

#include <QComboBox>
#include <QObject>
#include <QHostAddress>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

#include "SpidrController.h"

class Mpx3GUI;

#include <stdint.h>
#define __default_IP  "192.168.1.10"
#define __default_port 50000
#define __default_matrixSizePerChip_X 	256
#define __default_matrixSizePerChip_Y 	256

#define __efuse_Nnibbles 8
//#define

//#define __operationMode_SequentialRW  0
//#define __operationMode_ContinuousRW  1

class Mpx3Config: public QObject {

	Q_OBJECT
	//Spidr stuff
    SpidrController * controller = nullptr;
	bool connected = false;
	QHostAddress SpidrAddress;
	uint16_t port;
	int _trigPeriod_ms;
	//Operation stuff
	bool colourMode = false, decodeFrames = false, readBothCounters = false, Polarity = true;
	int OperationMode = -1, PixelDepth = -1, CsmSpm =-1, GainMode =-1, MaxPacketSize =-1, TriggerMode =-1, TriggerLength_us = -1, TriggerDowntime_us = -1, nTriggers = -1;
	QVector<int> _dacVals[MPX3RX_DAC_COUNT];
	// Stepper
	bool stepperUseCalib = false;
	double stepperAcceleration = -1., stepperSpeed = -1., stepperCalibPos0 = -1., stepperCalibAngle0 = -1., stepperCalibPos1 = -1., stepperCalibAngle1 = -1.;

    // Bias
    double biasVolt = 0.;

    // Some constants in the configuration (MPX3 manual pag. 18)
    unsigned int * __pixelDepthMap;// = { 1 , 6 , 12 , 24 };
    const unsigned int __pixelDepth12BitsIndex = 2;

    typedef enum {
        __operationMode_SequentialRW = 0,
        __operationMode_ContinuousRW,
        __operationMode_NumberOf
    } operation_mode;

	typedef enum {
		__NOT_RESPONDING = 0,
		__CONTROLLER_OK,
		__DAC_OK
	} detector_response;

	QVector<detector_response> _responseChips;
	QVector<int> _activeChips;

public:

	Mpx3Config();
	void SetMpx3GUI(Mpx3GUI * p) { _mpx3gui = p; };
	//void setIpAddress(QString ip, uint16_t port);
	bool isConnected(){return connected;}
	bool fromJsonFile(QString filename, bool includeDacs = true);
	bool toJsonFile(QString filename, bool includeDacs = true);
	QString getIpAddress(){return SpidrAddress.toString();}
	SpidrController* getController(){return controller;}
	SpidrController* establishConnection();
    void destroyController();
	int getDacCount(){return _dacVals[0].length(); }
	int getDACValue(int chip, int dacIndex) { return _dacVals[dacIndex][chip]; }
	void setDACValue(int chip, int dacIndex, int val) { _dacVals[dacIndex][chip] = val; }
	QVector<QPoint> getDevicePresenceLayout(){ return _devicePresenceLayout; };
	int getNDevicesPresent() { return _nDevicesPresent; }
	int getDataBufferId(int devIndx);
	int getNDevicesSupported() { return _nDevicesSupported; };
	int getNActiveDevices(){return _activeChips.size();}
	QVector<int>  getActiveDevices(){return _activeChips;}
    QString getDeviceWaferId(int id){return _deviceWaferIdMap.at(id); }
    int getIndexFromID(int id){return _activeChips.indexOf(id);}
	int getTriggerPeriodMS(){return _trigPeriod_ms;}
    unsigned int getPixelDepthFromIndex(int indx);
    unsigned int getPixelDepth12BitsIndex() { return __pixelDepth12BitsIndex; }
    double getBiasVoltage() { return biasVolt; }

	void checkChipResponse(int devIndx, detector_response dr);
	bool detectorResponds(int devIndx);

	typedef struct {
		int nTriggers;
		bool equalizationBit;
		int DiscCsmSpm;
    } extra_config_parameters;

    typedef struct {
        int V_TP_Ref;
        int V_TP_RefA;
        int V_TP_RefB;
    } testpulses_config_parameters;


	void SendConfiguration();
	void Configuration(bool reset, int deviceIndex);
    void Configuration(bool reset, int deviceIndex, extra_config_parameters);

	quint32 getIpAddressInt(){return SpidrAddress.toIPv4Address();}
	uint16_t getIpAddressPort(){return port;}
	bool getColourMode(){return colourMode;}
	bool getDecodeFrames(){return decodeFrames;}
	bool getReadBothCounters() {return readBothCounters;}
	int getOperationMode(){return OperationMode;}
	int getPixelDepth(){return PixelDepth;}
	bool getPolarity(){return Polarity;}
	int getCsmSpm(){return CsmSpm;  }
	int getGainMode(){return GainMode;}
	int getMaxPacketSize(){return MaxPacketSize;}
	int getTriggerMode(){return TriggerMode;}

	int getTriggerLength(){return TriggerLength_us;}
	int getTriggerLength_ms(){return (TriggerLength_us/1000);}

	int getTriggerDowntime(){return TriggerDowntime_us;}
	int getTriggerDowntime_ms(){return TriggerDowntime_us/1000;}

    int getNTriggers(){ return nTriggers; }

    bool getStepperUseCalib() { return stepperUseCalib; }
	double getStepperAcceleration() { return stepperAcceleration; }
	double getStepperSpeed() { return stepperSpeed; }
	double getStepperCalibPos0() { return stepperCalibPos0; }
	double getStepperCalibAngle0() { return stepperCalibAngle0; }
	double getStepperCalibPos1() { return stepperCalibPos1; }
	double getStepperCalibAngle1() { return stepperCalibAngle1; }

private:

	Mpx3GUI * _mpx3gui;
	// Layout of the matrix. Each QPoint is a chip connected with X,Y sizes.
	QVector<QPoint> _devicePresenceLayout;
    QVector<QString> _deviceWaferIdMap;
	int _nDevicesPresent;
	int _nDevicesSupported;

	signals:
	void IpAdressChanged(QString);
	void portChanged(int);
	void colourModeChanged(bool);
	void readBothCountersChanged(bool);
	void decodeFramesChanged(bool);
	void operationModeChanged(int);
	void pixelDepthChanged(int);
	void polarityChanged(int);
	void csmSpmChanged(int);
	void gainModeChanged(int);
	void MaxPacketSizeChanged(int);
	void TriggerModeChanged(int);
	void TriggerLengthChanged(int);
	void TriggerDowntimeChanged(int);
	void nTriggersChanged(int);
	// stepper
	void UseCalibChanged(bool);
	void AccelerationChanged(double);
	void SpeedChanged(double);
	void CalibPos0Changed(double);
	void CalibAngle0Changed(double);
	void CalibPos1Changed(double);
	void CalibAngle1Changed(double);
    void BiasVoltageChanged(double);

public slots:

void setBiasVoltage(double volt) {
        if ( volt != this->getBiasVoltage() ) {
            biasVolt = volt;
            BiasVoltageChanged(volt);
        }
        SendConfiguration();
}

void setIpAddress(QString ip) {

    /*
    // The ip and port will come in the string.
    // 192.168.10:50000
    QStringList list = ipn.split(':', QString::SkipEmptyParts);
    // expect the ip address in the first part
    QString ip = list.at( 0 );
    QString portS = list.at( 1 );

    qDebug() << ip << " : " << port;
*/
    // IP
    if ( ip != this->getIpAddress() ) {

        SpidrAddress.setAddress(ip);
		if(SpidrAddress.toString().length() == 0)
            SpidrAddress.setAddress( __default_IP );
        emit IpAdressChanged( this->getIpAddress() );
        //establishConnection();
	}
/*
    // Port
    bool ok = true;
    int newVal = portS.toInt( &ok );
    if ( newVal != port && ok ) {
        port = newVal;
        emit portChanged(newVal);
        //establishConnection();
    } else {
        port = __default_port;
        emit portChanged( __default_port );
    }
*/
}

void setPort(int newVal){
	if(newVal != port){
		port = newVal;
		emit portChanged(newVal);
		//establishConnection();
	}
}

void setColourMode(bool mode){
	if(mode != colourMode){
		colourMode =mode; emit colourModeChanged(mode);

		//updateColourMode();
	}
	SendConfiguration();
}
void updateColourMode(){}

void setReadBothCounters(bool rbc) {
	if(rbc != readBothCounters) {
		readBothCounters=rbc; emit readBothCountersChanged(rbc);
	}
	SendConfiguration();
}

void setDecodeFrames(bool decode){
	if(decode != decodeFrames){
		decodeFrames = decode; emit decodeFramesChanged(decode);
		//updateDecodeFrames();
	}
	SendConfiguration();
}
void updateDecodeFrames(){}

void setOperationMode(int newVal){

    // This can only take some values
    if ( newVal > __operationMode_NumberOf || newVal < __operationMode_SequentialRW ) {
        newVal = __operationMode_SequentialRW;
    }

	if(newVal != OperationMode){
		OperationMode = newVal; emit operationModeChanged(newVal);
		//updateOperationMode();
	}
	SendConfiguration();
}
void updateOperationMode(){}


void setPixelDepthByIndex(int indx) {
    setPixelDepth( indx, false );
}
void setPixelDepth(int newVal, bool byValue = true) {

    if ( ! byValue ) {
        // in this case newVal is an index, not the pixel depth value
        newVal = __pixelDepthMap[ newVal ]; // by index
    }

    qDebug() << "pixel depth : " << newVal;

    if ( newVal != PixelDepth ) {
		PixelDepth = newVal; emit pixelDepthChanged(newVal);
		//updatePixelDepth();
	}
	SendConfiguration();

}
void updatePixelDepth(){}

void setCsmSpm(int newVal){
	if(newVal != CsmSpm){
		CsmSpm = newVal; emit csmSpmChanged(newVal);
		//updateCsmSpm();
	}
	SendConfiguration();
}
void updateCsmSpm(){}


void setGainMode(int newVal){
	if(newVal != GainMode){
		GainMode = newVal; emit gainModeChanged(newVal);
		//updateGainMode();
	}
	SendConfiguration();
}
void updateGainMode(){}

void setPolarity(int); // implemented in cpp
void setPolarityByString(QString itemS, int indx = -1); // implemented in cpp
void updatePolarity(){}

void setMaxPacketSize(int newVal){
	if(newVal != MaxPacketSize){
		MaxPacketSize = newVal; emit MaxPacketSizeChanged(newVal);
		//updateMaxPacketSize();
	}
	SendConfiguration();
}
void updateMaxPacketSize(){controller->setMaxPacketSize(MaxPacketSize);}


void setTriggerMode(int newVal){
	if(newVal != TriggerMode){
		TriggerMode = newVal; emit TriggerModeChanged(newVal);
		//updateTriggerMode();
	}
	SendConfiguration();
}
void updateTriggerMode(){}

void setTriggerLength(int newVal){
	if(newVal != TriggerLength_us){
		TriggerLength_us = newVal; emit TriggerLengthChanged(newVal);
		//updateTriggerLength();
	}
	SendConfiguration();
}
void updateTriggerLength(){}

// This is connected to QAbstractSpinBox::editingFinished() which takes no argument.
// Pick the value from the spin-box directly.
void setTriggerDowntime();
void setTriggerDowntime(int);
void updateTriggerDowntime(){}

void setNTriggers(int newVal) {

    if(newVal != nTriggers){
		nTriggers = newVal; emit nTriggersChanged(newVal);
		//updateNTriggers();
	}

	SendConfiguration();

}
void updateNTriggers(){}

////////////////////////////////////////////////////////////
// Stepper
// The messages to the hardware won't be sent from here in
//  the case of the stepper.  It has it's own interface in
//  qctsmconfigmonitoring.cpp
void setStepperConfigUseCalib(bool newVal) {

	if ( newVal != stepperUseCalib ) {
		stepperUseCalib = newVal;
		//emit UseCalibChanged(newVal);
	}

}

void setStepperConfigAcceleration(double newVal) {

	if ( newVal != stepperAcceleration ) {
		stepperAcceleration = newVal;
		//emit AccelerationChanged(newVal);
	}

}

void setStepperConfigSpeed(double newVal) {

	if ( newVal != stepperSpeed ) {
		stepperSpeed = newVal;
		//emit SpeedChanged(newVal);
	}

}

void setStepperConfigCalib(QStandardItem * item);


void setStepperConfigCalibPos0(double newVal) {

	if ( newVal != stepperCalibPos0 ) {
		stepperCalibPos0 = newVal;
		//emit CalibPos0Changed(newVal);
	}

}

void setStepperConfigCalibAngle0(double newVal) {

	if ( newVal != stepperCalibAngle0 ) {
		stepperCalibAngle0 = newVal;
		//emit CalibAngle0Changed(newVal);
	}

}

void setStepperConfigCalibPos1(double newVal) {

	if ( newVal != stepperCalibPos1 ) {
		stepperCalibPos1 = newVal;
		//emit CalibPos1Changed(newVal);
	}

}

void setStepperConfigCalibAngle1(double newVal) {

	if ( newVal != stepperCalibAngle1 ) {
		stepperCalibAngle1 = newVal;
		//emit CalibAngle1Changed(newVal);
	}

}

};


#endif // MPX3CONFIG_H
