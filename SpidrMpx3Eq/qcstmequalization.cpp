#include "qcstmequalization.h"
#include "ui_qcstmequalization.h"

QCstmEqualization::QCstmEqualization(QWidget *parent) :
  QWidget(parent),
  ui(new Ui::QCstmEqualization)
{
  ui->setupUi(this);
  QList<int> defaultSizesMain; //The ratio of the splitters. Defaults to the golden ratio because "oh! fancy".
  defaultSizesMain.append(2971215);
  defaultSizesMain.append(1836312);
  QList<int> defaultSizesHist;
  defaultSizesHist.append(2971215);
  defaultSizesHist.append(1836312);

  for(int i = 0; i < ui->mainSplitter->count();i++){
      QWidget *child = ui->mainSplitter->widget(i);
      child->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
      child->setMinimumSize(1,1);
    }
  for(int i = 0; i < ui->histSetupSplitter->count();i++){
      QWidget *child = ui->histSetupSplitter->widget(i);
      child->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
      child->setMinimumSize(1,1);
    }
  ui->histSetupSplitter->setSizes(defaultSizesHist);
  ui->mainSplitter->setSizes(defaultSizesMain);
}

QCstmEqualization::~QCstmEqualization()
{
  delete ui;
}
