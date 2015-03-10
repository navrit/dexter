#include "qcstmvisualization.h"
#include "ui_qcstmvisualization.h"

QCstmVisualization::QCstmVisualization(QWidget *parent) :
  QWidget(parent),
  ui(new Ui::QCstmVisualization)
{
  ui->setupUi(this);
  QList<int> defaultSizesMain = {2971215,1836312}; //The ratio of the splitters. Defaults to the golden ratio because "oh! fancy".

  for(int i = 0; i < ui->mainSplitter->count();i++){
      QWidget *child = ui->mainSplitter->widget(i);
      child->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
      child->setMinimumSize(1,1);
    }
  ui->mainSplitter->setSizes(defaultSizesMain);
}

QCstmVisualization::~QCstmVisualization()
{
  delete ui;
}
