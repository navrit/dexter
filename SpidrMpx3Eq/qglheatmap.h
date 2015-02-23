#ifndef QGLHEATMAP_H
#define QGLHEATMAP_H

#include <QObject>
#include <QWidget>
#include <QOpenGLWidget>

class QGLHeatmap : public QOpenGLWidget
{
Q_OBJECT
public:
  QGLHeatmap();
  ~QGLHeatmap();
};

#endif // QGLHEATMAP_H
