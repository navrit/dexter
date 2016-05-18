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

    void changePlotsProperties();

private slots:

    void on_radioButtonSelCounts_toggled(bool checked);

    void on_radioButtonSelMean_toggled(bool checked);

    void on_radioButtonSelStdv_toggled(bool checked);

    void on_radioButtonSelPixelsON_toggled(bool checked);

    void on_radioButtonSelNumberOfClusters_toggled(bool checked);

    void on_barCharLogYCheckBox_clicked(bool checked);

    void on_timePlotLogYCheckBox_clicked(bool checked);

private:

    Mpx3GUI * _mpx3gui = nullptr;
    Ui::MTADialog * ui = nullptr;

    typedef enum {
        __counts = 0,
        __mean,
        __stdv,
        __pixelsON,
        __NofClusters,
        __numberOfModes
    } display_mode;

    // modes
    display_mode _displayMode = __pixelsON;
    bool _barCharLogY  = false;
    bool _timePlotLogY = false;

};

#endif // MTADIALOG_H
