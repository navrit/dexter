#ifndef QCSTMCONFIGMONITORING_H
#define QCSTMCONFIGMONITORING_H

#include <QWidget>
#include "mpx3gui.h"

#include <QtWidgets>
//server
#include "tcpserver.h"



namespace Ui {
class QCstmConfigMonitoring;
}

class QCstmConfigMonitoring : public QWidget
{

    Q_OBJECT

    Mpx3GUI * _mpx3gui;

public:
    void SetMpx3GUI(Mpx3GUI * p);
    void widgetInfoPropagation();
    explicit QCstmConfigMonitoring(QWidget *parent = 0);
    ~QCstmConfigMonitoring();
    Ui::QCstmConfigMonitoring * getUI() { return ui; }

    void timerEvent( QTimerEvent * );

    void setWindowWidgetsStatus(win_status s = win_status::startup);


    void activeInGUI();
    void activateItemsGUI();
    void deactivateItemsGUI();

    void angleModeGUI();
    void stepsModeGUI();

    void readMonitoringInfo();

    unsigned int getPixelDepthFromIndex(int indx);
    unsigned int getPixelDepth12BitsIndex() { return __pixelDepth12BitsIndex; }

    void setReadoutFrequency(int frequency);

public slots:

    void OperationModeSwitched(int indx);
    void when_taking_data_gui();
    void when_idling_gui();

    void shortcutGainModeSLGM();
    void shortcutGainModeLGM();
    void shortcutGainModeHGM();
    void shortcutGainModeSHGM();
    void shortcutCSMOff();
    void shortcutCSMOn();

private slots:
    void ConnectionStatusChanged(bool);

    void setPixelDepthByIndex(int newValIndx);
    void pixelDepthChangedByValue(int val);

    void setTriggerModeByIndex(int newValIndx);
    void triggerModeChangedByValue(int val);

    void setCsmSpmByIndex(int newValIndx);
    void csmSpmChangedByValue(int val);

    void IpAddressEditFinished();
    void IpZmqPubAddressEditFinished(); // Nearly the same as IpAddressEditFinished, this could be refactored
    void IpZmqSubAddressEditFinished(); // Nearly the same as IpAddressEditFinished, this could be refactored

    void nTriggersEdited();
    void ContRWFreqEdited();
    void TriggerLengthEdited();
    void TriggerDowntimeEdited();

    void on_SaveButton_clicked();

    void on_LoadButton_clicked();

    void on_ColourModeCheckBox_toggled(bool checked);

    void on_tempReadingActivateCheckBox_toggled(bool checked);

    void on_readOMRPushButton_clicked();

    void biasVoltageChanged();
    void setLogLevel();

    void on_tstBtn_clicked();

private:
    Ui::QCstmConfigMonitoring *ui;
    int _timerId;

    // Some constants in the configuration (MPX3 manual pag. 18)
    vector<unsigned int> __pixelDepthMap; // = { 1 , 6 , 12 , 24 };
    const unsigned int __pixelDepth12BitsIndex = 2;

    vector<unsigned int> __triggerModeMap;

    vector<unsigned int> __csmSpmMap;

};


#endif // QCSTMCONFIGMONITORING_H
