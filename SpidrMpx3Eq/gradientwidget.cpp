#include "gradientwidget.h"
#include <QPainter>
#include <QString>
#include <stdio.h>
GradientWidget::GradientWidget(QWidget *parent) : QWidget(parent)
{

}

GradientWidget::~GradientWidget()
{

}

void GradientWidget::paintEvent(QPaintEvent *event){ //TODO: caching, auto-formating of labels.
  int requiredWidth = 0;
  printf("Called paintEvent\n");
  if(!m_gradient_image)
    return;
  printf("Starting drawing\n");
  QPainter painter(this);
  painter.begin(this);
  printf("Image size: %d, %d\n", m_gradient_image->width(), m_gradient_image->height());
  m_label_spacing = this->height()/((float) m_nlabels);
  //painter.drawImage(0,0, *m_gradient_image);
  //int vpadding= this->fontMetrics().height("0123456789.")/2;
  QRectF boundingBox;
  QString label;
  label.sprintf(" %.0f", m_max);
  painter.drawText(QRectF(30,0, this->width(), this->height()),Qt::AlignTop,
                   label, &boundingBox);
  requiredWidth = (requiredWidth < 30+boundingBox.width()? 30+boundingBox.width() : requiredWidth);
  //painter.drawRect(boundingBox);
  float spacingTop = boundingBox.height();
  label.sprintf(" %.0f", m_min);
  painter.drawText(QRectF(30,0, this->width(), this->height()),Qt::AlignBottom,
                   label, &boundingBox);
  requiredWidth = (requiredWidth < 30+boundingBox.width()? 30+boundingBox.width() : requiredWidth);
  //painter.drawRect(boundingBox);
  float spacingBottom = boundingBox.height();
  float span = this->height()-spacingBottom/2-spacingTop/2;
  painter.drawPixmap(0,(int)spacingTop/2, 30, (int)(span), m_gradient_pixmap);
  m_label_spacing = (this->height()-spacingBottom-spacingTop)/m_nlabels;
  for(int i = 0; i < m_nlabels; i++){
      QRectF newBounds(30, i*m_label_spacing+spacingTop, this->width()-m_gradient_image->width(), m_label_spacing);
      label.sprintf(" %.0f", m_min+(m_max-m_min)*(newBounds.y()+0.5*newBounds.height()-spacingTop/2)/span);
      painter.drawText(newBounds,Qt::AlignVCenter,
                       label, &boundingBox);
      requiredWidth = (requiredWidth < 30+boundingBox.width()? 30+boundingBox.width() : requiredWidth);
      //painter.drawRect(newBounds);
    }
  if(requiredWidth > this->width())
    this->setMinimumWidth(requiredWidth);
}

void GradientWidget::resizeEvent(QResizeEvent *event){
  QVector<GLfloat> colors = m_gradient->getColorArray();
  delete m_gradient_image;
  m_gradient_image = new QImage(1,event->size().height(),QImage::Format_RGB32);
  double delta =(colors.length()/3)/ ((double)m_gradient_image->height());
  for(int i = 0; i < m_gradient_image->height();i++){
    QRgb *scanline = (QRgb*) m_gradient_image->scanLine( m_gradient_image->height()-i-1);//TODO: check if this is the right way around.
    int sample = round(i*delta);
    QRgb color = qRgb(255*colors[sample*3+0], 255*colors[sample*3+1], 255*colors[sample*3+2]);
    for(int j = 0; j < m_gradient_image->width();j++)
      scanline[j] = color;
  }
  m_gradient_pixmap = QPixmap::fromImage(*m_gradient_image);
  this->update();
}

void GradientWidget::setGradient(Gradient *gradient){
  m_gradient = gradient;
  QVector<GLfloat> colors = m_gradient->getColorArray();
  delete m_gradient_image;
  printf("Requesting new image: %d/3, %d\n",colors.length(),1);
  m_gradient_image = new QImage(this->width()/2,this->height(),QImage::Format_RGB32);
  for(int i = 0; i < m_gradient_image->height();i++){
    QRgb *scanline = (QRgb*) m_gradient_image->scanLine(i);
    for(int j = 0; j < m_gradient_image->width();j++)
      scanline[j] = qRgb(255*colors[i*3+0], 255*colors[i*3+1], 255*colors[i*3+2]);
  }
  //this->repaint();
  this->update();
}

void GradientWidget::set_range(int min, int max){
  m_min = min; m_max = max;
  this->update();
}
