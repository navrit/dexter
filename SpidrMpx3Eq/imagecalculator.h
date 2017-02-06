#ifndef IMAGECALCULATOR_H
#define IMAGECALCULATOR_H

#include <QDialog>
#include "mpx3gui.h"

namespace Ui {
class ImageCalculator;
}

class ImageCalculator : public QDialog
{
    Q_OBJECT

public:
    explicit ImageCalculator(Mpx3GUI *mg, QWidget *parent = 0);
    ~ImageCalculator();

private slots:
    void on_okCancelBox_accepted(); //! Ok pressed
    void on_okCancelBox_rejected(); //! Cancel pressed

    void on_comboBox_removeLayer_currentIndexChanged(int index); //! Remove selected item by index
                                                                 //! Note: Intended for manual cleanup of operations without major interruption
    void on_pushButton_removeLayer_clicked();
    void on_pushButton_calcAllEnergyBins_clicked();

private:
    Ui::ImageCalculator *ui = nullptr;
    Mpx3GUI *_mpx3gui = nullptr;
};

#endif // IMAGECALCULATOR_H
