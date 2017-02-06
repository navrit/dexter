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

private:
    Ui::ImageCalculator *ui = nullptr;
    Mpx3GUI *_mpx3gui = nullptr;
};

#endif // IMAGECALCULATOR_H
