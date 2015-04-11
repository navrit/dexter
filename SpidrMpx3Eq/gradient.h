#ifndef GRADIENT_H
#define GRADIENT_H
#include <QOpenGLFunctions>
#include <QVector>
#include <QMap>
#include <QColor>
#include <stdio.h>

class Gradient{//Should have just used QGradient for this... oh well.
private:
  int interpolationMode=0 ;
  int nPoints = 512;
  QVector<GLfloat> colorArray;
  QMap<GLfloat, QColor> fixedPoints;
  QString name;
public:
  static QVector<Gradient*> fromJsonFile(QString filename);
  static Gradient* fromJsonObject(QJsonObject object);
  QVector<GLfloat> getColorArray(){return colorArray;}
  void setNPoints(int nPoints){ this->nPoints = nPoints; }
  void fillArray();
  void setInterpolationMode(int mode){ interpolationMode = mode;}
  GLfloat* getArray(){return colorArray.data();}
  int getLength(){return colorArray.size()/3;}
  QString getName(){return name;}
  void setName(QString name){this->name  = name;}
  void addColor(GLfloat anchor, QColor color){ fixedPoints[anchor] = color;  }
};

#endif // GRADIENT_H
