#include "qcstmglplot.h"
#include <QPainter>
#include <iostream>
#include <QDirIterator>
QCstmGLPlot::QCstmGLPlot(QWidget* &parent){
  this->setParent(parent);
  program =  new QOpenGLShaderProgram[3];
  dataTex = new QOpenGLTexture(QOpenGLTexture::Target2DArray);
  gradientTex = new QOpenGLTexture(QOpenGLTexture::Target1D);
  //atlas.addAscii();
  this->setFocusPolicy(Qt::WheelFocus);
  this->setMouseTracking(true);
}

QCstmGLPlot::~QCstmGLPlot()
{
  delete[] program;
  delete dataTex;
  delete gradientTex;
}

void QCstmGLPlot::initializeGL(){
  points[0] = -1.0f; points[1] = -1.0f; points[2] = -1.0f; points[3] = +1.0f; points[4] = +1.0f; points[5] = -1.0f; points[6]  = +1.0f; points[7] = +1.0f;
  textureCoordinates[0] = 0.0f; textureCoordinates[1] = 0.0f; textureCoordinates[2] = 0.0f; textureCoordinates[3] = +1.0f;
  textureCoordinates[4] = +1.0f; textureCoordinates[5] = 0.0f; textureCoordinates[6]  = +1.0f; textureCoordinates[7] = +1.0f;
  initializeOpenGLFunctions();
  QColor bgColor = this->palette().color(this->backgroundRole());
  glClearColor((GLfloat)(bgColor.red()/255.), (GLfloat)(bgColor.green()/255.), (GLfloat)(bgColor.blue()/255.),1.0); //Sets the background Color to match Qt.
  glEnable (GL_BLEND); glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glGenBuffers (2, vbo);

  glGenVertexArrays (1, vao);

  glBindVertexArray (vao[0]);
  glEnableVertexAttribArray (0);
  glBindBuffer (GL_ARRAY_BUFFER, vbo[0]);
  glBufferData (GL_ARRAY_BUFFER, sizeof(points), points, GL_STATIC_DRAW);
  glVertexAttribPointer (0, 2, GL_FLOAT, GL_FALSE, 0, NULL);

  glEnableVertexAttribArray (1);
  glBindBuffer (GL_ARRAY_BUFFER, vbo[1]);
  glBufferData (GL_ARRAY_BUFFER, sizeof(textureCoordinates), textureCoordinates, GL_STATIC_DRAW);
  glVertexAttribPointer (1, 2, GL_FLOAT, GL_FALSE, 0, NULL);

  nLayers  = 1;

  grabShadersFrom("./shaders");
  program[0].bindAttributeLocation("position",0);
  program[0].bindAttributeLocation("texCoordsIn",1);
  program[0].addShader(vertexShaders["passthrough"]);
  program[0].addShader(fragmentShaders["heatmap"]);
  program[0].link();
  if(program[0].log().length() != 0)
    std::cout << program[0].log().toStdString();
  program[0].bind();

  zoomLoc = program[0].uniformLocation("zoom");
  offsetLoc = program[0].uniformLocation("offset");
  clampLoc = program[0].uniformLocation("clampRange");
  GLint **frames = new GLint*[nLayers];
  for(int k = 0; k < nLayers; k++){
      frames[k] = new GLint[nx*ny];
      for(int i = 0; i < ny;i++)
        for(int j = 0;j < nx;j++){
          frames[k][i*nx+j] = 100*sin(1.0/ny*i*(k+1)*6.28)*sin(1.0/nx*j*(k+1)*6.28);
          //printf("%2d ", frames[k][i*nx+j]);
        }
      //printf("\n");
      //QOpenGLWidget::initializeGL();
   }

  //loadGradient();
  loadFrames(nx,ny,nLayers, frames);
  program[0].setUniformValue("layer",0);
  program[0].setUniformValue(clampLoc, QSizeF(-100, 100));
  program[0].setUniformValue("tex", 0); //set "tex" to  unit 0.
  program[0].setUniformValue("gradient", 1); //set "gradient" to  unit 0.

  for(int i = 0; i < nLayers; i++)
    delete[] frames[i];
  delete[] frames;
  setZoom(1);
}

void QCstmGLPlot::paintGL(){
  if(gradientChanged){
      gradientChanged = false;
      loadGradient();
    }
  glClear(GL_COLOR_BUFFER_BIT);

  program[0].bind();
  glBindVertexArray (vao[0]);
  dataTex->bind(0);
  glDrawArrays (GL_TRIANGLE_STRIP, 0, 4);

}

void QCstmGLPlot::resizeGL(int w, int h){
  program[0].bind();
  double ratioX = ((double)nx)/w;
  double ratioY = ((double)ny)/h;
  double scaleFactor = 1.0/(ratioX >  ratioY? ratioX:ratioY);
  baseSizeX = ratioX*scaleFactor; baseSizeY = ratioY*scaleFactor;
  program[0].setUniformValue("aspectRatio", baseSizeX, baseSizeY);
  qDebug() << "(" << w << "," << h << ") --> (" << nx << "," << ny << ") = " << baseSizeX << ", " << baseSizeY;
  program[0].setUniformValue("resolution", (float)w, (float)h);
}

void QCstmGLPlot::grabShadersFrom(QString directory){ //TODO: don't compile until used in a porgram.
  QDirIterator dirIt(directory,QDirIterator::Subdirectories);//TODO: give shader names relative to base folder?
  while (dirIt.hasNext()) {
      dirIt.next();
      if (QFileInfo(dirIt.filePath()).isFile())
          if (QFileInfo(dirIt.filePath()).suffix() == "frag"){
              QOpenGLShader *newShader = new QOpenGLShader(QOpenGLShader::Fragment,this);
              newShader->compileSourceFile(dirIt.filePath());
              if(newShader->log().length() != 0)
                std::cout << dirIt.fileName().toUpper().toStdString() << ": " <<  newShader->log().toStdString();
               if(newShader->isCompiled())
                  fragmentShaders[dirIt.fileInfo().completeBaseName()] = newShader;
        }
      if (QFileInfo(dirIt.filePath()).suffix() == "vert"){
        QOpenGLShader *newShader = new QOpenGLShader(QOpenGLShader::Vertex, this);
        newShader->compileSourceFile(dirIt.filePath());
        if(newShader->log().length() != 0)
          std::cout << dirIt.fileName().toUpper().toStdString() << ": " << newShader->log().toStdString();
        if(newShader->isCompiled())
            vertexShaders[dirIt.fileInfo().completeBaseName()] = newShader;
       }
    }
}

void QCstmGLPlot::loadGradient(){
  if(gradientTex->isCreated())
    gradientTex->destroy();
  gradientTex->create();
  gradientTex->bind(1);
  gradientTex->setFormat(QOpenGLTexture::RGB32F); //TODO: use u8 for color elements?
  gradientTex->setSize(gradient->getLength());
  gradientTex->allocateStorage();
  gradientTex->setData(QOpenGLTexture::RGB, QOpenGLTexture::Float32, gradient->getArray());
  gradientTex->setWrapMode(QOpenGLTexture::ClampToEdge);//Clamp to Border not valid for 1D?

  gradientTex->setMagnificationFilter(QOpenGLTexture::Linear);//Do not interpolate when zooming in.
  gradientTex->setMinificationFilter(QOpenGLTexture::Linear);
  //program[0].bind();
  //program[0].setUniformValue("gradient",1); //set "tex" to texture unit 0.
}
void QCstmGLPlot::setGradient(Gradient *gradient){
  this->gradient = gradient;
  gradientChanged = true;
  update();
}

void QCstmGLPlot::setData(QVector<int *> layers){ //TODO: Set size functions, only grow nLayes when necessary.
  this->makeCurrent();
  if(dataTex->isCreated()){
      dataTex->destroy();
    }
  dataTex->setFormat(QOpenGLTexture::R32I);
  dataTex->setWrapMode(QOpenGLTexture::ClampToEdge);

  dataTex->create(); //glGenTexture
  dataTex->setLayers(layers.count());
  dataTex->setSize(this->nx, this->ny);
  dataTex->bind(0); //bind to unit 0;
  dataTex->allocateStorage();
  for(int i = 0; i < layers.count();i++){
    dataTex->setData(0,i,QOpenGLTexture::Red_Integer,QOpenGLTexture::Int32, layers[i]);
  }
  dataTex->setMagnificationFilter(QOpenGLTexture::Nearest);//Do not interpolate when zooming in.
  dataTex->setMinificationFilter(QOpenGLTexture::Nearest);
  this->update();
}

void QCstmGLPlot::loadFrames(int nx, int ny, int nFrames, GLint **data ){
  this->makeCurrent();
  //this->makeCurrent();
  if(dataTex->isCreated()){
      dataTex->destroy();
    }
  dataTex->setFormat(QOpenGLTexture::R32I);
  dataTex->create(); //glGenTexture
  dataTex->setLayers(nFrames);
  dataTex->setSize(nx, ny);
  dataTex->bind(0); //bind to unit 0;
  dataTex->setWrapMode(QOpenGLTexture::ClampToEdge);
  dataTex->allocateStorage();
  dataTex->setData(QOpenGLTexture::Red_Integer, QOpenGLTexture::Int32, data[0]);
  dataTex->setMagnificationFilter(QOpenGLTexture::Nearest);//Do not interpolate when zooming in.
  dataTex->setMinificationFilter(QOpenGLTexture::Nearest);
  this->update();
}

void QCstmGLPlot::setRange(QCPRange range){
    program[0].bind();
    program[0].setUniformValue(clampLoc, QSizeF(range.lower, range.upper));
    this->update();
}

void QCstmGLPlot::setActive(int layer){
  this->makeCurrent();
  program[0].bind();
  program[0].setUniformValue("layer",layer);
  this->update();
}

void QCstmGLPlot::setZoom(float newZoom){
  zoom = newZoom;
  program[0].bind();
  program[0].setUniformValue(zoomLoc, zoom);
  this->update();
}

void QCstmGLPlot::setRange(int min, int max){
  program[0].bind();
  program[0].setUniformValue(clampLoc, QSize(min, max));
  this->update();
}

void QCstmGLPlot::setOffset(GLfloat x, GLfloat y){
  offsetX = x; offsetY = y;
  program[0].bind();
  program[0].setUniformValue(offsetLoc, offsetX,  offsetY);
  update();
}

void QCstmGLPlot::addOffset(GLfloat x, GLfloat y){
  setOffset(offsetX+x, offsetY+y);
}

QPoint QCstmGLPlot::pixelAt(QPoint position){
  GLfloat pixelX = 2.0*position.x()/this->width()-1.0;
  GLfloat pixelY = -2.0*position.y()/this->height()+1.0;
  pixelX -= offsetX; pixelY -= offsetY;
  pixelX /= (zoom*baseSizeX); pixelY /= (zoom*baseSizeY);
  pixelX = (1.0+pixelX)*0.5; pixelY = (1.0+pixelY)*0.5;
  int truePixelX = pixelX*nx;
  int truePixelY = pixelY*ny;
  truePixelX = truePixelX >= nx? nx-1 : truePixelX < 0? 0 : truePixelX;
  truePixelY = truePixelY >= ny? ny-1 : truePixelY < 0? 0 : truePixelY;
  return QPoint(truePixelX, truePixelY);
}

void QCstmGLPlot::wheelEvent(QWheelEvent *event){
  QPoint angle = event->angleDelta(); //Can use high-res tracking on e.g. macbooks, see documentation.
  double deltaZoom = (1+0.125*angle.y()/120.);
  float newZoom = zoom*deltaZoom;
  double oldX = 2*((double)event->x())/this->width()-1, oldY = -(2*((double)event->y())/this->height()-1);
  oldX -= offsetX; oldY -= offsetY;
  if(newZoom < 1.0){
    newZoom = 1.0;
    deltaZoom = 1/zoom;
  }
  GLfloat displacementX = oldX*(1-deltaZoom); GLfloat displacementY = oldY*(1-deltaZoom);
  qDebug() << "got zoom event, newZoom = " <<  newZoom;
  setZoom(newZoom);
  addOffset(displacementX, displacementY);
  event->accept();
  //paintGL();
  update();
}

void QCstmGLPlot::keyPressEvent(QKeyEvent *event){//Doesn't really work that well for controls.
  //Can't seem to handle multiple presses at once and stutters quite badly.
  const GLfloat speed = 0.05;
  switch(event->key()){
    case Qt::Key_Left:
      addOffset(+speed, 0);
      event->accept();
      break;
    case Qt::Key_Right:
      addOffset(-speed, 0);
      event->accept();
      break;
    case Qt::Key_Up:
      addOffset(0, -speed);
      event->accept();
      break;
    case Qt::Key_Down:
      addOffset(0, +speed);
      event->accept();
      break;
    case Qt::Key_Space:
      setOffset(0,0);
      setZoom(1.0);
      event->accept();
    default:
      event->ignore();
    }
}

void QCstmGLPlot::contextMenuEvent(QContextMenuEvent *event){
  emit(pixel_selected(pixelAt(event->pos()), event->globalPos()));
  event->accept();
}

void QCstmGLPlot::mouseMoveEvent(QMouseEvent *event){//TODO: verify dragspeed should be so that the same pixel stays under the cursor at all times.
  if(clicked){
    float translationX = 2*((double)event->x()-clickedLocation.x())/this->width();
    float translationY = -2*((double)event->y()-clickedLocation.y())/this->height();
    clickedLocation = event->pos();
    addOffset(translationX, translationY);
  }
  //QPointF pixelHovered = pixelAt(event->pos());
  emit(hovered_pixel_changed(pixelAt(event->pos())));// QString("%1, %2, ??").arg(pixelHovered.x()).arg(pixelHovered.y())));
}
