#ifndef THRESHOLDSCAN_H
#define THRESHOLDSCAN_H

#include <QWidget>

namespace Ui {
class thresholdScan;
}

class thresholdScan : public QWidget
{
    Q_OBJECT

public:
    explicit thresholdScan(QWidget *parent = 0);
    ~thresholdScan();

private:
    Ui::thresholdScan *ui;
};

#endif // THRESHOLDSCAN_H
