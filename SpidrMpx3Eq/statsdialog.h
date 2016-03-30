#ifndef STATSDIALOG_H
#define STATSDIALOG_H

#include <QDialog>
#include "mpx3gui.h"
#include <QString> //A

namespace Ui {
class StatsDialog;
}

class StatsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit StatsDialog(QWidget *parent = 0);
    ~StatsDialog();
    void SetMpx3GUI(Mpx3GUI * p);
    void changeText();

private slots:
    void on_buttonBox_accepted();

private:
    Ui::StatsDialog *ui;
    Mpx3GUI * _mpx3gui;

signals:
    void user_accepted_stats();

};

#endif // STATSDIALOG_H
