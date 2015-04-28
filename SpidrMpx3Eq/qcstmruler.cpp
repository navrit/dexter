#include "qcstmruler.h"
#include <QPainter>
#include <stdio.h>
#include <QResizeEvent>
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

void QCstmRuler::resizeEvent(QResizeEvent *event){
  if(m_orientation == orientationTop)
    m_nSteps = event->size().width()/10;
  else
    m_nSteps = event->size().height()/10;
}

void QCstmRuler::paintTop(){
  QPainter painter(this);
  float label_spacing = this->width()/((float) m_nSteps+1);
  float subSpacing = label_spacing/(m_subDashCount+1);
  int requiredHeight = 0;
  QRectF boundingBox;
  QString label;
  int toPrint, lastPrint = m_display_max.x();
  if(!painter.isActive())
    painter.begin(this);
  QRectF newBounds;
  for(int i = 0; i <= m_nSteps; i++){
       newBounds.setRect( i*label_spacing,0, label_spacing, this->height()-m_dashLength);
      if(newBounds.left() < boundingBox.right()){
          m_nSteps /= 2;
          return repaint();
        }
      toPrint =  m_display_max.x()-(m_display_max.x()-m_display_min.x())*(newBounds.x()+0.5*newBounds.width())/this->width();
      if(toPrint != lastPrint && toPrint >= m_cutoff.left() && toPrint <= m_cutoff.right()){
          lastPrint = toPrint;
          label.sprintf(" %d",toPrint);
          painter.drawText(newBounds,Qt::AlignHCenter, label, &boundingBox);
          requiredHeight = (requiredHeight < 0+boundingBox.height()? 0+boundingBox.height() : requiredHeight);
      }
      painter.drawLine(QPointF(newBounds.center().x(), this->height()), QPointF(newBounds.center().x(),this->height()-m_dashLength));
      for(int i = 1; i <= m_subDashCount;i++)
        painter.drawLine(QPointF(newBounds.center().x()-subSpacing*i, this->height()), QPointF(newBounds.center().x()-subSpacing*i,this->height()-m_subDashLength));
    }
  for(int i = 1; i <= m_subDashCount+1;i++)
    painter.drawLine(QPointF(newBounds.center().x()+subSpacing*i, this->height()), QPointF(newBounds.center().x()+subSpacing*i,this->height()-m_subDashLength));
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
  int toPrint, lastPrint = m_display_max.y();
  QRectF boundingBox;
  QString label;
  if(!painter.isActive())
    painter.begin(this);
  QRectF newBounds;
  for(int i = 0; i <= m_nSteps; i++){
      newBounds.setRect(0, i*label_spacing, this->width()-m_dashLength, label_spacing);
      if(newBounds.top() < boundingBox.bottom()){
          m_nSteps /= 2;
          return repaint();
        }
      toPrint =  m_display_max.y()-(m_display_max.y()-m_display_min.y())*(newBounds.y()+0.5*newBounds.height())/this->height();
      if(toPrint != lastPrint && toPrint >= m_cutoff.top() && toPrint <= m_cutoff.bottom() ){
          lastPrint = toPrint;
          label.sprintf(" %4d",toPrint);
          painter.drawText(newBounds,Qt::AlignVCenter, label, &boundingBox);
          requiredWidth = (requiredWidth < 0+boundingBox.width()? 0+boundingBox.width() : requiredWidth);
      }
      painter.drawLine(QPointF(this->width(), newBounds.center().y()), QPointF(this->width()-m_dashLength, newBounds.center().y()));
      for(int i = 1; i <= m_subDashCount;i++)
        painter.drawLine(QPointF(this->width(), newBounds.center().y()-subSpacing*i), QPointF(this->width()-m_subDashLength, newBounds.center().y()-subSpacing*i));
    }
  for(int i = 1; i <= m_subDashCount+1;i++)
    painter.drawLine(QPointF(this->width(), newBounds.center().y()+subSpacing*i), QPointF(this->width()-m_subDashLength, newBounds.center().y()+subSpacing*i));
  if(requiredWidth > m_requiredWidth){
      m_requiredWidth = requiredWidth;
      this->setMinimumWidth(m_requiredWidth +m_dashLength);
    }
  painter.end();
}
