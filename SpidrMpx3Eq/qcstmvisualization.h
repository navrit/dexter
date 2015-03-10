#ifndef QCSTMVISUALIZATION_H
#define QCSTMVISUALIZATION_H

#include <QWidget>

namespace Ui {
  class QCstmVisualization;
}

class QCstmVisualization : public QWidget
{
  Q_OBJECT
private:
  Ui::QCstmVisualization *ui;

public:
  explicit QCstmVisualization(QWidget *parent = 0);
  ~QCstmVisualization();
/* Signals and slots for inttercommunication between te different tabs goes here.
 * S&S for communication between members of this tab can be set in the .ui file or constructor.
 */
signals:

public slots:

};

#endif // QCSTMVISUALIZATION_H
