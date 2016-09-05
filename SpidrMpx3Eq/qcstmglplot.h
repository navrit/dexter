/**
 * \class QCstmGLPlot
 *
 * \brief And OpenGL 3 dependent heatmap renderer.
 *
 * This class renders integer data provided by the Dataset class. It uses the class Gradient for its rendering.
 *
 */

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
    //void loadFrames(int nx, int ny, int nFrames, GLint **data );

private:
    const double _minimumRefreshDistance = 3.0;
    void loadGradient();
    bool initialized = false;
    GLuint  vao;
    bool _drawSelectionRectangle = false;
    QPoint _startSelectionPoint;
    QPoint _lastSelectionPoint;
    QPoint _currentSelectionPoint;
    //TODO: optimize this handling. Merge different vbos/coordinates when possible.
    GLuint  vbo[4];//square, offsets, texture coords, orientation.
    QOpenGLShaderProgram program;
    QOpenGLTexture *gradientTex = nullptr, *dataTex = nullptr;
    Gradient *gradient;

    QRubberBand *rubberBand;
    QPoint origin;

    QVarLengthArray<QVector2D> offsets;
    //QVarLengthArray<QVector2D> textureCoordinates;
    int scaleFactor = 1;

    int nx =1, ny =1;
    int nLayers = 0;
    GLfloat offsetX =0, offsetY = 0, zoom = 1.0, baseSizeX, baseSizeY;
    GLfloat _lastZoom = 1.0, _lastdisplacementX = 0., _lastdisplacementY = 0.;
    GLint offsetLoc, zoomLoc, layerLoc, aspectRatioLoc, clampLoc, texLoc, gradientLoc; //uniform binding locations.
    GLint offsetAttributeLoc, squareLoc, textureCoordsLoc, orientationLoc;//Attribute binding locations.
    QPoint clickedLocation;
    QPoint clickReleaseLocation;

    bool clicked = false, gradientChanged = true;
    bool rightClicked = false;

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
    int XYtoX(int x, int y, int dimX) { return y * dimX + x; }
    int XYtoX(QPoint p, int dimX) { return XYtoX(p.x(), p.y(), dimX); }

public: //functions
    //! Computes and emits the coordinates of the viewport. This is used by the Ruler class to determine what it should display.
    void recomputeBounds();
    QPoint pixelAt(QPoint position);
    Gradient* getGradient(){return gradient;}
    void setGradient(Gradient *gradient);
    double distance2D(QPoint p1, QPoint p2);

public: //events
    void wheelEvent(QWheelEvent *event);
    void keyPressEvent(QKeyEvent *event);
    void mouseDoubleClickEvent(QMouseEvent * event);
    //void contextMenuEvent(QContextMenuEvent *);
    int getNx(){return nx;}
    int getNy(){return ny;}
    void setAlphaBlending(bool setOn);//!< toggles alpha blending. When on, out-of-bounds pixels will become transparent.

public slots:
    //void setSize(int nx, int ny);
    //void setSize(QPoint size){this->setSize(size.x(), size.y());}
    void readData(Dataset &data);//!< (Re-)reads the entire Dataset.  Should be optimized to only grab the active threshold.
    //void setData(QVector<int*> layers);
    void setActive(int layer); //!< Sets the active layer to display
    void setRange(int min, int max); //!< Sets the data-range of the heatmap.
    void setRange(QCPRange range);//!< Sets the data-range of the heatmap.
    void setZoom(float change);//!< sets the zoom factor (multiplicative).
    void setOffset(GLfloat x, GLfloat y); //!< Set the offset of the images, used for moving around.
    void addOffset(GLfloat x, GLfloat y);//!< Adds an offset to the current one.

private slots:
    void mouseMoveEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);//{if(event->buttons()== Qt::LeftButton){this->setCursor(Qt::ClosedHandCursor); clickedLocation = event->pos();clicked = true;}}
    void mouseReleaseEvent(QMouseEvent * event);//{this->setCursor(Qt::ArrowCursor); clicked = false;}

signals:
    void hovered_pixel_changed(QPoint); //!< The pixel hovered by the mouse-cursor has changed. Passes the coordinate of the hovered pixel.
    void pixel_selected(QPoint, QPoint);//!< A pixel has been clicked. Passes the coordinate in the chip and position of the relevant chip.
    void region_selected(QPoint, QPoint, QPoint);//!< A right-button mouse drag selects a region. Passes the coordinate in the chip and position of the relevant chip.
    void offset_changed(QPointF offset);
    void zoom_changed(float zoom);
    void size_changed(QPoint size);
    void bounds_changed(QRectF bounds);
    void double_click(bool with_corrections = false);
};

#endif // QCSTMGLPLOT_H
