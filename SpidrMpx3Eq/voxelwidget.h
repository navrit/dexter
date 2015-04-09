#ifndef VOXELWIDGET_H
#define VOXELWIDGET_H

#include <QObject>
#include <QWidget>
#include <QVector>
#include <QKeyEvent>

#include <QOpenGLWidget>
#include <QOpenGLShaderProgram>
#include <QOpenGLShader>
#include <QOpenGLTexture>
#include <QOpenGLBuffer>
#include <QOpenGLFunctions_3_3_Core>


class VoxelWidget : public QOpenGLWidget,  protected QOpenGLFunctions_3_3_Core
{
  QOpenGLShaderProgram program;
  QOpenGLTexture *dataTex;
  QMatrix4x4 mvp_matrix;
  QVarLengthArray<QVector3D> cube;
  QVarLengthArray<QVector3D> normals;
  QVarLengthArray<GLushort> indices;
  QVarLengthArray<GLfloat> data;

  unsigned data_x =0, data_y = 0, data_z = 0;
  QVector3D translation, rotations, range;
  float scale;
  //QOpenGLBuffer *arrayBuffer, *indexBuffer;
  GLuint arrayBuffer, indexBuffer;
  GLuint vao;
  int mvp_matrix_position, range_position;
  void paintGL();
  void initializeGL();
  void resizeGL(int w, int h) ;
  void initShaders();
  void initUniforms();
  void initBuffers();
  void generateCube(QVector3D center, qreal radius);
  void generateTriangle();
  void setModelView();
  void keyPressEvent(QKeyEvent *event);
  void createData(unsigned x, unsigned y, unsigned z);
  void loadData();
public:
  VoxelWidget(QWidget* &parent);
  ~VoxelWidget();
};

#endif // VOXELWIDGET_H
