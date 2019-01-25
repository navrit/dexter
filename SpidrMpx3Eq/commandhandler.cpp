#include "commandhandler.h"
#include <QDebug>
#include <QTime>
#include "handlerfunctions.h"
#include "qcstmglvisualization.h"
#include "qcstmdacs.h"
#include "thresholdscan.h"
#include "ui_thresholdscan.h"
#include "mpx3gui.h"
#include "dataconsumerthread.h"
#include "qcstmequalization.h"


//#include <QTimer> // For the test function

CommandHandler::CommandHandler(QObject *parent) : QObject(parent)
{
    QCstmGLVisualization * visualisation = ((Mpx3GUI*) parent)-> getVisualization();
    connect(this,SIGNAL(requestForDataTaking(bool)),visualisation,SLOT(StartDataTakingRemotely(bool)));
    connect(this,SIGNAL(requestForInfDataTracking(bool)),visualisation,SLOT(on_infDataTakingCheckBox_toggled(bool)));
    connect(this,SIGNAL(requestForSnap()),visualisation,SLOT(on_singleshotPushButton_clicked()));
    connect(this,SIGNAL(requestForAutoSave(bool)),visualisation,SLOT(onRequestForAutoSaveFromServer(bool)));
    connect(this,SIGNAL(requestForSettingSavePath(QString)),visualisation,SLOT(onRequestForSettingPathFromServer(QString)));
    connect(visualisation,SIGNAL(someCommandHasFinished_Successfully()),this,SLOT(on_someCommandHasFinished_Successfully()));
    connect(this,SIGNAL(requestToMaskPixelRemotely(int,int)),visualisation,SLOT(onRequestToMaskPixelRemotely(int,int)));
    connect(this,SIGNAL(requestToUnmaskPixelRemotely(int,int)),visualisation,SLOT(onRequestToUnmaskPixelRemotely(int,int)));
    connect(this,SIGNAL(requestToLoadEqualizationRemotely(QString)),getGui(),SLOT(loadEqualisationFromPathRemotely(QString)));
    connect(QCstmEqualization::getInstance(),SIGNAL(equalizationPathExported(QString)),this,SLOT(on_equalizationPathExported(QString)));
    connect(this,SIGNAL(requestToStartStopThresholdScan()),thresholdScan::getInstance(),SLOT(button_startStop_clicked_remotely()));
    connect(this,SIGNAL(requestToDoEqualizationRemotely(QString)),QCstmEqualization::getInstance(),SLOT(StartEqualizationSequentialSingleChipsRemotely(QString)));
    connect(this,SIGNAL(requestToLoadConfigRemotely(QString)),getGui(),SLOT(load_config_remotely(QString)));
    connect(this,SIGNAL(requestToSaveConfigRemotely(QString)),QCstmConfigMonitoring::getInstance(),SLOT(saveConfigFileRemotely(QString)));
    connect(this,SIGNAL(requestToChangeGuiforThreshold(int)),QCstmDacs::getInstance(),SLOT(onDevNumChanged(int)));
    initializeCmdTable();

    //QTimer::singleShot(10000, this, SLOT(testSetThreshold_idx_val_chipId())); // Test function to show setting thresholds does work
}

Mpx3GUI* CommandHandler::getGui() {
    return (Mpx3GUI*) parent();
}

void CommandHandler::initializeCmdTable()
{
    cmd_struct hello {helloHandler};
    cmdTable.insert("Hello",hello);
    cmd_struct bye{byeHandler};
    cmdTable.insert("Bye",bye);
    cmd_struct setReadoutMode {setReadoutModeHandler};
    cmdTable.insert("SetReadoutMode",setReadoutMode);
    cmd_struct getReadoutMode {getReadoutModeHandler};
    cmdTable.insert("GetReadoutMode",getReadoutMode);
    cmd_struct setCounterDepth{setCounterDepthHandler};
    cmdTable.insert("SetCounterDepth",setCounterDepth);
    cmd_struct getCounterDepth{getCounterDepthHandler};
    cmdTable.insert("GetCounterDepth",getCounterDepth);



    //temperatures info
    cmd_struct getChipTemperature{getChipTemperatureHandler};
    cmdTable.insert("GetChipTemperature",getChipTemperature);
    cmd_struct getBoardTemperature{getBoardTemperatureHandler};
    cmdTable.insert("GetBoardTemperature",getBoardTemperature);
    cmd_struct getFpgaTemperature{getFpgaTemperatureHandler};
    cmdTable.insert("GetFpgaTemperature",getFpgaTemperature);
    cmd_struct getHumidity{getHumidityHandler};
    cmdTable.insert("GetHumidity",getHumidity);



    cmd_struct setOperationalMode{setOperationalModeHandler};
    cmdTable.insert("SetOperationalMode",setOperationalMode);
    cmd_struct getOperationalMode{getOperationalModeHandler};
    cmdTable.insert("GetOperationalMode",getOperationalMode);
    cmd_struct open {openHandler};
    cmdTable.insert("Open",open);
    cmd_struct close {closeHandler};
    cmdTable.insert("Close",close);
    cmd_struct start{startHandler};
    cmdTable.insert("Start",start);
    cmd_struct stop{stopHandler};
    cmdTable.insert("Stop",stop);
    cmd_struct snap{snapHandler};
    cmdTable.insert("Snap",snap);
    cmd_struct setAutoSave { setAutoSaveHandler};
    cmdTable.insert("SetAutoSave",setAutoSave);
    cmd_struct setRecordPath{setRecordPathHandler};
    cmdTable.insert("SetRecordPath",setRecordPath);
    cmd_struct setRecordFormat {setRecordFormatHandler};
    cmdTable.insert("SetRecordFormat",setRecordFormat);
    cmd_struct getImage{getImageHandler};
    cmdTable.insert("GetImage",getImage);
    cmd_struct setGainMode {setGainModeHandler};
    cmdTable.insert("SetGainMode",setGainMode);
    cmd_struct getGainMode {getGainModeHandler};
    cmdTable.insert("GetGainMode",getGainMode);
    cmd_struct setPolarity {setPolarityHandler};
    cmdTable.insert("SetPolarity",setPolarity);
    cmd_struct getPolarity { getPolarityHandler};
    cmdTable.insert("GetPolarity",getPolarity);
    cmd_struct setCounterSelectFrequency {setCounterSelectFrequencyHandler};
    cmdTable.insert("SetCounterSelectFrequency",setCounterSelectFrequency);
    cmd_struct getCounterSelectFrequency = {getCounterSelectFrequencyHandler};
    cmdTable.insert("GetCounterSelectFrequency",getCounterSelectFrequency);
    cmd_struct setShutterLength{setShutterLengthHandler};
    cmdTable.insert("SetShutterLength",setShutterLength);
    cmd_struct getShutterLength{getShutterLengthHandler};
    cmdTable.insert("GetShutterLength",getShutterLength);
    cmd_struct setBothCounters{setBothCountersHandler};
    cmdTable.insert("SetBothCounters",setBothCounters);
    cmd_struct getBothCounters{getBothCountersHandler};
    cmdTable.insert("GetBothCounters",getBothCounters);
    cmd_struct setColourMode{setColourModeHandler};
    cmdTable.insert("SetColourMode",setColourMode);
    cmd_struct getColourMode{getColourModeHandler};
    cmdTable.insert("GetColourMode",getColourMode);
    cmd_struct setChargeSumming{setChargeSummingHandler};
    cmdTable.insert("SetChargeSumming",setChargeSumming);
    cmd_struct getChargeSumming{getChargeSummingHandler};
    cmdTable.insert("GetChargeSumming",getChargeSumming);
    cmd_struct setShutterPeriod{setShutterPeriodHandler};
    cmdTable.insert("SetShutterPeriod",setShutterPeriod);
    cmd_struct getShutterPeriod{getShutterPeriodHandler};
    cmdTable.insert("GetShutterPeriod",getShutterPeriod);
    cmd_struct setNumberOfFrame {setNumberOfFrameHandler};
    cmdTable.insert("SetFrameNumber",setNumberOfFrame);
    cmd_struct getNumberOfFrame {getNumberOfFrameHandler};
    cmdTable.insert("GetFrameNumber",getNumberOfFrame);
    cmd_struct setThreshold {setThresholdHandler};
    cmdTable.insert("SetThreshold",setThreshold);
    cmd_struct getThreshold {getThresholdHandler};
    cmdTable.insert("GetThreshold",getThreshold);

    cmd_struct setThresholdPerChip{setThresholdPerChipHandler};
    cmdTable.insert("SetThresholdPerChip",setThresholdPerChip);
    cmd_struct getThresholdPerChip{getThresholdPerChipHandler};
    cmdTable.insert("GetThresholdPerChip",getThresholdPerChip);

    cmd_struct setStartScan {setStartScanHandler};
    cmdTable.insert("SetStartScan",setStartScan);
    cmd_struct getStartScan {getStartScanHandler};
    cmdTable.insert("GetStartScan",getStartScan);
    cmd_struct setStopScan {setStopScanHandler};
    cmdTable.insert("SetStopScan",setStopScan);
    cmd_struct getStopScan {getStopScanHandler};
    cmdTable.insert("GetStopScan",getStopScan);
    cmd_struct setStepScan {setStepScanHandler};
    cmdTable.insert("SetStepScan",setStepScan);
    cmd_struct getStepScan {getStepScanHandler};
    cmdTable.insert("GetStepScan",getStepScan);
    cmd_struct setThresholdScan{setThresholdScanHandler};
    cmdTable.insert("SetThresholdScan",setThresholdScan);
    cmd_struct getThresholdScan{getThresholdScanHandler};
    cmdTable.insert("GetThresholdScan",getThresholdScan);
    cmd_struct setFramesPerScan{setFramesPerScanHandler};
    cmdTable.insert("SetFramesPerScan",setFramesPerScan);
    cmd_struct getFramesPerScan{getFramesPerScanHandler};
    cmdTable.insert("GetFramesPerScan",getFramesPerScan);
    cmd_struct setScanPath{setScanPathHandler};
    cmdTable.insert("SetScanPath",setScanPath);
    cmd_struct getScanPath{getScanPathHandler};
    cmdTable.insert("GetScanPath",getScanPath);


    cmd_struct startScan {startScanHandler};
    cmdTable.insert("StarStoptScan",startScan);


    cmd_struct setOperatingEnergy {setOperatingEnergyHandler};
    cmdTable.insert("SetOperatingEnergy",setOperatingEnergy);
    cmd_struct getOperatingEnergy{getOperatingEnergyHandler};
    cmdTable.insert("GetOperatingEnergy",getOperatingEnergy);
    cmd_struct setProfile {setProfileHandler};
    cmdTable.insert("SetProfile",setProfile);
    cmd_struct getProfile {getProfileHandler};
    cmdTable.insert("GetProfile",getProfile);
    cmd_struct setTriggerMode{setTriggerModeHandler};
    cmdTable.insert("SetTriggerMode",setTriggerMode);
    cmd_struct getTriggerMode{getTriggerModeHandler};
    cmdTable.insert("GetTriggerMode",getTriggerMode);
    cmd_struct setMaskPixel{setMaskPixelHandler};
    cmdTable.insert("SetMaskPixel",setMaskPixel);
    cmd_struct setUnmaskPixel{setUnmaskPixelHandler};
    cmdTable.insert("SetUnmaskPixel",setUnmaskPixel);
    cmd_struct setEqualizationPath {setEqualizationHandler};
    cmdTable.insert("SetEqualizationPath",setEqualizationPath);
    cmd_struct getEqualizationPath {getEqualizationHandler};
    cmdTable.insert("GetEqualizationPath",getEqualizationPath);

    cmd_struct setConfig {setConfigHandler};
    cmdTable.insert("SetConfig",setConfig);
    cmd_struct getConfig {getConfigHandler};
    cmdTable.insert("GetConfig",getConfig);

    cmd_struct saveConfig{saveConfigHandler};
    cmdTable.insert("SaveConfig",saveConfig);


    cmd_struct setDoEqualization {setDoEqualizationHandler};
    cmdTable.insert("SetDoEqualization",setDoEqualization);

    cmd_struct setInhibitShutter { setInhibitShutterHandler};
    cmdTable.insert("SetInhibitShutter",setInhibitShutter);

    cmd_struct getInhibitShutter { getInhibitShutterHandler};
    cmdTable.insert("GetInhibitShutter",getInhibitShutter);

    cmd_struct setSlope {setSlopeHandler};
    cmdTable.insert("SetSlope",setSlope);

    cmd_struct getSlope {getSlopeHandler};
    cmdTable.insert("GetSlope",getSlope);

    cmd_struct setOffset {setOffsetHandler};
    cmdTable.insert("SetOffset",setOffset);

    cmd_struct getOffset {getOffsetHandler};
    cmdTable.insert("GetOffset",getOffset);

    cmd_struct resetSlopesAndOffsets{resetSlopesAndOffsetsHandler};
    cmdTable.insert("ResetSlopesAndOffsets",resetSlopesAndOffsets);


}

bool Command::enoughArguments(int argsNum,QString command)
{
    if(argsNum != arguments.size())
    {
        data = "Too many or too few arguments...";
        return false;
    }
    return true;
}

ERROR_TYPE Command::getError()
{
    return _error;
}

void Command::setError(ERROR_TYPE et)
{
    _error = et;
}

void CommandHandler::startLiveCamera(bool takingDataFlag)
{
    //emit requestForInfDataTracking(true);
    emit requestForDataTaking(!takingDataFlag);
    //QCstmGLVisualization::getInstance()->setInfDataTaking(true);
}

void CommandHandler::startSnap()
{
    emit requestForSnap();
}

void CommandHandler::setAutoSave(bool val)
{
    emit requestForAutoSave(val);
}

void CommandHandler::setRecordPath(QString path)
{
    emit requestForSettingSavePath(path);
}

void CommandHandler::setRecordFormat(int idx)
{
    emit requestForSettingSaveFormat(idx);
}

int CommandHandler::setThreshold(int idx, int val)
{
    if(idx >= 0 && idx <= 7){
        QCstmDacs::getInstance()->GetCheckBoxList()[idx]->setChecked(true);
        QCstmDacs::getInstance()->GetSpinBoxList()[idx]->setValue(val);
        QCstmDacs::getInstance()->getAllDACSimultaneousCheckBox()->setChecked(true);

        qApp->processEvents(); /* The GUI won't process events if they all occur in a very short timescale unless you make it */

        return NO_ERROR;
    }
    return UNKNOWN_ERROR;
}

int CommandHandler::setThreshold(int idx, int val, int chipId)
{

    if (idx >= 0 && idx <= 7 && chipId >= 0 && chipId < 4) {
        SpidrController *spidrController = Mpx3GUI::getInstance()->GetSpidrController();
        Mpx3GUI::getInstance()->getConfigMonitoring()->protectTriggerMode(spidrController);
        QCstmDacs::getInstance()->setRemoteRequestForSettingThreshold(true);
        // Set actual DAC
        spidrController->setDac(chipId,idx+1,val);
        QCstmGLVisualization::getInstance()->setThresholdsVector(chipId,idx,val);
        Mpx3GUI::getInstance()->getConfigMonitoring()->returnLastTriggerMode(spidrController);
        return NO_ERROR;
    }
    return UNKNOWN_ERROR;
}

void CommandHandler::testSetThreshold_idx_val_chipId() {
    setThreshold(0, 1, 0);
    setThreshold(0, 2, 0);
    setThreshold(0, 3, 1);
    setThreshold(0, 4, 1);
    setThreshold(0, 5, 1);
    setThreshold(0, 6, 0);
}

int CommandHandler::getThreshold(int idx)
{
    if (idx >= 0 && idx <= 7){
        if (QCstmDacs::getInstance()->GetCheckBoxList()[idx]->isChecked())
            return QCstmDacs::getInstance()->GetSpinBoxList()[idx]->value();
        return NO_ERROR;
    }
    return UNKNOWN_ERROR;
}

int CommandHandler::getThreshold(int idx,int chipId,int *val)
{
    if (idx >= 0 && idx <= 7 && chipId >= 0 && chipId < 4) {
//         QCstmDacs::getInstance()->getAllDACSimultaneousCheckBox()->setChecked(false);
//         QCstmDacs::getInstance()->getDeviceIdSpinBox()->setValue(chipId);
//         *val = QCstmDacs::getInstance()->GetSpinBoxList()[idx]->value();
        *val = QCstmGLVisualization::getInstance()->getThresholdVector(chipId,idx);
        return NO_ERROR;
    }
    return UNKNOWN_ERROR;
}

int CommandHandler::setStartScan(int val)
{
    if (val < 0 || val > 511)
        return ARG_VAL_OUT_RANGE;
    thresholdScan::getInstance()->GetUI()->spinBox_minimum->setValue(val);
    return NO_ERROR;
}

int CommandHandler::setStopScan(int val)
{
    if(val < 1 || val > 512)
        return ARG_VAL_OUT_RANGE;
    thresholdScan::getInstance()->GetUI()->spinBox_maximum->setValue(val);
    return NO_ERROR;
}

int CommandHandler::setStepScan(int val)
{
    if(val < 1 || val > 510)
        return ARG_VAL_OUT_RANGE;
    qDebug() << "step : " << val;
    thresholdScan::getInstance()->GetUI()->spinBox_spacing->setValue(val);
    return NO_ERROR;
}

int CommandHandler::setThresholdScan(int val)
{
    QComboBox* comboBox = thresholdScan::getInstance()->GetUI()->comboBox_thresholdToScan;
    int max = comboBox->count();
    if (val >= 0 && val < max)
    {
       comboBox->setCurrentIndex(val);
       return NO_ERROR;
    }
    return ARG_VAL_OUT_RANGE;
}

int CommandHandler::setFramesPerScan(int val)
{
    QSpinBox* spinBox = thresholdScan::getInstance()->GetUI()->spinBox_framesPerStep;
    int max = spinBox->maximum();
    int min = spinBox->minimum();
    if (val >= min && val < max)
    {
       spinBox->setValue(val);
       return NO_ERROR;
    }
    return ARG_VAL_OUT_RANGE;
}

int CommandHandler::setScanPath(QString path)
{
    QFileInfo outputDir(path);
    if(!outputDir.exists())
        return UNKNOWN_COMMAND;
    thresholdScan::getInstance()->GetUI()->textEdit_path->setText(path);
    return NO_ERROR;

}

int CommandHandler::getStartScan()
{
    return thresholdScan::getInstance()->GetUI()->spinBox_minimum->value();
}

int CommandHandler::getStopScan()
{
    return thresholdScan::getInstance()->GetUI()->spinBox_maximum->value();
}

int CommandHandler::getStepScan()
{
    return thresholdScan::getInstance()->GetUI()->spinBox_spacing->value();
}

int CommandHandler::getThresholdScan()
{
    return thresholdScan::getInstance()->GetUI()->comboBox_thresholdToScan->currentIndex();
}

int CommandHandler::getFramesPerScan()
{
    return thresholdScan::getInstance()->GetUI()->spinBox_framesPerStep->value();
}

QString CommandHandler::getScanPath()
{
    return thresholdScan::getInstance()->GetUI()->textEdit_path->toPlainText();
}

void CommandHandler::startScan()
{
    emit requestToStartStopThresholdScan();
}

void CommandHandler::startSendingImage(bool send)
{
    _sendingImage = send;
}

void Command::merlinErrorToPslError(int errNum)
{
    //ERROR_TYPE{NO_ERROR = 0, UNKNOWN_ERROR = 1, UNKNOWN_COMMAND = 2, PARAM_OUT_OF_RANGE = 3};
    //ERROR_TYPE{NO_ERROR = 0, UNKNOWN_ERROR = -1, UNKNOWN_COMMAND = -2 , ARG_NUM_OUT_RANGE = -3, ARG_VAL_OUT_RANGE = -4};
    switch (errNum) {
    case 0:
        _error = NO_ERROR;
        break;
    case 1:
        _error = UNKNOWN_ERROR;
        break;
    case 2:
        _error = UNKNOWN_COMMAND;
        break;
    case 3:
        _error = ARG_NUM_OUT_RANGE;
        break;
    default:
        _error = UNKNOWN_ERROR;
        break;
    }
}

FrameHeaderDataStruct::FrameHeaderDataStruct(Mpx3GUI *gui)
{
    if(gui->getConfig()->getPixelDepth() == 24)
        pixelDepth = "U32";
    else
        pixelDepth = "U16";

    colorMode = uint8_t(gui->getConfig()->getColourMode());
    counter = 0;/// to be set
    dataOffset = 256 + (128 * 4);
    uint8_t  gainMap[] = {3,2,1,0};
    gainMode =  gainMap[gui->getConfig()->getGainMode()];
    numberOfChips = uint8_t(gui->getConfig()->getActiveDevices().count());
    for (int i = 0; i < numberOfChips; ++i) {
        chipSelect |= 1 << i;
    }
    shutterOpen = double(gui->getConfig()-> getTriggerLength_ms() / double(1000));
    QPoint pnt = gui->getDataset()->getSize();
    xDim = uint32_t(pnt.x()*2);
    yDim = uint32_t(pnt.y()*2);
    QDateTime time = QDateTime::currentDateTime();
    timeStamp = time.toString("yyyy-dd-MM hh:mm:ss.ssssss");
    auto dacs = gui->getDACs();
    threshold0 = dacs->GetSpinBoxList()[0]->value();
    threshold1 = dacs->GetSpinBoxList()[1]->value();
    threshold2 = dacs->GetSpinBoxList()[2]->value();
    threshold3 = dacs->GetSpinBoxList()[3]->value();
    threshold4 = dacs->GetSpinBoxList()[4]->value();
    threshold5 = dacs->GetSpinBoxList()[5]->value();
    threshold6 = dacs->GetSpinBoxList()[6]->value();
    threshold7 = dacs->GetSpinBoxList()[7]->value();

    //dac
    dacThreshold0 = dacs->GetSpinBoxList()[0]->value();
    dacThreshold1 = dacs->GetSpinBoxList()[1]->value();
    dacThreshold2 = dacs->GetSpinBoxList()[2]->value();
    dacThreshold3 = dacs->GetSpinBoxList()[3]->value();
    dacThreshold4 = dacs->GetSpinBoxList()[4]->value();
    dacThreshold5 = dacs->GetSpinBoxList()[5]->value();
    dacThreshold6 = dacs->GetSpinBoxList()[6]->value();
    dacThreshold7 = dacs->GetSpinBoxList()[7]->value();
    preamp = dacs->GetSpinBoxList()[8]->value();
    ikrum = dacs->GetSpinBoxList()[9]->value();
    shaper = dacs->GetSpinBoxList()[10]->value();
    disc = dacs->GetSpinBoxList()[11]->value();
    discLs = dacs->GetSpinBoxList()[12]->value();
    shaperTest = dacs->GetSpinBoxList()[13]->value();
    dacDiscL = dacs->GetSpinBoxList()[14]->value();
    dacTest = dacs->GetSpinBoxList()[15]->value();
    dacDiscH = dacs->GetSpinBoxList()[16]->value();
    delay = dacs->GetSpinBoxList()[17]->value();
    tpBuffIn = dacs->GetSpinBoxList()[18]->value();
    tpBuffOut = dacs->GetSpinBoxList()[19]->value();
    rpz = dacs->GetSpinBoxList()[20]->value();
    gnd = dacs->GetSpinBoxList()[21]->value();
    tpRef = dacs->GetSpinBoxList()[22]->value();
    fpk = dacs->GetSpinBoxList()[23]->value();
    cas = dacs->GetSpinBoxList()[24]->value();
    tpRefA = dacs->GetSpinBoxList()[25]->value();
    tpRefB = dacs->GetSpinBoxList()[26]->value();
}

QString FrameHeaderDataStruct::toQString(int frameid) {
    QString header = "MQ1," % QString::number(frameid) % "," % QString::number(dataOffset) % "," %
             QString::number(numberOfChips) %","% QString::number(xDim) %","% QString::number(yDim) %","% pixelDepth
            % "," % sensorLayout % "," % QString::number(chipSelect) % "," % timeStamp % "," %
            QString::number(shutterOpen) % "," % QString::number(counter) % "," % QString::number(colorMode) % "," %
            QString::number(gainMode) % "," % QString::number(threshold0) % "," %QString::number(threshold1) %
            "," % QString::number(threshold2) % "," % QString::number(threshold3) % "," % QString::number(threshold4)
            % "," % QString::number(threshold5) % "," % QString::number(threshold6) % "," % QString::number(threshold7)%","%
            dacFormat % "," % QString::number(dacThreshold0) % "," % QString::number(dacThreshold1)
            % "," % QString::number(dacThreshold2) % "," % QString::number(dacThreshold3) % "," % QString::number(dacThreshold4)
            % "," % QString::number(dacThreshold5)% "," % QString::number(dacThreshold6)% "," % QString::number(dacThreshold7)
            % "," % QString::number(preamp) % "," % QString::number(ikrum) % "," % QString::number(shaper)
            % "," % QString::number(disc) % "," % QString::number(discLs) % "," % QString::number(shaperTest)
            % "," % QString::number(dacDiscL) % "," % QString::number(dacTest) % "," % QString::number(dacDiscH)
            % "," % QString::number(delay) % "," % QString::number(tpBuffIn) % "," % QString::number(tpBuffOut)
            % "," % QString::number(rpz) % "," % QString::number(gnd) % "," % QString::number(tpRef)
            % "," % QString::number(fpk) % "," % QString::number(cas) % "," % QString::number(tpRefA)
            % "," % QString::number(tpRefB);
    return header.leftJustified(dataOffset, ' ', true);
}

/* TODO correct the following text - issue #166 */
QString CommandHandler::getAcquisitionHeader()
{
    Mpx3Config* config = getGui()->getConfig();
    QString len = QString::number(2044+3+2);
    QString file = "HDR,\nChip ID:	" % config->getDeviceWaferId(0) % "," % config->getDeviceWaferId(1) % "," % config->getDeviceWaferId(2) % "," % config->getDeviceWaferId(3) % ","
            % "\nChip Type (Medipix 3.0, Medipix 3.1, Medipix RX):	Medipix3RXAssembly"
            % "\nSize (1X1, 2X2):	   2x2"
            % "\nChip Mode  (SPM, CSM, CM, CSCM):	" % (config->getCsmSpm() == 0 ? "SPM" : "CSM")
            % "\nCounter Depth (number):	" % QString::number(config->getPixelDepth())
            % "\nGain:	" % config->getGainModeString()
            % "\nActive Counters:	Counter 0" % (config -> getReadBothCounters() ? ", Counter 1" : "")
            % "\nThresholds (keV):	0.000000E+0,1.000000E+1,1.500000E+1,2.000000E+1,2.500000E+1,3.000000E+1,3.500000E+1,4.000000E+1"
            % "\nDACs:	030,056,083,111,139,167,194,222,100,010,125,125,100,100,080,100,090,050,128,004,255,148,128,203,189,417,417; 030,056,083,111,139,167,194,222,100,010,125,125,100,100,080,100,090,050,128,004,255,142,128,192,180,417,417; 030,056,083,111,139,167,194,222,100,010,125,125,100,100,080,100,090,050,128,004,255,151,128,205,191,417,417; 030,056,083,111,139,167,194,222,100,010,125,125,100,100,080,100,090,050,128,004,255,138,128,189,181,417,417"
            % "\nbpc File:	c:\\MERLIN Quad Host\\Config\\W117_E7\\W117_E7_SPM.bpc,c:\\MERLIN Quad Host\\Config\\W117_H7\\W117_H7_SPM.bpc,c:\\MERLIN Quad Host\\Config\\W117_I7\\W117_I7_SPM.bpc,c:\\MERLIN Quad Host\\Config\\W117_G7\\W117_G7_SPM.bpc"
            % "\nDAC File:	c:\\MERLIN Quad Host\\Config\\W117_E7\\W117_E7_SPM.dacs,c:\\MERLIN Quad Host\\Config\\W117_H7\\W117_H7_SPM.dacs,c:\\MERLIN Quad Host\\Config\\W117_I7\\W117_I7_SPM.dacs,c:\\MERLIN Quad Host\\Config\\W117_G7\\W117_G7_SPM.dacs"
            % "\nGap Fill Mode:	None"
            % "\nFlat Field File:	Dummy (C:\\Temp.ffc)"
            % "\nDead Time File:	Dummy (C:\\<NUL>\\Temp.dtc)"
            % "\nAcquisition Type (Normal, Th_scan, Config):	Normal"
            % "\nFrames in Acquisition (Number):  " % QString::number(config->getNTriggers())
            % "\nTrigger Start (Positive, Negative, Internal):	Internal"
            % "\nTrigger Stop (Positive, Negative, Internal):	Internal"
            % "\nFrames per Trigger (Number):	1"
            % "\nTime and Date Stamp (yr, mnth, day, hr, min, s):	" % QDateTime::currentDateTime().toString(Qt::ISODate)
            % "\nSensor Bias (V, µA)	" % QString::number(config->getBiasVoltage())
            % "\nSensor Polarity (Positive, Negative):	" % config->getPolarityString()
            % "\nTemperature (C):	FPGA Temp 37.250000 Deg C"
            % "\nMedipix Clock (MHz):	" % QString::number(config->getSystemClock())
            % "\nReadout System:	Cheetah 1800"
            % "\nSoftware Version:	" % _softwareVersion
            % "\nEnd\n";
    QString acqHeader = "MPX,0000002048," + file.leftJustified(2048, ' ', true);
    return acqHeader;
}

void CommandHandler::getImage()
{
    /* TODO Pretty sure this is wrong... You connect a new signal every time this function is called? */
    connect(QCstmGLVisualization::getInstance()->getDataConsumerThread(), SIGNAL(doneWithOneFrame(int)), this, SLOT(on_doneWithOneFrame(int)));

//    int nFrames = Mpx3GUI::getInstance()->getConfig()->getNTriggers();
//    int frameCounter = 0;
//    int size = 5;
//    QString len = "";

//    while(frameCounter <nFrames){

//        FrameHeaderDataStruct frameHeader;
//        QByteArray ba;
//        QString hd = generateMerlinFrameHeader(frameHeader);
//        QByteArray frame = Mpx3GUI::getInstance()->getDataset()->toSocketData();
//        size = hd.length();
//        size += frame.length();
//        len = QString::number(size);

//        QString zeros ="";
//        for (int i = 0; i < 10 - len.length(); ++i) {
//            zeros += "0";
//        }
//        len = zeros + len;
//        QString firstPart = "MPX,"+ len + ",MQ1," + hd;
//        ba += firstPart.toLatin1();
//        ba += frame;
//        emit imageIsReady(ba);
//        frameCounter++;

//    }
//    FrameHeaderDataStruct frameHeader;
//    QByteArray ba = generateMerlinFrameHeader(frameHeader).toLatin1().data();
//    ba += Mpx3GUI::getInstance()->getDataset()->toSocketData();
//    commandIsDecoded("",ba,true);
    return;
}

int CommandHandler::setPixelMask(int x, int y)
{
    emit requestToMaskPixelRemotely(x,y);

    //! TODO return an int?
}

int CommandHandler::setPixelUnmask(int x , int y)
{
    emit requestToUnmaskPixelRemotely(x,y);

    //! TODO return an int?
}

int CommandHandler::loadEqualizationRemotely(QString path)
{
    QDir dir(path);
    if(!dir.exists()){
        return UNKNOWN_ERROR;
    }
    emit requestToLoadEqualizationRemotely(path);
    return NO_ERROR;
}

QString CommandHandler::getEqualizationPath()
{
    return _equalizationPath;
}

int CommandHandler::loadConfigRemotely(QString path)
{

    QFileInfo file(path);
    if(!file.exists() || path.split(".").last() != "json" ){
        return UNKNOWN_ERROR;
    }

    emit requestToLoadConfigRemotely(path);
    return NO_ERROR;
}

QString CommandHandler::getConfigPath()
{
    return getGui()->getConfigPath();
}

int CommandHandler::saveConfigRemotely(QString path)
{
    QStringList pathList = path.split(QDir::separator());
    QString filename = pathList.last();
    //construct the path directory again
    path.clear();
    for (int i = 0; i < pathList.length() - 1; ++i) {
         path += pathList.at(i) + QDir::separator();
    }

    qDebug() << "filename : " << filename;
    qDebug() << "path : " << path;
    QDir dir(path);
    if(!dir.exists() || filename.split(".").last() != "json"){
        return UNKNOWN_ERROR;
    }
    emit requestToSaveConfigRemotely(path + filename);
    return NO_ERROR;
}

int CommandHandler::doEqualizationRemotely(QString path)
{
    QDir dir(path);
    if (!dir.exists())
    {
       return UNKNOWN_ERROR;
    }
    emit requestToDoEqualizationRemotely(path);
    return NO_ERROR;
}

int CommandHandler::setInhibitShutter(bool turnOn)
{
    connect(this,SIGNAL(requestToSetInhibitShutterRemotely(bool)), getGui()->getConfig(), SLOT(setInhibitShutter(bool)));
    emit requestToSetInhibitShutterRemotely(turnOn);
    return NO_ERROR;
}

bool CommandHandler::getInhibitShutter()
{
    int val = 0;
    getGui()->GetSpidrController()->getSpidrReg(0x0810, &val);

    if((val & (0xF << 4)) == (0X8 << 4))
    {
        return true;
    }
    return false;
}

int CommandHandler::setSlope(int chipNum, double val)
{

    if(chipNum < 0 || chipNum >= NUMBER_OF_CHIPS)
        return ARG_VAL_OUT_RANGE;
    getGui()->getGeneralSettings()->setSlope(chipNum, val);
    getGui()->updateEnergyCalibratorParameters();
    return NO_ERROR;
}

double CommandHandler::getSlope(int chipNum)
{
    return getGui()->getGeneralSettings()->getSlope(chipNum);
}

int CommandHandler::setOffset(int chipNum, double val)
{
    if(chipNum < 0 || chipNum >= NUMBER_OF_CHIPS)
        return ARG_VAL_OUT_RANGE;
    getGui()->getGeneralSettings()->setOffset(chipNum, val);
    getGui()->updateEnergyCalibratorParameters();
    return NO_ERROR;

}

double CommandHandler::getOffset(int chipNum)
{
    return getGui()->getGeneralSettings()->getOffset(chipNum);
}

int CommandHandler::resetSlopesAndOffsets()
{
    //this means energy = dac
    for(int i = 0; i < NUMBER_OF_CHIPS; i++){
        getGui()->getGeneralSettings()->setOffset(i, 0);
        getGui()->getGeneralSettings()->setSlope(i, 1);
    }
    getGui()->updateEnergyCalibratorParameters();
    return NO_ERROR;
}



void CommandHandler::on_doneWithOneFrame(int frameid)
{
    FrameHeaderDataStruct frameHeader(getGui());
    auto dataset = getGui()->getDataset();
    for (int threshold = 0; threshold < 8; threshold++) {
        std::pair<const char*, int> frame = dataset->toCanvas(threshold);
        auto size = frame.second;
        if (size == 0) continue;

        frameHeader.counter = threshold;
        QString hd = frameHeader.toQString(frameid);
        size += hd.length();
        auto len = QString("%1").arg(size, 10, 10, QChar('0'));
        QString firstPart = "MPX," + len + "," + hd;
        emit imageIsReady(firstPart.toLatin1(), frame);
    }
}

void CommandHandler::on_someCommandHasFinished_Successfully()
{
    disconnect(QCstmGLVisualization::getInstance()->getDataConsumerThread(),SIGNAL(doneWithOneFrame(int)),this,SLOT(on_doneWithOneFrame(int)));
}

void CommandHandler::on_equalizationPathExported(QString path)
{
    _equalizationPath = path;
}

void CommandHandler::emitrequestForAnotherSocket(int port)
{
    emit requestAnotherSocket(port);
}

QString Command::getData()
{
    return data;
}

void Command::invoke(CommandHandler *ch, SERVER_BUSY_TYPE serverStatus)
{
    qDebug() << "debug cmd :" << cmd;
    if(ch->cmdTable.contains(cmd))
    {
        if(serverStatus == SB_DATA_TAKING && cmd != "Stop"){
            qDebug () << "Server is busy." << serverStatus;
            setError(SERVER_BUSY_DATA_TAKING);
            return;
        }
        if(serverStatus == SB_THRESHOLD_SCAN && cmd != "StarStoptScan"){
            qDebug () << "Server is busy." << serverStatus;
            setError(SERVER_BUSY_THRESHOLD_SCAN);
            return;
        }
        if(serverStatus == SB_DAC_SCAN){
            qDebug () << "Server is busy." << serverStatus;
            setError(SERVER_BUSY_DAC_SCAN);
            return;
        }
        ch->cmdTable[cmd].handler(ch, this);
    }
    else
    {
        data = cmd; //"Command does not exist...!";
        merlinErrorToPslError(cmd.toInt());
        qDebug()<<"No Matched...."<<cmd.toInt();
       // emit commandIsDecoded(data,nullptr,false);
        return;
    }
   // if(cmd == "GetImage")
        //emit commandIsDecoded(data,nullptr,true);
    //else
        //emit commandIsDecoded(data,nullptr,false);
}

Command::Command(QString command)
{
   arguments.clear();
   QStringList cmdList = command.split(";");
   cmd = cmdList.at(0); //core command

   for(int i=1; i<cmdList.size(); i++){
       QString str = cmdList.at(i);

       //! Tolerate new lines sent from netcat
       QStringList list = str.split(QRegularExpression("(\\n)"));
       arguments.push_back(list.join(""));
   }
}

void Command::setData(QString d)
{
    data = d;
}

void Command::setImage(QByteArray im)
{
    imageToSend = im;
}

void Command::print() //! Consider changing this function name...
{
    qDebug() << "Core command: " << cmd;
    for(int i=0; i<arguments.size(); i++){
        qDebug() << "argument: " << arguments.at(i);
    }
}
