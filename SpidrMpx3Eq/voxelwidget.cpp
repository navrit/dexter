#include "voxelwidget.h"
#include <iostream>
VoxelWidget::VoxelWidget(QWidget* &parent)
{
  this->setParent(parent);
  //arrayBuffer = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
  //indexBuffer = new QOpenGLBuffer(QOpenGLBuffer::IndexBuffer);
  this->setFocusPolicy(Qt::WheelFocus);
  rotations = QVector3D(0,0,0);
  translation = QVector3D(0,0,0);
  scale = .1;
  dataTex = new QOpenGLTexture(QOpenGLTexture::Target3D);

}

VoxelWidget::~VoxelWidget(){
  delete dataTex;
  /*arrayBuffer->destroy();
  indexBuffer->destroy();
  delete arrayBuffer;
  delete indexBuffer;*/
}

void VoxelWidget::initializeGL(){
  initializeOpenGLFunctions();
  //set the background color
  QColor bgColor = this->palette().color(this->backgroundRole());
  glClearColor((GLfloat)(bgColor.red()/255.), (GLfloat)(bgColor.green()/255.), (GLfloat)(bgColor.blue()/255.),1.0); //Sets the background Color to match Qt.
  glEnable (GL_BLEND); glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_DEPTH_TEST);
  //grab and build shaders
  initShaders();
  //setup uniform locations
  initUniforms();
  //Temporary testing
 // generateTriangle();
  generateCube(QVector3D(0,0,0),1.0);
  initBuffers();
  //create and load testing data.
  createData(128,128,128);
  loadData();
}

void VoxelWidget::paintGL(){
  GLenum glErr;
  while((glErr = glGetError()) !=  GL_NO_ERROR)
   std::cout << "pre-render-error! " << glErr << std::endl;
  glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
  program.bind();
  glBindBuffer(GL_ARRAY_BUFFER, arrayBuffer);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
  glBindVertexArray(vao);
 glDrawElements(GL_TRIANGLES, indices.count(), GL_UNSIGNED_SHORT, 0);
}

void VoxelWidget::resizeGL(int w, int h){
  // Calculate aspect ratio
  qreal aspect = qreal(w) / qreal(h ? h : 1);


  setModelView();
  range = QVector3D(w, h, 10);
  program.bind();
  program.setUniformValue(range_position, range);
}

void VoxelWidget::setModelView(){
  // Reset projection
  mvp_matrix.setToIdentity();
  // Set near plane to 3.0, far plane to 7.0, field of view 45 degrees
  qreal aspect = qreal(this->width()) / qreal(this->height() ? this->height() : 1);
  const qreal zNear = 0.1, zFar = 10, fov = 60.0;
  mvp_matrix.perspective(fov, aspect, zNear, zFar);
  mvp_matrix.translate(translation);
  mvp_matrix.rotate(rotations.x(), 1, 0, 0);
  mvp_matrix.rotate(rotations.y(), 0, 1, 0);
  mvp_matrix.rotate(rotations.z(), 0, 0, 1);
  mvp_matrix.scale(scale);

  // Set perspective projection
  //mvp_matrix.perspective(fov, aspect, zNear, zFar);
  program.bind();
  program.setUniformValue(mvp_matrix_position, mvp_matrix);
  this->update();
}

void VoxelWidget::initShaders(){
  if (!program.addShaderFromSourceFile(QOpenGLShader::Vertex, "./shaders/simple3d.vert")){
          std::cout << "Couldn't open vertex shader!\n";
          //return;
    }
  if (!program.addShaderFromSourceFile(QOpenGLShader::Fragment, "./shaders/simple3d.frag")){
          std::cout <<  "Couldn't open fragment shader!\n";
          //return;
    }
  if(!program.link()){
      std::cout << "Couldn't link shaders!\n";
  }
  program.bind();
  std::cout << program.log().toStdString() << std::endl;
}

void VoxelWidget::initUniforms(){
  mvp_matrix_position = program.uniformLocation("mvp_matrix");
  range_position = program.uniformLocation("range");
  program.setUniformValue("dataTex",0);
}

void VoxelWidget::generateTriangle(){
  cube.clear();
  cube << QVector3D(0, 0.5, 0) << QVector3D(0.5, -0.5, 0) << QVector3D(-0.5, -0.5, 0);
  indices.clear();
  indices << 0 << 1 << 2;
}

void VoxelWidget::generateCube(QVector3D center, qreal radius){
  cube.clear();
  cube.append(radius*QVector3D(-1.0f, -1.0f,  -1.0f)+center);
  cube.append(radius*QVector3D( 1.0f, -1.0f,  -1.0f)+center);
  cube.append(radius*QVector3D(1.0f, -1.0f,  1.0f)+center);
  cube.append(radius*QVector3D(-1.0f, -1.0f,  1.0f)+center);

  cube.append(radius*QVector3D(-1.0f, 1.0f,  -1.0f)+center);
  cube.append(radius*QVector3D( 1.0f, 1.0f,  -1.0f)+center);
  cube.append(radius*QVector3D(1.0f, 1.0f,  1.0f)+center);
  cube.append(radius*QVector3D(-1.0f, 1.0f,  1.0f)+center);

  indices.clear();
  indices << 3 << 2 << 7 << 6 << 2 << 7; //front
  indices << 2 << 1 << 6 << 5 << 1 << 6;//right
  indices << 1 << 0 << 5 << 4 << 0 << 5;//back
  indices << 0 << 3 << 4 << 7 <<  3 << 4;//left
  indices << 7 << 6 << 4 << 5 << 6 << 4;//top
  indices << 0 << 1 << 3 << 2 << 1 << 3;//bottom
}

void VoxelWidget::initBuffers(){
  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);
  glGenBuffers(1, &arrayBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, arrayBuffer);
  glBufferData(GL_ARRAY_BUFFER, cube.count()*sizeof(cube[0]), cube.data(), GL_STATIC_DRAW);
  glGenBuffers(1, &indexBuffer);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.count()*sizeof(indices[0]), indices.data(), GL_STATIC_DRAW);
  glEnableVertexAttribArray(11);
  glVertexAttribPointer(11, 3, GL_FLOAT,GL_FALSE,0, 0);
}

void VoxelWidget::keyPressEvent(QKeyEvent *event){
  switch(event->key()){//multiplekeys at once?
    case(Qt::Key_Z):
      translation +=  QVector3D(0,0,0.1);
      break;
    case(Qt::Key_X):
      translation -=  QVector3D(0,0,0.1);
      break;
    case(Qt::Key_Up):
      translation +=  QVector3D(0,0.1,0);
      break;
    case(Qt::Key_Down):
      translation +=  QVector3D(0,-0.1,0);
      break;
    case(Qt::Key_Left):
      translation -=  QVector3D(0.1,0,0);
      break;
    case(Qt::Key_Right):
      translation +=  QVector3D(0.1,0,0);
      break;
    case(Qt::Key_Q):
      rotations += QVector3D(1,0,0);
      while(rotations.x() >= 360)
        rotations -= QVector3D(360,0,0);
      break;
    case(Qt::Key_A):
      rotations -= QVector3D(1,0,0);
      while(rotations.x() < 0)
        rotations += QVector3D(360,0,0);
      break;
    case(Qt::Key_W):
      rotations += QVector3D(0,1,0);
      while(rotations.x() >= 360)
        rotations -= QVector3D(0,360,0);
      break;
    case(Qt::Key_S):
      rotations -= QVector3D(0,1,0);
      while(rotations.x() < 0)
        rotations += QVector3D(0,360,0);
      break;
    case(Qt::Key_E):
      rotations += QVector3D(0,0,1);
      while(rotations.x() >= 360)
        rotations -= QVector3D(0,0,360);
      break;
    case(Qt::Key_D):
      rotations -= QVector3D(0,0,1);
      while(rotations.x() < 0)
        rotations += QVector3D(0,0,360);
      break;
    case(Qt::Key_Space):
      std::cout << "Reseting!" <<std::endl;
      translation = QVector3D(0,0,-3);
      rotations = QVector3D(45,45,45);
      scale = 0.5;
      break;
    }
    setModelView();
}

void VoxelWidget::createData(unsigned x, unsigned y, unsigned z){
  data_x = x; data_y = y; data_z = z;
  data.clear();
  data.resize(x*y*z);
  for(unsigned u = 0; u < z; u++)
    for(unsigned v = 0; v < y; v++)
      for(unsigned w = 0; w < x; w++)
        data[u*y*x+v*x+w] = qrand()&1? 1.0 : 0;// fabs(sin((10.*(double)u)/z)*sin(4.*((double)v)/y)*sin((16.*(double)w)/x));
}

void VoxelWidget::loadData(){
  dataTex->destroy();
  dataTex->create();
  dataTex->bind(0);
  dataTex->setFormat(QOpenGLTexture::R32F);
  dataTex->setSize(data_x, data_y, data_z);
  dataTex->allocateStorage();
  dataTex->setData(QOpenGLTexture::Red, QOpenGLTexture::Float32, data.data());
  dataTex->setMagnificationFilter(QOpenGLTexture::Linear);//Do not interpolate when zooming in.
  dataTex->setMinificationFilter(QOpenGLTexture::Linear);
  this->update();
}
