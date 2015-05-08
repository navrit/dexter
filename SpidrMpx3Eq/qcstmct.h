#ifndef QCSTMCT_H
#define QCSTMCT_H

#include <QWidget>

namespace Ui {
  class QCstmCT;
}

class QCstmCT : public QWidget
{
  Q_OBJECT

public:
  explicit QCstmCT(QWidget *parent = 0);
  ~QCstmCT();

private:
  Ui::QCstmCT *ui;
};

#endif // QCSTMCT_H
