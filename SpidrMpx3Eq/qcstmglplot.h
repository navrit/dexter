#ifndef QCSTMGLPLOT_H
#define QCSTMGLPLOT_H

#include <QObject>
#include <QOpenGLWidget>
#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>
#include <QOpenGLShader>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLTexture>
#include <QOpenGLFunctions_3_3_Core>

class QCstmGLPlot : public QOpenGLWidget,  protected QOpenGLFunctions_3_3_Core
{
public:
  QCstmGLPlot(QWidget *&parent);
  ~QCstmGLPlot();
 private:
  GLuint  vao;
  GLuint  vbo;
  QOpenGLShaderProgram *program = 0;
  QMap<QString, QOpenGLShader*> shaders;
  QOpenGLTexture *texture = 0;
  void paintGL();
  void initializeGL();
  void resizeGL(int w, int h) ;
};

#endif // QCSTMGLPLOT_H
