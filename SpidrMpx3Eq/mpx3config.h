#ifndef MPX3CONFIG_H
#define MPX3CONFIG_H

#include <iostream>
#include <iterator>
#include <stdint.h>

#include <QComboBox>
#include <QDebug>
#include <QFile>
#include <QHostAddress>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QObject>
#include <QRegExp>
#include <QStandardItem>

#include "EnergyCalibrator.h"
#include "SpidrController.h"
#include "SpidrDaq.h"
#include "mpx3dacsdescr.h"
#include "mpx3defs.h"
#include "qcstmdacs.h"
#include <cmath>
#include <limits>

class Mpx3GUI;

#define __default_IP "192.168.1.10"
#define __default_port 50000
#define __default_matrixSizePerChip_X 256
#define __default_matrixSizePerChip_Y 256
#define __efuse_Nnibbles 8


class Mpx3Config : public QObject {

  Q_OBJECT

public:
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

  typedef enum {
    __nTriggers = 0,
    __contRWFreq,
    __triggerLength,
    __triggerDowntime,
    __operationMode,
    __polarity,
    __gainMode,
    __pixelDepth,
    __triggerMode,
    __biasVoltage,
    __CsmSpm,
    __readBothCounters,
    __colourMode,
    __decodeFrames,
    __logLevel,

    __maxPacketSize,

    __mpx3Clock,

    __ALL
  } config_items;

private:
  // Spidr stuff
  SpidrController *_controller = nullptr;
  bool connected = false;
  QHostAddress SpidrAddress;
  QString Zmq_Pub_address; // Not using QHostAddress because I need more
                           // flexibility, eg. using *, tcp://, inproc://,
                           // ipc:// etc.
  QString Zmq_Sub_address;
  uint16_t port;
  // Operation stuff
  bool colourMode = false, decodeFrames = false, readBothCounters = false,
       Polarity = true;
  int OperationMode = -1, PixelDepth = -1, CsmSpm = -1, GainMode = -1,
      MaxPacketSize = -1, ContRWFreq = -1, TriggerMode = -1, nTriggers = -1;
  int LogLevel = -1;
  uint64_t TriggerLength_us_64 = 0, TriggerDowntime_us_64 = 0;
  // The following are static characteristics read from the SPIDR, not
  // configurable.
  int SystemClock = -1;
  QVector<int> _dacVals[MPX3RX_DAC_COUNT] = {{-1}};
  // Stepper
  bool stepperUseCalib = false;
  double stepperAcceleration = -1., stepperSpeed = -1., stepperCalibPos0 = -1.,
         stepperCalibAngle0 = -1., stepperCalibPos1 = -1.,
         stepperCalibAngle1 = -1.;

  // Bias
  double biasVolt = 0.;

  QVector<detector_response> _responseChips;
  QVector<int> _activeChips;
  QVector<uint8_t> _chipIDELAYS = {15, 15, 15, 10};
  std::array<double, __max_number_of_thresholds> targetEnergies = {{-1.0}};

public:
  Mpx3Config();
  void SetMpx3GUI(Mpx3GUI *p) { _mpx3gui = p; }
  bool isConnected() { return connected; }
  bool fromJsonFile(QString filename, bool includeDacs = true);
  bool toJsonFile(QString filename, bool includeDacs = true);
  QString getIpAddress() { return SpidrAddress.toString(); }
  QString getIpZmqPubAddress() { return Zmq_Pub_address; }
  QString getIpZmqSubAddress() { return Zmq_Sub_address; }
  QString getIpAddressPortString();
  QString getIpZmqPubAddressPortString();
  QString getIpZmqSubAddressPortString();
  SpidrController *getController() { return _controller; }
  SpidrController *establishConnection();
  void closeConnection();
  void destroyController();

  int getDacCount();
  int getDACValue(uint chip, int dacIndex);
  void setDACValue(uint chip, int dacIndex, int dac_value);

  QVector<QPoint> getDevicePresenceLayout() { return _devicePresenceLayout; }
  int getNDevicesPresent() { return _nDevicesPresent; }
  int getDataBufferId(int devIndx);
  int getNDevicesSupported() { return _nDevicesSupported; }
  uint getNActiveDevices() { return uint(_activeChips.size()); }
  QVector<int> getActiveDevices() { return _activeChips; }
  QString getDeviceWaferId(int id) { return _deviceWaferIdMap.at(id); }
  int getIndexFromID(int id) { return _activeChips.indexOf(id); }
  int getSystemClock() { return SystemClock; }
  double getBiasVoltage() { return biasVolt; }
  double getTargetEnergies(int threshold);

  void checkChipResponse(int devIndx, detector_response dr);
  bool detectorResponds(int devIndx);

  typedef struct {
    int nTriggers;
    bool equalizationBit;
    int DiscCsmSpm;
  } extra_config_parameters;

  void SendConfiguration(config_items item = __ALL);
  void PickupStaticConfigurationFigures();
  void Configuration(bool reset, int deviceIndex, config_items item = __ALL);
  void Configuration(bool reset, int deviceIndex, extra_config_parameters,
                     config_items item = __ALL);

  quint32 getIpAddressInt() { return SpidrAddress.toIPv4Address(); }
  uint16_t getIpAddressPort() { return port; }
  bool getColourMode() { return colourMode; }
  bool getDecodeFrames() { return decodeFrames; }
  bool getReadBothCounters() { return readBothCounters; }
  int getOperationMode() { return OperationMode; }
  int getPixelDepth() { return PixelDepth; }
  bool getPolarity() { return Polarity; }
  QString getPolarityString() {
    return QString(Polarity ? "Positive" : "Negative");
  }
  int getCsmSpm() { return CsmSpm; }
  int getGainMode() { return GainMode; }
  QString getGainModeString() {
    QString modes[] = {
        "SHGM", "HGM", "LGM",
        "SLGM"}; // in the right order, read the manual carefully...
    return modes[GainMode];
  }
  int getMaxPacketSize() { return MaxPacketSize; }
  int getTriggerMode() { return TriggerMode; }
  int getLogLevel() { return LogLevel; }

  int getContRWFreq() { return ContRWFreq; }
  uint64_t getTriggerLength_ms_64() { return (TriggerLength_us_64 / 1000); }
  uint64_t getTriggerLength_64() { return (TriggerLength_us_64); }
  uint64_t getTriggerDowntime_ms_64() { return TriggerDowntime_us_64 / 1000; }
  uint64_t getTriggerDowntime_64() { return TriggerDowntime_us_64; }
  uint64_t getTriggerPeriod() {
    return TriggerLength_us_64 + TriggerDowntime_us_64;
  }
  uint64_t getTriggerPeriod_ms() { return getTriggerPeriod() / 1000; }
  int getTriggerFreq_mHz() { return int(1e9 / getTriggerPeriod()); }
  int getNTriggers() { return nTriggers; }

  bool getStepperUseCalib() { return stepperUseCalib; }
  double getStepperAcceleration() { return stepperAcceleration; }
  double getStepperSpeed() { return stepperSpeed; }
  double getStepperCalibPos0() { return stepperCalibPos0; }
  double getStepperCalibAngle0() { return stepperCalibAngle0; }
  double getStepperCalibPos1() { return stepperCalibPos1; }
  double getStepperCalibAngle1() { return stepperCalibAngle1; }

  bool RequiredOnEveryChipConfig(config_items);
  bool RequiredOnGlobalConfig(config_items item);

  QJsonDocument buildConfigJSON(bool includeDacs);
  void onSetInhibitShutterRegisterOffset(int offset);
  bool isInhibitShutterSelected();
  void setHdmiRegisterValue(int value);

private:
  Mpx3GUI *_mpx3gui = nullptr;
  // Layout of the matrix. Each QPoint is a chip connected with X,Y sizes.
  QVector<QPoint> _devicePresenceLayout;
  QVector<QString> _deviceWaferIdMap;
  int _nDevicesPresent = -1;
  int _nDevicesSupported = -1;
  int _inhibitShutterRegisterOffset = 2;
  bool _isInhibitShutterSelected = false;
  int _hdmiregisterValue = 0;

signals:
  void IpAdressChanged(QString);
  void IpZmqPubAddressChanged(QString);
  void IpZmqSubAddressChanged(QString);
  void IpZmqPubAddressChangedFailed(QString);
  void IpZmqSubAddressChangedFailed(QString);
  void colourModeChanged(bool);
  void readBothCountersChanged(bool);
  void decodeFramesChanged(bool);
  void operationModeChanged(int);
  void pixelDepthChanged(int);
  void polarityChanged(int);
  void csmSpmChanged(int);
  void logLevelChanged(int);
  void gainModeChanged(int);
  void MaxPacketSizeChanged(int);
  void TriggerModeChanged(int);
  void TriggerLengthChanged(double);
  void ContRWFreqChanged(int);
  void TriggerDowntimeChanged(double);
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

  void inhibitShutterchanged(bool);

public slots:

  void setLogLevel(int newVal) {
    if (newVal != LogLevel) {
      LogLevel = newVal;
      emit logLevelChanged(newVal);
      SendConfiguration(__logLevel);
    }
  }

  void setBiasVoltage(double volt) {
    if (!qFuzzyCompare(volt, this->getBiasVoltage())) {
      biasVolt = volt;
      emit BiasVoltageChanged(volt);
      SendConfiguration(__biasVoltage);
    }
  }

  void setIpAddress(QString ipn);

  void setIpZmqPubAddress(QString ip_and_port);
  void setIpZmqSubAddress(QString ip_and_port);

  void setColourMode(bool mode);

  void setReadBothCounters(bool rbc) {
    if (rbc != readBothCounters) {
      readBothCounters = rbc;
      emit readBothCountersChanged(rbc);
      SendConfiguration(__readBothCounters);
    }
  }

  void setDecodeFrames(bool decode) {
    if (decode != decodeFrames) {
      decodeFrames = decode;
      emit decodeFramesChanged(decode);
      SendConfiguration(__decodeFrames);
    }
  }

  void setOperationMode(int newVal) {

    // This can only take some values
    if (newVal > __operationMode_NumberOf ||
        newVal < __operationMode_SequentialRW) {
      newVal = __operationMode_SequentialRW;
    }

    if (newVal != OperationMode) {
      OperationMode = newVal;
      emit operationModeChanged(newVal);
      SendConfiguration(__operationMode);
      if (newVal == __operationMode_ContinuousRW) {
        setReadBothCounters(false);
      }
      for (uint i = 0; i < getNActiveDevices(); i++) {
        _controller->setDiscCsmSpm(int(i), 1);
      }
    }
  }

  void setPixelDepth(int newVal) {

    if (newVal != PixelDepth) {
      PixelDepth = newVal;
      emit pixelDepthChanged(newVal);
      SendConfiguration(__pixelDepth);
      for (uint i = 0; i < getNActiveDevices(); i++) {
        _controller->setDiscCsmSpm(int(i), 1);
      }
    }
  }

  void setCsmSpm(int newVal) {
    if (newVal != CsmSpm) {
      CsmSpm = newVal;
      emit csmSpmChanged(newVal);
      SendConfiguration(__CsmSpm);
    }
  }
  void updateCsmSpm() {}

  void setGainMode(int newVal) {
    if (newVal != GainMode) {
      GainMode = newVal;
      emit gainModeChanged(GainMode);
      SendConfiguration(__gainMode);
    }
  }

  void setPolarity(int newVal);
  void setPolarityByString(QString itemS, int indx = -1);

  void setMaxPacketSize(int newVal) {
    if (newVal != MaxPacketSize) {
      MaxPacketSize = newVal;
      emit MaxPacketSizeChanged(newVal);
    }
    SendConfiguration(__maxPacketSize);
  }
  void updateMaxPacketSize() { _controller->setMaxPacketSize(MaxPacketSize); }

  void setTriggerMode(int newVal) {
    if (newVal != TriggerMode) {
      TriggerMode = newVal;
      emit TriggerModeChanged(newVal);
      // SendConfiguration( __triggerMode );
    }
  }

  void setContRWFreq(int newVal) {
    if (newVal != ContRWFreq) {
      ContRWFreq = newVal;
      emit ContRWFreqChanged(newVal);
      SendConfiguration(__contRWFreq);
    }
  }

  void setTriggerLength(double newVal) {
    uint64_t newVal_us = uint64_t(1000 * newVal);
    if (newVal_us != TriggerLength_us_64) {
      TriggerLength_us_64 = newVal_us;
      emit TriggerLengthChanged(newVal);
      SendConfiguration(__triggerLength);
    }
  }
  // This is connected to QAbstractSpinBox::editingFinished() which takes no
  // argument. Pick the value from the spin-box directly.

  // Units for newVal are now ms, hence the *1000
  void setTriggerDowntime(double newVal) {
    uint64_t newVal_us = uint64_t(1000 * newVal);
    if (newVal_us != TriggerDowntime_us_64) {
      TriggerDowntime_us_64 = newVal_us;
      emit TriggerDowntimeChanged(newVal);
      SendConfiguration(__triggerDowntime);
    }
  }

  void setNTriggers(int newVal) {
    if (newVal != nTriggers) {
      nTriggers = newVal;
      emit nTriggersChanged(newVal);
      SendConfiguration(__nTriggers);
    }
  }

  ////////////////////////////////////////////////////////////
  // Stepper
  // The messages to the hardware won't be sent from here in
  //  the case of the stepper.  It has it's own interface in
  //  qctsmconfigmonitoring.cpp
  void setStepperConfigUseCalib(bool newVal) {

    if (newVal != stepperUseCalib) {
      stepperUseCalib = newVal;
      // emit UseCalibChanged(newVal);
    }
  }

  void setStepperConfigAcceleration(double newVal) {

    if (!qFuzzyCompare(newVal, stepperAcceleration)) {
      stepperAcceleration = newVal;
      // emit AccelerationChanged(newVal);
    }
  }

  void setStepperConfigSpeed(double newVal) {

    if (!qFuzzyCompare(newVal, stepperSpeed)) {
      stepperSpeed = newVal;
      // emit SpeedChanged(newVal);
    }
  }

  void setStepperConfigCalib(QStandardItem *item);

  void setStepperConfigCalibPos0(double newVal) {

    if (!qFuzzyCompare(newVal, stepperCalibPos0)) {
      stepperCalibPos0 = newVal;
      // emit CalibPos0Changed(newVal);
    }
  }

  void setStepperConfigCalibAngle0(double newVal) {

    if (!qFuzzyCompare(newVal, stepperCalibAngle0)) {
      stepperCalibAngle0 = newVal;
      // emit CalibAngle0Changed(newVal);
    }
  }

  void setStepperConfigCalibPos1(double newVal) {

    if (!qFuzzyCompare(newVal, stepperCalibPos1)) {
      stepperCalibPos1 = newVal;
      // emit CalibPos1Changed(newVal);
    }
  }

  void setStepperConfigCalibAngle1(double newVal) {

    if (newVal != stepperCalibAngle1) {
      stepperCalibAngle1 = newVal;
      // emit CalibAngle1Changed(newVal);
    }
  }

  void setInhibitShutter(bool turnOn); // Slot to config inhibit_shutter signal

  void setTargetEnergy(int threshold, double energy);
};

#endif // MPX3CONFIG_H
