#include "qcstmruler.h"
#include <QPainter>
#include <stdio.h>
QCstmRuler::QCstmRuler(QWidget *parent) : QWidget(parent)
{
  m_display_min.setX(0); m_display_min.setY(0);
  m_display_max.setY(1); m_display_max.setX(1);
}


void QCstmRuler::paintEvent(QPaintEvent *event){
  if(m_orientation == orientationTop)
    return paintTop();
  else
    return paintLeft();
}

void QCstmRuler::paintTop(){
  QPainter painter(this);
  float label_spacing = this->width()/((float) m_nSteps+1);
  float subSpacing = label_spacing/(m_subDashCount+1);
  int requiredHeight = 0;
  QRectF boundingBox;
  QString label;
  if(!painter.isActive())
    painter.begin(this);
  for(int i = 0; i <= m_nSteps; i++){
      QRectF newBounds( i*label_spacing,0, label_spacing, this->height()-m_dashLength);
      label.sprintf(" %.0f", m_display_max.x()-(m_display_max.x()-m_display_min.x())*(newBounds.x()+0.5*newBounds.width())/this->width());
      //painter.rotate(45);
      painter.drawText(newBounds,Qt::AlignHCenter, label, &boundingBox);
      //painter.rotate(-45);
      requiredHeight = (requiredHeight < 0+boundingBox.height()? 0+boundingBox.height() : requiredHeight);
      painter.drawLine(QPointF(boundingBox.center().x(), this->height()), QPointF(boundingBox.center().x(),this->height()-m_dashLength));
      for(int i = 1; i <= m_subDashCount;i++)
        painter.drawLine(QPointF(boundingBox.center().x()-subSpacing*i, this->height()), QPointF(boundingBox.center().x()-subSpacing*i,this->height()-m_subDashLength));
   }
  for(int i = 1; i <= m_subDashCount+1;i++)
      painter.drawLine(QPointF(boundingBox.center().x()+subSpacing*i, this->height()), QPointF(boundingBox.center().x()+subSpacing*i,this->height()-m_subDashLength));
  if(requiredHeight > m_requiredWidth){
    m_requiredWidth = requiredHeight;
    this->setMinimumHeight(m_requiredWidth +m_dashLength);
    }
  painter.end();
}

void QCstmRuler::paintLeft(){
  QPainter painter(this);
  float label_spacing = this->height()/((float) m_nSteps+1);
  float subSpacing = label_spacing/(m_subDashCount+1);
  int requiredWidth = 0;
  QRectF boundingBox;
  QString label;
  if(!painter.isActive())
    painter.begin(this);
  for(int i = 0; i <= m_nSteps; i++){
      QRectF newBounds(0, i*label_spacing, this->width()-m_dashLength, label_spacing);
      label.sprintf(" %4.0f", m_display_max.y()-(m_display_max.y()-m_display_min.y())*(newBounds.y()+0.5*newBounds.height())/this->height());
      painter.drawText(newBounds,Qt::AlignVCenter, label, &boundingBox);
      requiredWidth = (requiredWidth < 0+boundingBox.width()? 0+boundingBox.width() : requiredWidth);
      painter.drawLine(QPointF(this->width(), boundingBox.center().y()), QPointF(this->width()-m_dashLength, boundingBox.center().y()));
      for(int i = 1; i <= m_subDashCount;i++)
          painter.drawLine(QPointF(this->width(), boundingBox.center().y()-subSpacing*i), QPointF(this->width()-m_subDashLength, boundingBox.center().y()-subSpacing*i));
    }
  for(int i = 1; i <= m_subDashCount+1;i++)
      painter.drawLine(QPointF(this->width(), boundingBox.center().y()+subSpacing*i), QPointF(this->width()-m_subDashLength, boundingBox.center().y()+subSpacing*i));
  if(requiredWidth > m_requiredWidth){
    m_requiredWidth = requiredWidth;
    this->setMinimumWidth(m_requiredWidth +m_dashLength);
    }
  painter.end();
}
