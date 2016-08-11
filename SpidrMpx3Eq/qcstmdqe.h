#ifndef QCSTMDQE_H
#define QCSTMDQE_H

#include <QWidget>
#include "mpx3gui.h"
#include "ui_qcstmdqe.h"
#include "qcstmglvisualization.h"

namespace Ui {
class QCstmDQE;
}

//Necessary datatypes for dlib.
typedef dlib::matrix<double,2,1> input_vector;
typedef dlib::matrix<double,3,1> parameter_vector;

//prototypes:
double model(const input_vector &input, const parameter_vector &params);
double residual(const std::pair<input_vector, double> &data, const parameter_vector &params);


class QCstmDQE : public QWidget
{
    Q_OBJECT

public:
    explicit QCstmDQE(QWidget *parent = 0);
    ~QCstmDQE();
    //Ui::QCstmDQE * GetUI(){ return ui; };
    void SetMpx3GUI(Mpx3GUI *p);
    void setSelectedThreshold(int threshold){ ui->comboBox->setCurrentText(QString("Threshold %1").arg(threshold)); }
    //void updateSelectedThreshold(){ui->comboBox->setCurrentText( QString("Threshold %1").arg(_mpx3gui->getVisualization()->getActiveThreshold()) ); }
    void setRegion(QPoint pixel_begin, QPoint pixel_end);
    void plotESF();
    //void setParams(parameter_vector params){_params = params;}
    //void setxStart(double start){_xstart = start;}
    //void setPlotLength(int length){_plotrange = length;}
    void clearDataAndPlots();
    void refreshLog(bool emptylog){ if(emptylog)ui->textBrowser->clear(); else ui->textBrowser->setText(_logtext);}

private slots:
    void on_takeDataPushButton_clicked();

    void on_comboBox_currentIndexChanged(const QString &arg1);

//    void on_fitESFpushButton_clicked();

//    void on_fitLSFpushButton_clicked();

    void on_loadDataPushButton_clicked();

    void on_listWidget_currentRowChanged(int currentRow);

    void on_removeDataFilePushButton_clicked();

    void on_clearDataFilesPushButton_clicked();

    void on_mtfPushButton_clicked();

    void on_saveMTFpushButton_clicked();

    void on_logClearPushButton_clicked();

    void on_logSavePushButton_clicked();

    void on_binSizeLineEdit_editingFinished();

private:
    Ui::QCstmDQE *ui;
    Mpx3GUI * _mpx3gui;
    //int _currentThreshold;
    QPoint _beginpix, _endpix;
    QVector<QVector<double> > _ESFdata; //Contains a vector for the distances and one for the pixel values of the esf data.
    QVector<QVector<double> > _ESFbinData; //Necessary?? //Contains a vector for the middle of distance bins and one for the mean pixel values of the esf data.
    QVector<QVector<double> > _LSFdata;
    QVector<QVector<double> > _MTFdata;
    parameter_vector _params;   //!Contains the parameters of the error function used for the fitting.
    double _xstart;
    double _plotrange;

    QStringList _NPSfilepaths;
    QString _logtext;   //The text that is to appear in the log window.

    //Options
    bool _useDerFit = true; //! Indicates whether the LSF should be made of the theoretical derivative using the calculated parameters (true) or the numerical derivative of the (smoothed) binned data.
    double _binsize = 1;    //!Specifies the size of the bins to be used for the ESF data.
    double _stepsize = 0.1;   //!Specifies the distance between datapoints of the fitplot in pixels.
    double _histStep = 0.5;   //!Specifies the distance between datapoints in the LSF (in pixels)

    //functions:
    QVector<QVector<double> > calcESFbinData();
    QVector<QVector<double> > calcESFfitData();
    QVector<QVector<double> > calcLSFfromFitdata();
    QVector<QVector<double> > calcNumDerivativeOfdata(QVector<QVector<double> > data);
    QVector<QVector<double> > calcMTFdata();
    void plotMTF();

    void plotFitESF();
    void plotLSF();
    void plotEdge(QPoint ab);    
    void fitESFparams(QVector<QVector<double> > esfdata);

    QString dataToString(QVector<QVector<double> > data);    
    double FivePointsStencil(QVector<double> func, int x, double bw);


signals:
    void start_takingData();
    void open_data(bool, bool, QString);

};

#endif // QCSTMDQE_H
