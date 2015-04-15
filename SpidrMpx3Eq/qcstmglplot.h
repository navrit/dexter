#ifndef QCSTMGLPLOT_H
#define QCSTMGLPLOT_H
#include "gradient.h"
#include "qcustomplot.h"
#include <QObject>
#include <QVector>
#include <QPointF>
#include <QWheelEvent>

#include <iterator>
#include "dataset.h"

#include <QOpenGLWidget>
#include <QOpenGLShaderProgram>
#include <QOpenGLShader>
#include <QOpenGLTexture>
#include <QOpenGLFunctions_3_3_Core>

#include <cfloat>
#include <iostream>
#include <stdio.h>

class QCstmGLPlot : public QOpenGLWidget,  protected QOpenGLFunctions_3_3_Core
{
Q_OBJECT
public:
  QCstmGLPlot(QWidget *&parent);
  ~QCstmGLPlot();
  void loadFrames(int nx, int ny, int nFrames, GLint **data );
  void loadGradient();
 private:
  /*We use normal OpenGL vao's and vbo's as opposed to QOpenGLBuffer because the latter is not feature complete*/
  GLuint  vao;
  //TODO: optimize this handling. Merge different vbos/coordinates when possible.
  GLuint  vbo[3];//0 = position, 1 = texture, 2 = layer selectors
  QOpenGLShaderProgram program;
  //QMap<QString, QOpenGLShader*> vertexShaders;
  //QMap<QString, QOpenGLShader*>fragmentShaders;
  QOpenGLTexture *gradientTex = 0, *dataTex = 0;
  //QOpenGLTexture *texture = 0, *gradientTex = 0;
  Gradient *gradient;

  GLfloat points[4*2];
  GLfloat textureCoordinates[2*4];
  //GLfloat outerBoxGradient[2*4];
  //GLfloat innerBoxGradient[2*4];
  //GLfloat gradientTexCoords[1*2*2]; //TODO: reduce to 2 using instanced rendering., remove using vertex Ids?

  int nx =512, ny =512;
  int nLayers = 0;
  GLfloat offsetX =0, offsetY = 0, zoom = 1.0, baseSizeX, baseSizeY;
  GLint offsetLoc, zoomLoc, aspectRatioLoc, resolutionLoc, textureLoc, gradientTexLoc, layerLoc, clampLoc; //uniform binding locations.
  GLint positionLoc, texcoordLoc;//Attribute binding locations.
  QPoint clickedLocation;
  bool clicked = false, gradientChanged = true;

  void paintGL();
  void initializeGL();
  void resizeGL(int w, int h) ;
  void initializeLocations();
  void initializeShader();
  //void grabShadersFrom(QString directory);
public: //functions
  QPoint pixelAt(QPoint position);
  Gradient* getGradient(){return gradient;}
  void setGradient(Gradient *gradient);
public: //events
  void setSize(int nx, int ny){this->nx=nx; this->ny = ny;}
  void wheelEvent(QWheelEvent *event);
  void mouseMoveEvent(QMouseEvent *event);
  void keyPressEvent(QKeyEvent *event);
  void mousePressEvent(QMouseEvent *event){this->setCursor(Qt::ClosedHandCursor); clickedLocation = event->pos();clicked = true;}
  void mouseReleaseEvent(QMouseEvent *event){this->setCursor(Qt::ArrowCursor); clicked = false;}
  void contextMenuEvent(QContextMenuEvent *);
public slots:
  void setData(QVector<int*> layers);
  void setActive(int layer);
  void setRange(int min, int max);
  void setRange(QCPRange range);
  void setZoom(float change);
  void setOffset(GLfloat x, GLfloat y);
  void addOffset(GLfloat x, GLfloat y);
 signals:
  void hovered_pixel_changed(QPoint);
  void pixel_selected(QPoint, QPoint);
};

#endif // QCSTMGLPLOT_H
