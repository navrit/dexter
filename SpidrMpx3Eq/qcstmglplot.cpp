#include "qcstmglplot.h"

QCstmGLPlot::QCstmGLPlot(QWidget* &parent){
  this->setParent(parent);
  program =  new QOpenGLShaderProgram(this);
}

QCstmGLPlot::~QCstmGLPlot()
{

}

void QCstmGLPlot::initializeGL(){
  qDebug() << "init called!";
  printf("init called\n");
  GLfloat points[] = {
    0.0f,  1.0f,
    1.0f, -1.0f,
   -1.0f, -1.0f,
  };
  initializeOpenGLFunctions();
  glClearColor(1.0,0,0,1.0);
  vbo = 0;
  glGenBuffers (1, &vbo);
  glBindBuffer (GL_ARRAY_BUFFER, vbo);
  glBufferData (GL_ARRAY_BUFFER, sizeof(points), points, GL_STATIC_DRAW);
  vao = 0;
  glGenVertexArrays (1, &vao);
  glBindVertexArray (vao);
  glEnableVertexAttribArray (0);
  glBindBuffer (GL_ARRAY_BUFFER, vbo);
  glVertexAttribPointer (0, 2, GL_FLOAT, GL_FALSE, 0, NULL);
  program->addShaderFromSourceCode(QOpenGLShader::Vertex, "#version 330\n"
                                                          "in vec2 position;"
                                                          "void main(void){ gl_Position = vec4(position,0,1);}");
  program->addShaderFromSourceCode(QOpenGLShader::Fragment, "#version 330\n"
                                                            "void main(void) { gl_FragColor = vec4(1); }");
  program->link();
  program->bind();

  /*vao.create();
  vao.bind();
  //glBindBuffer (GL_ARRAY_BUFFER, vbo.);
  glEnableVertexAttribArray (0);
  glVertexAttribPointer (0, 2, GL_FLOAT, GL_FALSE, 0, NULL);

  vbo.create();
  vbo.bind();
  vbo.allocate(points, sizeof(points));
  vbo.setUsagePattern(QOpenGLBuffer::StaticDraw);*/

}

void QCstmGLPlot::paintGL(){
  printf("paint called\n");
  glClear(GL_COLOR_BUFFER_BIT);
  program->bind();
  //vao.bind();
  //glUseProgram (shader_programme);
   //glBindVertexArray (vao);
   // draw points 0-3 from the currently bound VAO with current in-use shader
   glDrawArrays (GL_TRIANGLES, 0, 3);
   //vao.release();
   //program->release();
}

void QCstmGLPlot::resizeGL(int w, int h){
  QOpenGLWidget::resizeGL(w,h);
  glClearColor(((double)rand())/RAND_MAX, ((double)rand())/RAND_MAX, ((double)rand())/RAND_MAX,1);
}
