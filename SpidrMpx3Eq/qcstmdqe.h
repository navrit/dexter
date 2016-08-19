#ifndef QCSTMDQE_H
#define QCSTMDQE_H

#include <QWidget>
#include "mpx3gui.h"
#include "ui_qcstmdqe.h"
#include "qcstmglvisualization.h"
#include <QtDataVisualization>

namespace Ui {
class QCstmDQE;
}


typedef dlib::matrix<double,2,1> input_vector;      //!Necessary datatype for dlib algorithm, contains the variables going into the fitting model function.
typedef dlib::matrix<double,5,1> parameter_vector;  //!Necessary datatype for dlib algorithm, contains the parameters used for fitting. 3 for error function, 5 for 4th order poly

//prototypes:
double model(const input_vector &input, const parameter_vector &params);    //!The function model that is to be fitted to the data.
double residual(const std::pair<input_vector, double> &data, const parameter_vector &params);   //!Calculates the residual, difference between the data value and model value.
double planeModel(const input_vector &input, const parameter_vector &params);    //!The function model of a plane that is to be fitted to the data.
double planeResidual(const std::pair<input_vector, double> &data, const parameter_vector &params);   //!Calculates the residual, difference between the data value and plane model value.
double polyModel(const input_vector &input, const parameter_vector &params);    //!The function model of a 4th order polynomial that is to be fitted to the data within a window.
double polyResidual(const std::pair<input_vector, double> &data, const parameter_vector &params);   //!Calculates the residual, difference between the data value and polynomial model value.


class QCstmDQE : public QWidget
{
    Q_OBJECT

public:
    explicit QCstmDQE(QWidget *parent = 0);
    ~QCstmDQE();
    //Ui::QCstmDQE * GetUI(){ return ui; };
    void SetMpx3GUI(Mpx3GUI *p);
    void setSelectedThreshold(int threshold){ ui->comboBox->setCurrentText(QString("Threshold %1").arg(threshold)); _currentThreshold = threshold;  }
    //void updateSelectedThreshold(){ui->comboBox->setCurrentText( QString("Threshold %1").arg(_mpx3gui->getVisualization()->getActiveThreshold()) ); }
    void setRegion(QPoint pixel_begin, QPoint pixel_end); //!Sets the region selected by the user by saving the upper left and lower right corner.
    void plotESF();
    //void setParams(parameter_vector params){_params = params;}
    //void setxStart(double start){_xstart = start;}
    //void setPlotLength(int length){_plotrange = length;}
    void clearDataAndPlots(); //!Clears all data and plots in the dqe view when new data is loaded or when a new region is selected.
    void refreshLog(bool emptylog){ if(emptylog)ui->textBrowser->clear(); else ui->textBrowser->setText(_logtext);} //!Changes or empties (when emptylog == true) the log.

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

    void on_npsPushButton_clicked();

    void ConnectionStatusChanged(bool connected);

    void on_logScaleCheckBox_toggled(bool checked);

private:
    Ui::QCstmDQE *ui;
    Mpx3GUI * _mpx3gui;
    //int _currentThreshold;
    QPoint _beginpix, _endpix;
    QVector<QVector<double> > _ESFdata; //Contains a vector for the distances and one for the pixel values of the esf data.
    QVector<QVector<double> > _ESFbinData; //Necessary?? //Contains a vector for the middle of distance bins and one for the mean pixel values of the esf data.
    QVector<QVector<double> > _ESFsmoothData; //?
    QVector<QVector<double> > _LSFdata;
    QVector<QVector<double> > _MTFdata;
    parameter_vector _params;   //!Contains the parameters of the error function used for the fitting.
    double _xstart;
    double _plotrange;
    int _currentThreshold;

    QStringList _NPSfilepaths;  //!Holds the filepaths of all the loaded datafiles.
    QString _logtext;   //!A QString that holds the text that is to appear in the log window.

    //Options
    bool _useDerFit = true;     //! Indicates whether the LSF should be made of the theoretical derivative using the calculated parameters (true) or the numerical derivative of the (smoothed) binned data.
    double _binsize = 1;        //!Specifies the size of the bins to be used for the ESF data.
    double _stepsize = 0.1;     //!Specifies the distance between datapoints of the fitplot in pixels.
    double _histStep = 0.5;     //!Specifies the distance between datapoints in the LSF (in pixels)
    bool useErrorFunc = false;

    //functions:
    QVector<QVector<double> > calcESFbinData();     //!Puts the Edge Spread Function data that is calculated in calcESFdata() into bins of size _binsize.
    QVector<QVector<double> > calcESFfitData();     //!Creates the datapoints of the fitted function by using the parameters calculated by the fitting in the used function model.
    QVector<QVector<double> > calcSmoothedESFdata(QVector<QVector<double> > data);
    QVector<QVector<double> > calcLSFdata(); //!Creates the datapoints of the derivative of the fitted function, by using the parameters calculated by the fitting and using them in the theoretical derivative of the function model.
    QVector<QVector<double> > calcNumDerivativeOfdata(QVector<QVector<double> > data);  //!Calculates the numerical derivative of a given set of data. Used for LSF.
    QVector<QVector<double> > calcMTFdata();        //!Calculates the datapoints for the MTF, by taking the Fourier Transform of the LSF.
    void plotMTF();
    void plotFitESF();
    void plotLSF();
    void plotEdge(QPoint ab);
    void fitESFparams(QVector<QVector<double> > esfdata);   //!Determines the parameters in the function model that best suit the data by performing a least squares fitting algorithm.


    QString dataToString(QVector<QVector<double> > data);   //!Turns the given data into a string for saving to a textfile.
    double FivePointsStencil(QVector<double> func, int x, double bw);   //! Used for numerical derivation.

    void calcFTsquareRoI();
    parameter_vector fitPlaneParams(QVector<QVector<int> > dataROI);
    void plotData3D(QtDataVisualization::QScatterDataArray data3D);


 signals:
    void start_takingData();
    void open_data(bool, bool, QString);

};

#endif // QCSTMDQE_H
