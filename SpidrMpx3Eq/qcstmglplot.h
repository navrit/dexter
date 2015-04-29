#ifndef QCSTMGLPLOT_H
#define QCSTMGLPLOT_H
#include "gradient.h"
#include "qcustomplot.h"
#include <QObject>
#include <QVector>
#include <QVarLengthArray>
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
  bool initialized = false;
  GLuint  vao;
  //TODO: optimize this handling. Merge different vbos/coordinates when possible.
  GLuint  vbo[4];//square, offsets, texture coords, orientation.
  QOpenGLShaderProgram program;
  QOpenGLTexture *gradientTex = nullptr, *dataTex = nullptr;
  Gradient *gradient;

  QVarLengthArray<QVector2D> offsets;
  //QVarLengthArray<QVector2D> textureCoordinates;
  int scaleFactor = 1;

  int nx =1, ny =1;
  int nLayers = 0;
  GLfloat offsetX =0, offsetY = 0, zoom = 1.0, baseSizeX, baseSizeY;
  GLint offsetLoc, zoomLoc, layerLoc, aspectRatioLoc, clampLoc, texLoc, gradientLoc; //uniform binding locations.
  GLint offsetAttributeLoc, squareLoc, textureCoordsLoc, orientationLoc;//Attribute binding locations.
  QPoint clickedLocation;
  bool clicked = false, gradientChanged = true;

  void paintGL();
  void initializeGL();
  void resizeGL(int w, int h) ;
  void initializeShaders();
  void initializeLocations();
  void initializeTextures();
  void initializeVAOsAndVBOs();
  void populateTextures(Dataset &data);
  void readLayouts(Dataset &data);
  void readOrientations(Dataset &data);
public: //functions
  void recomputeBounds();
  QPoint pixelAt(QPoint position);
  Gradient* getGradient(){return gradient;}
  void setGradient(Gradient *gradient);
public: //events
  void wheelEvent(QWheelEvent *event);
  void mouseMoveEvent(QMouseEvent *event);
  void keyPressEvent(QKeyEvent *event);
  void mousePressEvent(QMouseEvent *event){this->setCursor(Qt::ClosedHandCursor); clickedLocation = event->pos();clicked = true;}
  void mouseReleaseEvent(QMouseEvent *event){this->setCursor(Qt::ArrowCursor); clicked = false;}
  void contextMenuEvent(QContextMenuEvent *);
  int getNx(){return nx;}
  int getNy(){return ny;}
public slots:
  void setSize(int nx, int ny);
  void setSize(QPoint size){this->setSize(size.x(), size.y());}
  void readData(Dataset &data);
  //void setData(QVector<int*> layers);
  void setActive(int layer);
  void setRange(int min, int max);
  void setRange(QCPRange range);
  void setZoom(float change);
  void setOffset(GLfloat x, GLfloat y);
  void addOffset(GLfloat x, GLfloat y);
 signals:
  void hovered_pixel_changed(QPoint);
  void pixel_selected(QPoint, QPoint);
  void offset_changed(QPointF offset);
  void zoom_changed(float zoom);
  void size_changed(QPoint size);
  void bounds_changed(QRectF bounds);
};

#endif // QCSTMGLPLOT_H
