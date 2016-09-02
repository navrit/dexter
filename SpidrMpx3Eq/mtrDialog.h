#ifndef MTRDialog_H
#define MTRDialog_H

#include <QDialog>
#include <QVector>

#include "mpx3gui.h"

namespace Ui {
class MTRDialog;
}

class MTRDialog : public QDialog
{
    Q_OBJECT

public:
    explicit MTRDialog(Mpx3GUI *, QWidget *parent = 0);
    ~MTRDialog();

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
    Ui::MTRDialog * ui = nullptr;
    QVector<QLCDNumber *> _lcds;
    QVector<QLabel *> _labels;

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

#endif // MTRDialog_H
