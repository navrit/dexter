#ifndef TESTPULSES_H
#define TESTPULSES_H

#include <QDialog>

#include "mpx3gui.h"

namespace Ui {
class TestPulses;
}

class TestPulses : public QDialog
{
    Q_OBJECT

public:
    explicit TestPulses(Mpx3GUI *, QWidget *parent = 0);
    ~TestPulses();

private slots:
    void on_activateCheckBox_clicked(bool checked);

    void on_pushButton_setLength_clicked();

    void on_pushButton_setPeriod_clicked();

    void on_pushButton_setIDELAY_clicked();

private:
    Mpx3GUI * _mpx3gui = nullptr;
    Ui::TestPulses *ui = nullptr;
};

#endif // TESTPULSES_H
