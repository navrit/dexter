#ifndef HDMICONFIG_H
#define HDMICONFIG_H

#include <QWidget>
#include <QComboBox>
#include <QVector>
#include <QHash>

namespace Ui {
class hdmiConfig;
}

class hdmiConfig : public QWidget
{
    Q_OBJECT

public:
    explicit hdmiConfig(QWidget *parent = 0);
    ~hdmiConfig();

private slots:
    void on_hdmi1Pin1ComboBox_currentIndexChanged(int index);

    void on_hdmi1Pin2ComboBox_currentIndexChanged(int index);

    void on_hdmi1Pin3ComboBox_currentIndexChanged(int index);

    void on_hdmi2Pin1ComboBox_currentIndexChanged(int index);

    void on_hdmi2Pin2ComboBox_currentIndexChanged(int index);

    void on_hdmi2Pin3ComboBox_currentIndexChanged(int index);

    void on_submitPb_released();

    void ConnectionStatusChanged(bool con);

private:
    Ui::hdmiConfig *ui;
    QVector <QComboBox*> _comboBoxes;
    void _loadComboBoxesVector(void);
    void _configComboxItems(int itemIndex, QComboBox *excludeComboBox);
    unsigned _comboBoxIndexToHdmiCode(int comboBoxIndex);
    void _setHdmiRegister(unsigned code,unsigned shift);
    //combo box index for HDMI input
    static const int INPUT_INDEX_START = 1;
    static const int INPUT_INDEX_END  = 2;//could be expanded
    static const int _inputItemsTableSize = INPUT_INDEX_END - INPUT_INDEX_START + 1;
    //comboboxes enable/disable flags
    static const int notSelected  = 0;
    static const int hdmi1Pin1ComboBoxIndex = 1;
    static const int hdmi1Pin2ComboBoxIndex = 2;
    static const int hdmi1Pin3ComboBoxIndex = 3;
    static const int hdmi2Pin1ComboBoxIndex = 4;
    static const int hdmi2Pin2ComboBoxIndex = 5;
    static const int hdmi2Pin3ComboBoxIndex = 6;
    //hdmi register position (shift)
    static const unsigned hdmi1Pin1RegisterPosition = 0;
    static const unsigned hdmi1Pin2RegisterPosition = 1;
    static const unsigned hdmi1Pin3RegisterPosition = 2;
    static const unsigned hdmi2Pin1RegisterPosition = 3;
    static const unsigned hdmi2Pin2RegisterPosition = 4;
    static const unsigned hdmi2Pin3RegisterPosition = 5;
    //hdmi I/O codes
    static const unsigned externalShutterIn = 0x5;
    static const unsigned inhibitShutterInOn = 0x8;
    static const unsigned inhibitShutterInOff = 0xF;
    static const unsigned shutter_out = 0x4;
    static const unsigned inhibitShutterAndShutterOut = 0xA;
    static const unsigned counterSelectOut = 0xB;

    //hdmi config register;
    unsigned _hdmiRegisterArray[6] = {0};
    unsigned _hdmiRegister = 0;

    int _inputItemsTable[_inputItemsTableSize];

};

#endif // HDMICONFIG_H
