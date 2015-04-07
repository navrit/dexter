#ifndef QCSTMTHRESHOLD_H
#define QCSTMTHRESHOLD_H

#include <QWidget>

namespace Ui {
  class QCstmThreshold;
}

class QCstmThreshold : public QWidget
{
  Q_OBJECT

public:
  explicit QCstmThreshold(QWidget *parent = 0);
  ~QCstmThreshold();

private:
  Ui::QCstmThreshold *ui;
};

#endif // QCSTMTHRESHOLD_H
