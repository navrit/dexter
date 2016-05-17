#ifndef MTADIALOG_H
#define MTADIALOG_H

#include <QDialog>

#include "mpx3gui.h"

namespace Ui {
class MTADialog;
}

class MTADialog : public QDialog
{
    Q_OBJECT

public:
    explicit MTADialog(Mpx3GUI *, QWidget *parent = 0);
    ~MTADialog();

    void timerEvent( QTimerEvent * );

private:
    Mpx3GUI * _mpx3gui;
    Ui::MTADialog * ui;


};

#endif // MTADIALOG_H
