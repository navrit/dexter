/**
 * John Idarraga <idarraga@cern.ch>
 * Nikhef, 2014.
 */

#ifndef QCSTMDACS_H
#define QCSTMDACS_H

#include <QWidget>
class Mpx3GUI;
#include "mpx3gui.h"
#include "mpx3defs.h"

#include <QDialog>
#include <QThread>
#include <QJsonObject>

#include <map>
#include <set>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
using namespace std;

#define 	__nDACs_MPX3RX		27
#define		__default_DACs_filename "mpx3_defaultDACs.json"
#define		__N_RETRY_ORIGINAL_SETTING	3

class SpidrController;
class SpidrDaq;
class QCustomPlot;
class QCPGraph;

namespace Ui {
  class QCstmDacs;
}

// Taken from LEON Software.  Henk.
typedef struct dac_s
{
	int         code;
	const char *name;
	int         offset;
	int         size;
	int         dflt;
} dac_t;

// Tables with descriptions of DACs in the DACs register
// according to device type

// NB: with the 'offset' values given here (taken from the Medipix3.1 manual),
// a bit array of length 256 (32 bytes) should be used !
// (which is the correct length of the MPX3 DACs register;
//  existing code had 'Threshold[0]' offset at 0, etc, requiring an array
//  length value of 210, when filling it using 'bitarray' functions,
//  which is a bit strange...)

static const dac_t MPX3RX_DAC_TABLE[MPX3RX_DAC_COUNT] =
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
};

static const QColor COLOR_TABLE[] = {
		Qt::red, // 1
		Qt::black,
		Qt::darkRed,
		Qt::green,
		Qt::darkGreen,
		Qt::blue,
		Qt::darkBlue,
		Qt::cyan,
		Qt::darkCyan,
		Qt::magenta, // 10
		Qt::darkMagenta,
		Qt::yellow,
		Qt::darkYellow,
		QColor( "darkorange" ),
		QColor( "purple" ),
		QColor( "khaki" ),
		QColor( "gold" ),
		QColor( "dodgerblue" ), // 18
		QColor( "light gray" ),
		QColor( "medium gray" ),
		QColor( "red" ),
		QColor( "green" ),
		QColor( "blue" ),
		QColor( "cyan" ),
		QColor( "magenta" ),
		QColor( "yellow" ),
		QColor( "dark yellow" ) // 27
};


class QSpinBox;
class QSlider;
class QLabel;
class QCheckBox;
class SenseDACsThread;
class ScanDACsThread;
class UpdateDACsThread;
class QSignalMapper;
class ModuleConnection;

class SignalSlotMapping : public QObject {

	Q_OBJECT

public:
	explicit SignalSlotMapping(){};
	~SignalSlotMapping(){};

	int index;
	int value;

};

class QCstmDacs : public QWidget {

	Q_OBJECT

public:

	explicit QCstmDacs();
	explicit QCstmDacs(QWidget *parent = 0);
	//explicit QCstmDacs(QApplication * coreApp, Ui::Mpx3GUI * );
	~QCstmDacs();
	void PopulateDACValues();
	void FillDACValues(int devId = -1);

	bool ReadDACsFile(string);
	bool WriteDACsFile(string);


public:

	//Ui::Mpx3GUI
	Ui::QCstmDacs  * GetUI() { return ui; };
	QSpinBox ** GetSpinBoxList() { return _dacSpinBoxes; };
	QSlider ** GetSliderList() { return _dacSliders; };
	QLabel ** GetLabelsList() { return _dacVLabels ; };
	QCheckBox ** GetCheckBoxList() { return _dacCheckBoxes; };
	int GetDeviceIndex() { return _deviceIndex; };
	int GetNSamples() { return _nSamples; };
	int GetScanStep() { return _scanStep; };
	QCPGraph * GetGraph(int idx);
	//QCustomPlot * GetQCustomPlotPtr() { return _dacScanPlot; };
	void SetMpx3GUI(Mpx3GUI * p) { _mpx3gui = p; };
	void getConfig();
	void setConfig();
	int GetDACValue(int chip, int dacIndex) { return _dacVals[dacIndex][chip]; };

private:

	Ui::QCstmDacs * ui;

	void FillWidgetVectors();
	void SetLimits();
	/*all the configuration bits*/
	//QJsonObject configJson;
	//QJsonArray configJson;
	QJsonDocument * jsonDocument;
	int _nDACConfigsAvailable;

	// Connectivity between modules
	Mpx3GUI * _mpx3gui;

	// Currently active graph
	QCPGraph *_graph;

	//
	SenseDACsThread * _senseThread;
	ScanDACsThread * _scanThread;
	UpdateDACsThread * _updateDACsThread;

	// Vectors of Widgets
	QSpinBox  * _dacSpinBoxes[MPX3RX_DAC_COUNT];
	QSlider   * _dacSliders[MPX3RX_DAC_COUNT];
	QLabel    * _dacVLabels[MPX3RX_DAC_COUNT];
	QLabel    * _dacLabels[MPX3RX_DAC_COUNT];
	QCheckBox * _dacCheckBoxes[MPX3RX_DAC_COUNT];


	// Keep track of DAC values.  The second dimension here
	//  corresponds to the array
	vector<int> _dacVals[MPX3RX_DAC_COUNT];

	// Scan
	int _scanStep;
	// Current device Id
	int _deviceIndex;
	// Samples
	int _nSamples;

	// In case only a subset of the DACs are selected
	//  to produce the scan, keep track of the id's
	map<int, int> _plotIdxMap;
	int _plotIdxCntr;

	QSignalMapper * _signalMapperSliderSpinBoxConn;
	QSignalMapper * _signalMapperSlider;
	QSignalMapper * _signalMapperSpinBox;


private slots:

void setTextWithIdx(QString,int);
void UncheckAllDACs();
void CheckAllDACs();
void setValueDAC(int);
void StartDACScan();
void SetupSignalsAndSlots();
void FromSpinBoxUpdateSlider(int);
void FromSliderUpdateSpinBox(int);
void SenseDACs();
void ChangeDeviceIndex(int);
void ChangeNSamples(int);
void ChangeScanStep(int);
void addData(int, int, double);
void addData(int);
void scanFinished();
void slideAndSpin(int, int);
void openWriteMenu();
void ConnectionStatusChanged();

};

class SenseDACsThread : public QThread {

	Q_OBJECT

public:

	explicit SenseDACsThread (int devIndx, QCstmDacs * dacs, SpidrController * sc);

private:

	SpidrController * _spidrcontrol;
	QCstmDacs * _dacs;
	int _deviceIndex;
	// IP source address (SPIDR network interface)
	int _srcAddr;

	void run();

signals:

	// These are used in the parent class as a signal to thread-safe feed
	//  widgets in the ui
	void progress(int);
	void fillText(QString);

};

class ScanDACsThread : public QThread {

	Q_OBJECT

public:

	explicit ScanDACsThread (int devIndx, QCstmDacs * dacs, SpidrController * sc);

private:

	SpidrController * _spidrcontrol;
	QCstmDacs * _dacs;
	int _deviceIndex;
	// IP source address (SPIDR network interface)
	int _srcAddr;

	void run();

signals:

	// These are used in the parent class as a signal to thread-safe feed
	//  widgets in the ui
	void progress(int);
	void fillText(QString);
	void fillTextWithIdx(QString, int);
	void addData(int, int, double);
	void addData(int);
	void scanFinished();
	void slideAndSpin(int, int);

};


class UpdateDACsThread : public QThread {

	Q_OBJECT

public:

	explicit UpdateDACsThread (int devIndx, int nDACConfigsAvailable, QCstmDacs * dacs, SpidrController * sc);

private:

	SpidrController * _spidrcontrol;
	QCstmDacs * _dacs;
	int _deviceIndex;
	int _nDACConfigsAvailable;
	// IP source address (SPIDR network interface)
	int _srcAddr;

	void run();

signals:

	// These are used in the parent class as a signal to thread-safe feed
	//  widgets in the ui
	void progress(int);
	void fillText(QString);
	void fillTextWithIdx(QString, int);
	void scanFinished();
	void slideAndSpin(int, int);

};


#endif

