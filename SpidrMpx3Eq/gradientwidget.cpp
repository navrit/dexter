#include "gradientwidget.h"
#include <QPainter>
#include <QString>
#include <stdio.h>
GradientWidget::GradientWidget(QWidget *parent) : QWidget(parent){
}

GradientWidget::~GradientWidget()
{

}

void GradientWidget::paintEvent(QPaintEvent * /*event*/){ //TODO: caching, auto-formating of labels.
  int requiredWidth = 0;
  if(!m_gradient_image)
    return;
  QPainter painter(this);
  if(!painter.isActive())
    painter.begin(this);
  m_label_spacing = this->height()/((float) m_nlabels);
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
      label.sprintf(" %.0f", m_max-(m_max-m_min)*(newBounds.y()+0.5*newBounds.height()-spacingTop/2)/span);
      painter.drawText(newBounds,Qt::AlignVCenter,
                       label, &boundingBox);
      requiredWidth = (requiredWidth < 30+boundingBox.width()? 30+boundingBox.width() : requiredWidth);
      //painter.drawRect(newBounds);
    }
  if(requiredWidth > this->width())
    this->setMinimumWidth(requiredWidth);
  painter.end();
}

void GradientWidget::setGradient(Gradient *gradient){
  m_gradient = gradient;
  QVector<GLfloat> colors = m_gradient->getColorArray();
  delete m_gradient_image;
  m_gradient_image = new QImage(1, colors.length()/3,QImage::Format_RGB32);
  for(int i = 0; i < m_gradient_image->height();i++){
    QRgb *scanline = (QRgb*) m_gradient_image->scanLine(m_gradient_image->height()-1-i);
    scanline[0] = qRgb(255*colors[i*3+0], 255*colors[i*3+1], 255*colors[i*3+2]);
  }
  m_gradient_pixmap = QPixmap::fromImage(*m_gradient_image);
  this->update();
}

void GradientWidget::set_range(int min, int max){
  m_min = min; m_max = max;
  this->update();
}

void GradientWidget::set_range(QCPRange range){
  m_min = range.lower; m_max = range.upper;
  this->update();
}

