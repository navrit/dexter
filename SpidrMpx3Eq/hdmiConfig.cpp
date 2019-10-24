#include "hdmiConfig.h"
#include "ui_hdmiConfig.h"
#include <QStandardItemModel>
#include <QDebug>
#include "mpx3gui.h"

hdmiConfig::hdmiConfig(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::hdmiConfig)
{
    ui->setupUi(this);
    _loadComboBoxesVector();
    for (int i = 0; i < _inputItemsTableSize ; i++) {
        _inputItemsTable[i] = notSelected;
    }
}

hdmiConfig::~hdmiConfig()
{
    delete ui;
}

void hdmiConfig::setWindowWidgetsStatus(win_status s)
{
    switch (s) {

        case win_status::startup:
            this->setEnabled( false );
            break;

        case win_status::connected:
            this->setEnabled( true );
            break;

        default:
            break;
        }
}

void hdmiConfig::_loadComboBoxesVector()
{
    _comboBoxes.push_back(ui->hdmi1Pin1ComboBox);
    _comboBoxes.push_back(ui->hdmi1Pin2ComboBox);
    _comboBoxes.push_back(ui->hdmi1Pin3ComboBox);
    _comboBoxes.push_back(ui->hdmi2Pin1ComboBox);
    _comboBoxes.push_back(ui->hdmi2Pin2ComboBox);
    _comboBoxes.push_back(ui->hdmi2Pin3ComboBox);
}

void hdmiConfig::_configComboxItems(int itemIndex, QComboBox *excludeComboBox)
{

    if(itemIndex < INPUT_INDEX_START && itemIndex > INPUT_INDEX_END)
        return;

    foreach (QComboBox* comboBox, _comboBoxes) {
        if(comboBox == excludeComboBox)
            continue;
        QStandardItemModel *model = qobject_cast<QStandardItemModel *>(comboBox->model());
        Q_ASSERT(model != nullptr);
        for(int i = 0; i < _inputItemsTableSize; i++) {
            if(_inputItemsTable[i] != 0)
            {
                QStandardItem *item = model->item(i+1);
                item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
            }
            else
            {
                QStandardItem *item = model->item(i+1);
                item->setFlags(item->flags() | Qt::ItemIsEnabled);
            }
        }
    }
}

unsigned hdmiConfig::_comboBoxIndexToHdmiCode(int comboBoxIndex)
{
    switch (comboBoxIndex) {
    case 1:
        return externalShutterIn;
    case 2:
        if(Mpx3GUI::getInstance()->getConfig()->isInhibitShutterSelected())
            return inhibitShutterInOn;
        else
            return inhibitShutterInOff;
    case 3:
        return shutter_out;
    case 4:
        return inhibitShutterAndShutterOut;
    case 5:
        return counterSelectOut;
    default:
        return 0;
    }
}

void hdmiConfig::_setHdmiRegister(unsigned code, unsigned shift)
{
    if(shift < 6 )
        _hdmiRegisterArray[shift] = code;
    if(code == inhibitShutterInOff || code == inhibitShutterInOn)
        Mpx3GUI::getInstance()->getConfig()->onSetInhibitShutterRegisterOffset(int(shift*4));
}

void hdmiConfig::on_hdmi1Pin1ComboBox_currentIndexChanged(int index)
{
    for (int i = 0; i < _inputItemsTableSize; ++i) {
        if(_inputItemsTable[i] == hdmi1Pin1ComboBoxIndex) //deselect
            _inputItemsTable[i] = notSelected;
        if(i + 1 == index) //select
            _inputItemsTable[i] = hdmi1Pin1ComboBoxIndex;
    }
    _configComboxItems(index,ui->hdmi1Pin1ComboBox);
    _setHdmiRegister(_comboBoxIndexToHdmiCode(index),hdmi1Pin1RegisterPosition);
}

void hdmiConfig::on_hdmi1Pin2ComboBox_currentIndexChanged(int index)
{
    for (int i = 0; i < _inputItemsTableSize; ++i) {
        if(_inputItemsTable[i] == hdmi1Pin2ComboBoxIndex) //deselect
            _inputItemsTable[i] = notSelected;
        if(i + 1 == index) //select
            _inputItemsTable[i] = hdmi1Pin2ComboBoxIndex;
    }
    _configComboxItems(index,ui->hdmi1Pin2ComboBox);
    _setHdmiRegister(_comboBoxIndexToHdmiCode(index),hdmi1Pin2RegisterPosition);
}

void hdmiConfig::on_hdmi1Pin3ComboBox_currentIndexChanged(int index)
{
    for (int i = 0; i < _inputItemsTableSize; ++i) {
        if(_inputItemsTable[i] == hdmi1Pin3ComboBoxIndex) //deselect
            _inputItemsTable[i] = notSelected;
        if(i + 1 == index) //select
            _inputItemsTable[i] = hdmi1Pin3ComboBoxIndex;
    }
    _configComboxItems(index,ui->hdmi1Pin3ComboBox);
    _setHdmiRegister(_comboBoxIndexToHdmiCode(index),hdmi1Pin3RegisterPosition);
}

void hdmiConfig::on_hdmi2Pin1ComboBox_currentIndexChanged(int index)
{
    for (int i = 0; i < _inputItemsTableSize; ++i) {
        if(_inputItemsTable[i] == hdmi2Pin1ComboBoxIndex) //deselect
            _inputItemsTable[i] = notSelected;
        if(i + 1 == index) //select
            _inputItemsTable[i] = hdmi2Pin1ComboBoxIndex;
    }
    _configComboxItems(index,ui->hdmi2Pin1ComboBox);
    _setHdmiRegister(_comboBoxIndexToHdmiCode(index),hdmi2Pin1RegisterPosition);
}

void hdmiConfig::on_hdmi2Pin2ComboBox_currentIndexChanged(int index)
{
    for (int i = 0; i < _inputItemsTableSize; ++i) {
        if(_inputItemsTable[i] == hdmi2Pin2ComboBoxIndex) //deselect
            _inputItemsTable[i] = notSelected;
        if(i + 1 == index) //select
            _inputItemsTable[i] = hdmi2Pin2ComboBoxIndex;
    }
    _configComboxItems(index,ui->hdmi2Pin2ComboBox);
    _setHdmiRegister(_comboBoxIndexToHdmiCode(index),hdmi2Pin2RegisterPosition);
}

void hdmiConfig::on_hdmi2Pin3ComboBox_currentIndexChanged(int index)
{
    for (int i = 0; i < _inputItemsTableSize; ++i) {
        if(_inputItemsTable[i] == hdmi2Pin3ComboBoxIndex) //deselect
            _inputItemsTable[i] = notSelected;
        if(i + 1 == index) //select
            _inputItemsTable[i] = hdmi2Pin3ComboBoxIndex;
    }
    _configComboxItems(index,ui->hdmi2Pin3ComboBox);
    _setHdmiRegister(_comboBoxIndexToHdmiCode(index),hdmi2Pin3RegisterPosition);
}

void hdmiConfig::on_submitPb_released()
{
    _hdmiRegister = 0;
    bool inhibitShutterFound = false;
    for (int i = 0; i < 6; ++i) {
        if(_hdmiRegisterArray[i] == inhibitShutterInOff || _hdmiRegisterArray[i] == inhibitShutterInOn){
            inhibitShutterFound = true;
            if(Mpx3GUI::getInstance()->getConfig()->isInhibitShutterSelected())
                _hdmiRegister |= inhibitShutterInOn << (i*4);
            else
                _hdmiRegister |= inhibitShutterInOff << (i*4);
            continue;
        }
        _hdmiRegister |= _hdmiRegisterArray[i] << (i*4);
    }

    if(!inhibitShutterFound)
        Mpx3GUI::getInstance()->getConfig()->onSetInhibitShutterRegisterOffset(-1);

    qDebug().noquote() << "[INFO]\tHDMI config : " << QString::number(_hdmiRegister, 2);
    Mpx3GUI::getInstance()->GetSpidrController()->setSpidrReg(0x0810, int(_hdmiRegister), true);
    Mpx3GUI::getInstance()->getConfig()->setHdmiRegisterValue(int(_hdmiRegister));
}

void hdmiConfig::ConnectionStatusChanged(bool con)
{
    this->setEnabled(con);
}
