#ifndef PROFILEDIALOG_H
#define PROFILEDIALOG_H

#include <QDialog>
#include "mpx3gui.h"

class QCPGraph;

namespace Ui {
class ProfileDialog;
}

class ProfileDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ProfileDialog(QWidget *parent = 0);
    ~ProfileDialog();
    void SetMpx3GUI(Mpx3GUI * p);
    void changeText(QString axis, QPoint pixel_begin, QPoint pixel_end);
    void plotProfileX(QPoint pixel_begin, QPoint pixel_end, QMap<int, int> Xmap);
    void plotProfileY(QPoint pixel_begin, QPoint pixel_end, QMap<int, int> Ymap);

private:
    Ui::ProfileDialog *ui;
    Mpx3GUI * _mpx3gui;

private slots:
    void on_buttonBox_accepted();

signals:
    void user_accepted_profile();
};

#endif // PROFILEDIALOG_H
