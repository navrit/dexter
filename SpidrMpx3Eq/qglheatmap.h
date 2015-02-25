#ifndef QGLHEATMAP_H
#define QGLHEATMAP_H

#include <QObject>
#include <QWidget>
#include <QtGui/QOpenGLFunctions_3_3_Core>
//#include <QOpenGLWidget>
#include <QtGui/QOpenGLShader>
#include <QtGui/QOpenGLShaderProgram>
#include <QtGui/QOpenGLBuffer>
#include <QtGui/QOpenGLVertexArrayObject>
#include <QtGui/QOpenGLTexture>

/*TODO:
 * Error handling.
 * See http://doc-snapshot.qt-project.org/qt5-5.4/qopenglwidget.html#details and research blitting for large plots.
 * http://doc.qt.io/qt-5/qtgui-openglwindow-example.html
 */

class QGLHeatmap : public QOpenGLWidget, protected QOpenGLFunctions_3_3_Core{
Q_OBJECT
  static const GLfloat *square;
  QOpenGLVertexArrayObject VAO;
  QOpenGLBuffer VBO;
  QOpenGLShaderProgram program;
  //QOpenGLShader *shader;
  //QOpenGLTexture *texture;
  QMap<QString,QOpenGLShader*> heatmaps;
private:
  void indexHeatmaps(QString folder);
public:
  QGLHeatmap(QWidget* parent);
  ~QGLHeatmap();
public slots:
  void loadTexture(QString filename);
protected:
    void initializeGL();
    void paintGL();
};

#endif // QGLHEATMAP_H
