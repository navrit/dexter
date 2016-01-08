#include "gradient.h"
#include <QFile>
#include <iterator>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>

Gradient* Gradient::fromJsonObject(QJsonObject object){
  Gradient *grad = new Gradient;
  QJsonObject::iterator it = object.find("name");
  if(it== object.end()){
    printf("Name not found\n!");
    grad->setName("UNDEFINED");
  }
  else{
    // printf("name: %s\n", it_name.value().toString().toStdString().c_str());
     grad->setName(it.value().toString());
   }
  it = object.find("mode");
  if(it != object.end()){
      grad->setInterpolationMode( it.value().toInt());
  }
  it = object.find("points");
  QJsonArray pointsArray = it.value().toArray();
  foreach(QJsonValue jsonValue, pointsArray){
    QString colorString = jsonValue.toArray().last().toString();
    QColor color;
    color.setNamedColor(colorString);
    float anchor = jsonValue.toArray().first().toDouble();
    //printf("Got point: %f, %s\n", anchor, colorString.toStdString().c_str());
    grad->addColor(anchor, color);
  }
  grad->setNPoints(512);
  grad->fillArray();
  return grad;
}

QVector<Gradient*> Gradient::fromJsonFile(QString filename){
  QVector<Gradient*> ret(0);
   QFile loadFile(filename);
   if(!loadFile.open(QIODevice::ReadOnly)){
        printf("Couldn't open gradient file %s\n", filename.toStdString().c_str());
        return ret;
     }
   QByteArray binaryData = loadFile.readAll();
   QJsonDocument loadDoc(QJsonDocument::fromJson(binaryData));
   if(loadDoc.isArray()){
       QJsonArray jsonArray = loadDoc.array();
       foreach(QJsonValue jsonValue, jsonArray){
         QJsonObject jsonObject = jsonValue.toObject();
         Gradient *grad = Gradient::fromJsonObject(jsonObject);
         ret.append(grad);
        }
     }
   else
     ret.append(Gradient::fromJsonObject(loadDoc.object()));
   return ret;
}

void Gradient::fillArray(){
      int nFixedPoints = fixedPoints.count();
      if(nFixedPoints <  2)//We need at least 2 fixed points to interpolate.
        return;
      colorArray.resize(nPoints*3);
      QMap<float,QColor>::iterator prev = fixedPoints.end()-1;
      QMap<float,QColor>::iterator next = fixedPoints.begin();
      GLfloat deltaRange =prev.key() - next.key();
      GLfloat delta = deltaRange/nPoints;
      prev = fixedPoints.begin();
      next = prev+1;
      //printf("prev:%f\t next:%f\n", prev.key(), next.key());
      float innerDelta = next.key()-prev.key();
      float samplePoint = prev.key()-delta/2;
      for(int i = 0; i < nPoints;i++){
          samplePoint += delta;
          while(samplePoint >= next.key()){//TODO: breaks on negative values, add sign detection?
              prev = next;
              next++;
              //printf("prev:%f\t next:%f\n", prev.key(), next.key());
              innerDelta = next.key()-prev.key();
            }
          float a0 = (samplePoint-prev.key())/innerDelta;
          float a1 = 1-a0;
        QColor interpolated = Qt::white;
        if(1== interpolationMode ){
            interpolated.setHslF(a1*prev.value().hslHueF()+a0*next.value().hslHueF(),
                                 a1*prev.value().hslSaturationF()+a0*next.value().hslSaturationF(),
                                 a1*prev.value().lightnessF()+a0*next.value().lightnessF());
          }
        else if(2==interpolationMode){
            interpolated.setHsvF(a1*prev.value().hsvHueF()+a0*next.value().hsvHueF(),
                                 a1*prev.value().hsvSaturationF()+a0*next.value().hsvSaturationF(),
                                 a1*prev.value().lightnessF()+a0*next.value().lightnessF());
          }
        else{
            /*interpolated.setRgbF(a1*prev.value().redF()+a0*next.value().redF(),
                                  a1*prev.value().greenF()+a0*next.value().greenF(),
                                 a1*prev.value().blueF()+a0*next.value().blueF());*/ //For some reason this doesn't work for e.g. red-blue delta.
            interpolated.setRedF(a1*prev.value().redF()+a0*next.value().redF());
            interpolated.setGreenF(a1*prev.value().greenF()+a0*next.value().greenF());
            interpolated.setBlueF(a1*prev.value().blueF()+a0*next.value().blueF());
         }//TODO: other mdoes of interpolation
          colorArray[i*3+0]  = interpolated.redF();//a1*prev.value().redF()+a0*next.value().redF();
          colorArray[i*3+1]  = interpolated.greenF();//a1*prev.value().greenF()+a0*next.value().greenF();
          colorArray[i*3+2]  = interpolated.blueF();//a1*prev.value().blueF()+a0*next.value().blueF();
       }
}
