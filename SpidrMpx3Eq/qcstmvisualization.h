#ifndef QCSTMVISUALIZATION_H
#define QCSTMVISUALIZATION_H

#include <QWidget>

#include "mpx3gui.h"
#include "histogram.h"

namespace Ui {
  class QCstmVisualization;
}

class QCstmVisualization : public QWidget
{
  Q_OBJECT
private:
  Ui::QCstmVisualization *ui;
  ModuleConnection *connection = nullptr;
public:
  explicit QCstmVisualization(QWidget *parent = 0);
  ~QCstmVisualization();
  setConnection(ModuleConnection *connection){
    this->connection = connection;
  }

/* Signals and slots for inttercommunication between te different tabs goes here.
 * S&S for communication between members of this tab can be set in the .ui file or constructor.
 */
signals:

public slots:
private slots:
  void on_openfileButton_clicked();
  void on_data_changed();
};

#endif // QCSTMVISUALIZATION_H
