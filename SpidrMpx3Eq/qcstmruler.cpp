#include "qcstmruler.h"
#include <QPainter>
#include <stdio.h>
#include <QResizeEvent>
QCstmRuler::QCstmRuler(QWidget *parent) : QWidget(parent)
{
  m_display_min.setX(0); m_display_min.setY(0);
  m_display_max.setY(1); m_display_max.setX(1);
}


void QCstmRuler::paintEvent(QPaintEvent * /*event*/){
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
  m_margin = 5;
}

void QCstmRuler::paintTop(){
  QPainter painter(this);
  int requiredWidth = 0;
  int toPrint;// lastPrint = m_display_max.y();
  QString label;
  if(!painter.isActive())
    painter.begin(this);
  QRectF newBounds;
  QRectF boundingBox(INT_MIN, INT_MIN,0,0);
  QRectF oldBoundingBox(INT_MIN, INT_MIN,0,0);
  //double pixelConversionFactor = this->height()/(m_display_min.y()-m_display_max.y()); //multiplicative factor for converting "physical" coordinates to pixel coords., need to subtract max too.
  //double offset_min = this->height() - pixelConversionFactor*(ceil(m_display_min.y())-m_display_min.y());//offset from the bottom to the nearest integer, in pixels.
  double offset_min = (ceil(m_display_min.x())-m_display_min.x())/(m_display_max.x()-m_display_min.x())*this->width();
  double range = 1+floor(m_display_max.x())-ceil(m_display_min.x());//amount of integer points, including endpoints between the top and bottom.
  int nSteps = range;

  double label_spacing = (floor(m_display_max.x())-ceil(m_display_min.x()))/((nSteps-1)*(m_display_max.x()-m_display_min.x()))*this->width();
  //double subSpacing = label_spacing/(m_subDashCount+1);
  for(int i = 0; i < nSteps; i++){
      toPrint =  ceil(m_display_min.x())+i;
      newBounds.setRect(offset_min+i*label_spacing-0.5*label_spacing,0, label_spacing,this->height()-m_dashLength);
      if( toPrint > m_cutoff.right() ||  toPrint < m_cutoff.left())
        continue;
      label.sprintf(" %d",toPrint);
      if(!newBounds.intersects(oldBoundingBox)){
          painter.drawText(newBounds,Qt::AlignHCenter, label, &boundingBox);
          painter.eraseRect(newBounds);
          if(!boundingBox.intersects(oldBoundingBox)){
              painter.drawText(newBounds,Qt::AlignHCenter|Qt::TextDontClip, label, &boundingBox);
              painter.drawLine(QPointF(newBounds.center().x(), this->height()), QPointF( newBounds.center().x(), this->height()-m_dashLength));
              oldBoundingBox = boundingBox;
            }
          requiredWidth = (requiredWidth < 0+boundingBox.height()? 0+boundingBox.height() : requiredWidth);
       }
      /*for(int i = 1; i <= m_subDashCount;i++)
        painter.drawLine(QPointF(this->width(), newBounds.center().y()-subSpacing*i), QPointF(this->width()-m_subDashLength, newBounds.center().y()-subSpacing*i));*/
    }
  /*for(int i = 1; i <= m_subDashCount+1;i++)
    painter.drawLine(QPointF(this->width(), newBounds.center().y()+subSpacing*i), QPointF(this->width()-m_subDashLength, newBounds.center().y()+subSpacing*i));*/
  if(requiredWidth > m_requiredWidth){
      m_requiredWidth = requiredWidth;
      this->setMinimumHeight(m_requiredWidth +m_dashLength);
    }
  painter.end();
}

void QCstmRuler::paintLeft(){
  QPainter painter(this);
  int requiredWidth = 0;
  int toPrint; // lastPrint = m_display_max.y();
  QString label;
  if(!painter.isActive())
    painter.begin(this);
  QRectF newBounds;
  QRectF boundingBox(INT_MAX, INT_MAX,0,0);
  QRectF oldBoundingBox(INT_MAX, INT_MAX,0,0);
  //double pixelConversionFactor = this->height()/(m_display_min.y()-m_display_max.y()); //multiplicative factor for converting "physical" coordinates to pixel coords., need to subtract max too.
  //double offset_min = this->height() - pixelConversionFactor*(ceil(m_display_min.y())-m_display_min.y());//offset from the bottom to the nearest integer, in pixels.
  double offset_min = (1-(ceil(m_display_min.y())-m_display_min.y())/(m_display_max.y()-m_display_min.y()))*this->height();
  double range = 1+floor(m_display_max.y())-ceil(m_display_min.y());//amount of integer points, including endpoints between the top and bottom.
  int nSteps = range;

  double label_spacing = (floor(m_display_max.y())-ceil(m_display_min.y()))/((nSteps-1)*(m_display_max.y()-m_display_min.y()))*this->height();
  for(int i = 0; i < nSteps; i++){
      toPrint =  ceil(m_display_min.y())+i;
      newBounds.setRect(0,offset_min-i*label_spacing-0.5*label_spacing, this->width()-m_dashLength, label_spacing);
      if(toPrint < m_cutoff.top() ||  toPrint > m_cutoff.bottom())
        continue;
      label.sprintf(" %4d",toPrint);
      if(!newBounds.intersects(oldBoundingBox)){
          painter.drawText(newBounds,Qt::AlignVCenter, label, &boundingBox);
          painter.eraseRect(newBounds);
          if(!boundingBox.intersects(oldBoundingBox)){
              painter.drawText(newBounds,Qt::AlignVCenter|Qt::TextDontClip, label, &boundingBox);
              oldBoundingBox = boundingBox;
              painter.drawLine(QPointF(this->width(), newBounds.center().y()), QPointF(this->width()-m_dashLength, newBounds.center().y()));
            }
      requiredWidth = (requiredWidth < 0+boundingBox.width()? 0+boundingBox.width() : requiredWidth);

        }
      /*for(int i = 1; i <= m_subDashCount;i++)
        painter.drawLine(QPointF(this->width(), newBounds.center().y()-subSpacing*i), QPointF(this->width()-m_subDashLength, newBounds.center().y()-subSpacing*i));*/
    }
  /*for(int i = 1; i <= m_subDashCount+1;i++)
    painter.drawLine(QPointF(this->width(), newBounds.center().y()+subSpacing*i), QPointF(this->width()-m_subDashLength, newBounds.center().y()+subSpacing*i));*/
  if(requiredWidth > m_requiredWidth){
      m_requiredWidth = requiredWidth;
      this->setMinimumWidth(m_requiredWidth +m_dashLength);
    }
  painter.end();
}
