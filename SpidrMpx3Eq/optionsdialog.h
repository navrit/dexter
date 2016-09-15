#ifndef OPTIONSDIALOG_H
#define OPTIONSDIALOG_H

#include <QDialog>
#include <QAbstractButton>
#include "mpx3gui.h"

namespace Ui {
class optionsDialog;
}

class optionsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit optionsDialog(QWidget *parent = 0);
    ~optionsDialog();
    void setCurrentSettings();
    void SetMpx3GUI(Mpx3GUI *p);
    void setDataRange(int range){_datarange = range;}

private:
    Ui::optionsDialog *ui;
    Mpx3GUI *_mpx3gui;
    QHash<QString, int> _currentSettings;
    int _datarange;
    void resetSettings();

private slots:
    void on_buttonBox_clicked(QAbstractButton *button);

    void on_fitComboBox_currentIndexChanged(const QString &arg1);

    void on_windowLineEdit_editingFinished();

    void on_manualRadioButton_toggled(bool checked);

    void on_roiXsizeLineEdit_editingFinished();

    void on_roiYsizeLineEdit_editingFinished();

    void on_selectedRoIRadioButton_toggled(bool checked);

signals:
    void close_optionsDialog();
    void apply_options(QHash<QString, int>);

};

#endif // OPTIONSDIALOG_H
