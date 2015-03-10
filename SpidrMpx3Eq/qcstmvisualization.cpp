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

void QCstmVisualization::on_openfileButton_clicked()
{/*
  QImage image;
  QStringList files = QFileDialog::getOpenFileNames(this, tr("Open File"),QStandardPaths::writableLocation(QStandardPaths::PicturesLocation), tr("Images (*.png *.xpm *.jpg *.gif *.png)"));
  if(files.isEmpty())
          return;
  ui->layerCombobox->clear();
  ui->layerCombobox->addItems(files);
  ui->histogramPlot->clear();
  delete[] nx; delete[] ny;
  for(unsigned u = 0; u < nData; u++){
          delete[] data[u];
          delete hists[u];
  }
  delete[] data;
  delete[] hists;
  ui->heatmap->clear();
  nData = files.length();
  data = new int*[nData];
  hists = new histogram*[nData];
  nx = new unsigned[nData]; ny = new unsigned[nData];
  for(unsigned i = 0; i < nData; i++){
          image.load(files[i]);
          if (image.isNull()) {
                  QMessageBox::information(this, tr("Image Viewer"), tr("Cannot load %1.").arg(files[i]));
                  return;
          }
          nx[i] = image.width(); ny[i] = image.height();
          data[i] = new int[nx[i]*ny[i]];
          for(unsigned u = 0; u < ny[i]; u++)
                  for(unsigned w = 0; w < nx[i];w++){
                          QRgb pixel = image.pixel(w,u);
                          data[i][u*nx[i]+w] = qGray(pixel);
                  }
          hists[i] = new histogram(data[i],nx[i]*ny[i], 1);
          ui->histogramPlot->addHistogram(hists[i]);
          ui->heatmap->addData(data[i], nx[i], ny[i]);
  }
  ui->histogramPlot->setActive(0);
  ui->heatmap->setActive(0);
  //ui->histogramPlot->rescaleAxes();
  ui->heatmap->rescaleAxes();*/
}
