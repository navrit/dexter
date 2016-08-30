#ifndef PROFILEDIALOG_H
#define PROFILEDIALOG_H

#include <QDialog>
#include "mpx3gui.h"
#include "ui_profiledialog.h"

class QCPGraph;

namespace Ui {
class ProfileDialog;
}

class ProfileDialog : public QDialog
{
    Q_OBJECT

    enum constants{
        kdf_index = 1,
        N_maingraphs = 2, //The profile itself and the kernel density function.
        N_meangraphs = 3,
        N_left = 2  //number of fields/points a signal has to be shifted when on the left or right.
    };

public:
    explicit ProfileDialog(QWidget *parent = 0);
    ~ProfileDialog();
    void SetMpx3GUI(Mpx3GUI * p);
    void setPixels(QPoint pixel_begin, QPoint pixel_end){_begin = pixel_begin; _end = pixel_end;}
    void setAxisMap(QMap<int,int> Axismap){_Axismap = Axismap;}
    void setAxis(QString axis){
        _axis = axis;
        if (_axis=="X" ) ui->select_xy->setCurrentIndex(0);
        if (_axis=="Y" ) ui->select_xy->setCurrentIndex(1);

    }
    void changeTitle();
    void plotProfile();
    void setLayer(int layerIndex) { ui->comboBox->setCurrentText(QString("Threshold %1").arg(layerIndex));}


private:
    Ui::ProfileDialog *ui;
    Mpx3GUI * _mpx3gui;
    QPoint _begin; //! The coordinates of the pixel where the selected region begins.
    QPoint _end; //! The coordinates of the pixel where the selected region ends.
    QMap<int, int> _Axismap; //! Contains a total pixelvalue for each X or Y value in the selected profile region.
    QString _axis; //!The axis that is currently used as the horizontal axis in the profileplot.
    QList<QLineEdit*> editsList; //!Contains the QLineEdits that specify the points on the profile for CNR calculation.
    bool _left = true; //!Indicates whether a left background is present.
    bool _right = true; //!Indicates whether a right background is present.
    QVector<double> par_v;//!Vector containing parameters for the kernel density function.

    //Functions:
    void addMeanLines(QString data);
    void changeText(QString text);
    bool valueinRange(int value);
    void makeEditsList();
    void useKernelDensityFunction(double bandwidth);
    void createKernelDensityFunction(int Npoints, QVector<double> hist, double bandwidth);
    double GausFuncAdd(double x, QVector<double> par);
    QVector<double> calcPoints(QVector<double> function, int Nder, int bw);
    double FivePointsStencil(QVector<double> func, int x, double bw);
    double secderFivePointsStencil(QVector<double> func, int x, double bw);
    void setInflectionPoints(QVector<int> infls, int begin, int Npoints, int maxpt);
    int farthestFromAt(QVector<int> list, int x);
    int closestToAt(QVector<int> list, int x);

private slots:
    void on_buttonBox_accepted();

    void on_checkBox_toggled(bool checked);

    void on_CNRbutton_clicked();

    void on_KDFbutton_clicked();

    void onpointEdit_editingFinished(); //without _ to break pattern searching, object name changed.. to editlist[i]

    void on_comboBox_currentIndexChanged(const QString &arg1);


    //Eventhandlers:
    void mousePressEvent(QMouseEvent *event);
    void closeEvent(QCloseEvent *event);


    void on_clearbutton_clicked();

    void on_checkBox_left_toggled(bool checked);

    void on_checkBox_right_toggled(bool checked);

    void on_select_xy_currentIndexChanged(int index);

signals:
    void user_accepted_profile();

};

#endif // PROFILEDIALOG_H
