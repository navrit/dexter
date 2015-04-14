#ifndef MPX3CONFIG_H
#define MPX3CONFIG_H
#include "mpx3defs.h"
#include "qcstmdacs.h"

#include <QHostAddress>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

#include <stdint.h>

//Dac stuff
/*static const dac_t MPX3RX_DAC_TABLE[MPX3RX_DAC_COUNT] =
{
                {  1, "Threshold0",     30, 9, (1<<9)/2 },
                {  2, "Threshold1",     39, 9, (1<<9)/2 },
                {  3, "Threshold2",     48, 9, (1<<9)/2 },
                {  4, "Threshold3",     57, 9, (1<<9)/2 },
                {  5, "Threshold4",     66, 9, (1<<9)/2 },
                {  6, "Threshold5",     75, 9, (1<<9)/2 },
                {  7, "Threshold6",     84, 9, (1<<9)/2 },
                {  8, "Threshold7",     93, 9, (1<<9)/2 },
                {  9, "I_Preamp",          102, 8, (1<<8)/2 },
                { 10, "I_Ikrum",           110, 8, (1<<8)/2 },
                { 11, "I_Shaper",          118, 8, (1<<8)/2 },
                { 12, "I_Disc",            126, 8, (1<<8)/2 },
                { 13, "I_Disc_LS",         134, 8, (1<<8)/2 },
                { 14, "I_Shaper_Test",     142, 8, (1<<8)/2 },
                { 15, "I_DAC_DiscL",       150, 8, (1<<8)/2 },
                { 30, "I_DAC_test",        158, 8, (1<<8)/2 },
                { 31, "I_DAC_DiscH",       166, 8, (1<<8)/2 },
                { 16, "I_Delay",           174, 8, (1<<8)/2 },
                { 17, "I_TP_BufferIn",     182, 8, (1<<8)/2 },
                { 18, "I_TP_BufferOut",    190, 8, (1<<8)/2 },
                { 19, "V_Rpz",             198, 8, (1<<8)/2 },
                { 20, "V_Gnd",             206, 8, (1<<8)/2 },
                { 21, "V_Tp_ref",          214, 8, (1<<8)/2 },
                { 22, "V_Fbk",             222, 8, (1<<8)/2 },
                { 23, "V_Cas",             230, 8, (1<<8)/2 },
                { 24, "V_Tp_refA",         238, 9, (1<<9)/2 },
                { 25, "V_Tp_refB",         247, 9, (1<<9)/2 }
};*/

class Mpx3Config
{
  //Spidr stuff
  QHostAddress SpidrAddress;
  uint16_t port;
  //Operation stuff
  bool colourMode, decodeFrames;
  int OperationMode, PixelDepth, CsmSpm, GainMode, MaxPacketSize, TriggerMode, TriggerLength_us, nTriggers;

  QVector<int> _dacVals[MPX3RX_DAC_COUNT];

public:
  Mpx3Config();
  void fromJsonFile(QString filename);
  void toJsonFile(QString filename);
  QString getIpAddress(){return QString("%1:%2").arg(SpidrAddress.toString()).arg(port);}
  bool getColourMode(){return colourMode;}
  bool getDecodeFrames(){return decodeFrames;}
  int getOperationMode(){return OperationMode;}
  int getPixelDepth(){return PixelDepth;}
  int getCsmSpm(){return CsmSpm;  }
  int getGainMode(){return GainMode;}
  int getMaxPacketSize(){return MaxPacketSize;}
  int getTriggerLength(){return TriggerLength_us;}
  int getNTriggers(){return nTriggers;}
};

#endif // MPX3CONFIG_H
