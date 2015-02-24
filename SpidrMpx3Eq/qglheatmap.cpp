#include "qglheatmap.h"

#include <QDirIterator>
#include <QOpenGLContext>
#include <QPixmap>
#define SHADER_DIR "Z:/Bonz/SpidrMpx3Eq/shaders"
const GLfloat squareVertices[] =  {-1.0, -1.0,
                              -1,0,  1.0,
                               1.0, -1.0,
                               1.0,  1.0
                               };
const GLfloat* QGLHeatmap::square = squareVertices;

QGLHeatmap::QGLHeatmap(QWidget* parent) : QOpenGLWidget(parent)
{
  qDebug() << "Creating QGLHeatmap";
  //qDebug() << "current context: " << QOpenGLContext::currentContext();
  this->setParent(parent);
}

QGLHeatmap::~QGLHeatmap()
{

}

void QGLHeatmap::indexHeatmaps(QString folder){
  QDirIterator it(folder, QStringList() << "*.frag", QDir::Files, QDirIterator::Subdirectories);
  while (it.hasNext()) {
      QFileInfo fileInfo(it.next());
      qDebug() << "heatmap: " << fileInfo.absoluteFilePath() << " (" << fileInfo.baseName() << ")";
      QOpenGLShader *shader = new QOpenGLShader(QOpenGLShader::Fragment, this);
      shader->compileSourceFile(fileInfo.absoluteFilePath());
      heatmaps.insert(fileInfo.baseName(), shader);
  }
}

void QGLHeatmap::initializeGL()
{
    qDebug() << "Initing opengl!";
    initializeOpenGLFunctions();
    glClearColor(0, 0, 1, 1.0f);
    indexHeatmaps(SHADER_DIR);


    VBO.create();
    VBO.setUsagePattern(QOpenGLBuffer::StaticDraw);
    VBO.bind();
    VBO.allocate(square, sizeof(square));
    VAO.create();
    VAO.bind();

    //texture = new QOpenGLTexture(QImage)

    program.addShaderFromSourceFile(QOpenGLShader::Vertex, QString(SHADER_DIR).append("/passthrough.vert"));
    program.addShaderFromSourceFile(QOpenGLShader::Fragment, QString(SHADER_DIR).append("/test.frag"));
    program.link();
    program.bind();
    program.enableAttributeArray("position");
    program.setAttributeArray("position", GL_FLOAT, nullptr, 0, 2); //is this the same as below?
    //glVertexAttribIPointer(program.attributeLocation("position"), 2, GL_FLOAT, 0, NULL);

    qDebug() << "current context: " << QOpenGLContext::currentContext();
}

void QGLHeatmap::paintGL(){
  glClear(GL_COLOR_BUFFER_BIT);
  VBO.bind();
  VAO.bind();
  program.bind();
  glDrawArrays(GL_TRIANGLES,0,4);
  program.release();
}

void QGLHeatmap::loadTexture(QString filename){
  //texture = bindTexture(QPixMap(filename));
}
