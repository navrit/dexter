#ifndef GRADIENT_H
#define GRADIENT_H
#include <QOpenGLFunctions>
#include <QVector>
#include <QMap>
#include <QColor>
#include <stdio.h>

class Gradient{
private:
  int InterpolationMode;
  int nPoints = 512;
  QVector<GLfloat> colorArray;
  QMap<GLfloat, QColor> fixedPoints;
public:
  QVector<GLfloat> getColorArray(){return colorArray;}
  void setNPoints(int nPoints){
    this->nPoints = nPoints;
  }
  GLfloat* fillArray(){
    int nFixedPoints = fixedPoints.count();
    if(nFixedPoints <  2)//We need at least 2 fixed points to interpolate.
      return nullptr;
    colorArray.resize(nPoints*3);
    QMap<float,QColor>::iterator prev = fixedPoints.end()-1;
    QMap<float,QColor>::iterator next = fixedPoints.begin();
    GLfloat deltaRange =prev.key() - next.key();
    GLfloat delta = deltaRange/nPoints;
    prev = fixedPoints.begin();
    next = prev+1;
    printf("prev:%f\t next:%f\n", prev.key(), next.key());
    float innerDelta = next.key()-prev.key();
    float samplePoint = prev.key()-delta/2;
    for(int i = 0; i < nPoints;i++){
        samplePoint += delta;
        while(samplePoint >= next.key()){//TODO: breaks on negative values, add sign detection?
            prev = next;
            next++;
            printf("prev:%f\t next:%f\n", prev.key(), next.key());
            innerDelta = next.key()-prev.key();
          }
        float a0 = (samplePoint-prev.key())/innerDelta;
        float a1 = 1-a0;
      colorArray[i*3+0]  = a1*prev.value().redF()+a0*next.value().redF();
      colorArray[i*3+1]  = a1*prev.value().greenF()+a0*next.value().greenF();
      colorArray[i*3+2]  = a1*prev.value().blueF()+a0*next.value().blueF();
    }
    return colorArray.data();
  }

  void setInterpolationMode(int mode);

  GLfloat* getArray(){return colorArray.data();}
  int getLength(){return colorArray.size()/3;}
  void addColor(GLfloat anchor, QColor color){
    fixedPoints[anchor] = color;
  }
};

#endif // GRADIENT_H
