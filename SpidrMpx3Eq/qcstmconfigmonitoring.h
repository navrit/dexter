#ifndef QCSTMCONFIGMONITORING_H
#define QCSTMCONFIGMONITORING_H

#include <QWidget>
#include "mpx3gui.h"
#include <QtWidgets>

#define __nwords_OMR 6
#define __nbits_OMR 48 // 6 words of 8 bits

namespace Ui {
    class QCstmConfigMonitoring;
}

class QCstmConfigMonitoring : public QWidget
{
    Q_OBJECT

    Mpx3GUI * _mpx3gui = nullptr;

public:

    void SetMpx3GUI(Mpx3GUI * p);
    void widgetInfoPropagation();
    explicit QCstmConfigMonitoring(QWidget *parent = nullptr);
    ~QCstmConfigMonitoring();
    static QCstmConfigMonitoring *getInstance();
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

    void protectTriggerMode(SpidrController* spidrController); //this function firstreturn the current trigger mode and then set trigger mode to auto
    //the purpose is to prevent the power drop after setting the dac when the trigger mode is external. after setting the dac(e.g. threshold)one
    //has to set the trigger mode to the return value of this function

    void returnLastTriggerMode(SpidrController *spidrController);  //call after u set the dac , input should be the output of protetTriggerMode() function.
    void returnLastTriggerMode2(SpidrController *spidrController); //this function set the trigger mode to the one that user set either locally or remotely.
    //the mode is set by user, is saved in TriggerMode  field of Mpx3Config class and is read from this field in this function to send it to device


public slots:

    void OperationModeSwitched(int indx);
    void when_taking_data_gui();
    void when_idling_gui();
    void developerMode(bool enabled);

    void shortcutGainModeSLGM();
    void shortcutGainModeLGM();
    void shortcutGainModeHGM();
    void shortcutGainModeSHGM();
    void shortcutCSMOff();
    void shortcutCSMOn();
    void setTriggerModeByIndex(int newValIndx);
    int getTriggerModeIndex();
    void saveConfigFileRemotely(QString path);

private slots:
    void ConnectionStatusChanged(bool);
    void setUpGuiForReadOutMode(int);
    void setPixelDepthByIndex(int newValIndx);
    void pixelDepthChangedByValue(int val);

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

    void on_merlinInterfaceTestButton_clicked();

    void setContRWFreqFromConfig(int val);
    void setMaximumFPSinVisualiation();

    void hideLabels();

private:
    Ui::QCstmConfigMonitoring *ui = nullptr;
    int _timerId;

    // Some constants in the configuration (MPX3 manual pag. 18)
    vector<unsigned int> __pixelDepthMap; // = { 1 , 6 , 12 , 24 };
    const unsigned int __pixelDepth12BitsIndex = 2;

    vector<unsigned int> __triggerModeMap;
    int findTriggerModeIndex(int val);

    vector<unsigned int> __csmSpmMap;

    bool _saveConfigFileRemotely = false;
    QString _conigFileDestination = " ";

    void setMaximumFPSFromPixelDepth(int idx=-1, int val=-1);

    bool _isDeveloperMode = false;

    struct SHUTTER_INFO{
        int trigger_mode;
        int trigger_width_us;
        int trigger_freq_mhz;
        int nr_of_triggers;
        int trigger_pulse_count;
    };

    SHUTTER_INFO shutterInfo;

};

#endif // QCSTMCONFIGMONITORING_H
