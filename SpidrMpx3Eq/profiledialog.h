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
    void setPixels(QPoint pixel_begin, QPoint pixel_end){_begin = pixel_begin; _end = pixel_end;}
    void setAxisMap(QMap<int,int> Axismap){_Axismap = Axismap;}
    void changeTitle(QString axis);
    void plotProfile(QString axis);


private:
    Ui::ProfileDialog *ui;
    Mpx3GUI * _mpx3gui;
    QPoint _begin; //! The coordinates of the pixel where the selected region begins.
    QPoint _end; //! The coordinates of the pixel where the selected region ends.
    QMap<int, int> _Axismap; //! Contains a total pixelvalue for each X or Y value in the selected profile region.

    //Functions:
    void addMeanLines(QString data);
    void changeText(QString text);

private slots:
    void on_buttonBox_accepted();

    void on_checkBox_toggled(bool checked);

    void on_pushButton_clicked();

    void on_comboBox_currentIndexChanged(int index);

    void on_lineEdit_editingFinished();

    void on_lineEdit_2_editingFinished();

    void on_lineEdit_3_editingFinished();

    void on_lineEdit_4_editingFinished();

signals:
    void user_accepted_profile();

};

#endif // PROFILEDIALOG_H
