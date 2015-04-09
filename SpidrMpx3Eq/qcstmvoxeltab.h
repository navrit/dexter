#ifndef QCSTMVOXELTAB_H
#define QCSTMVOXELTAB_H

#include <QWidget>

namespace Ui {
  class QCstmVoxeltab;
}

class QCstmVoxeltab : public QWidget
{
  Q_OBJECT

public:
  explicit QCstmVoxeltab(QWidget *parent = 0);
  ~QCstmVoxeltab();

private:
  Ui::QCstmVoxeltab *ui;
};

#endif // QCSTMVOXELTAB_H
