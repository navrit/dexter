#include "qcstmglplot.h"
#include <iostream>

QCstmGLPlot::QCstmGLPlot(QWidget* &parent){
	this->setParent(parent);
	dataTex = new QOpenGLTexture(QOpenGLTexture::Target2DArray);
	gradientTex = new QOpenGLTexture(QOpenGLTexture::Target1D);
	this->setFocusPolicy(Qt::WheelFocus);
	this->setMouseTracking(true);
}

QCstmGLPlot::~QCstmGLPlot()
{
	delete dataTex;
	delete gradientTex;
}

void QCstmGLPlot::initializeLocations(){
	orientationLoc = program.attributeLocation("orientationIn");
	offsetAttributeLoc =  program.attributeLocation("offsetsIn");
	textureCoordsLoc = program.attributeLocation("textureCoordsIn");
	squareLoc = program.attributeLocation("verticesIn");

	layerLoc = program.uniformLocation("layer");
	zoomLoc = program.uniformLocation("zoom");
	offsetLoc = program.uniformLocation("globalOffset");
	clampLoc = program.uniformLocation("clampRange");
	aspectRatioLoc = program.uniformLocation("aspectRatio");
	texLoc = program.uniformLocation("tex");
	gradientLoc = program.uniformLocation("gradient");
}

void QCstmGLPlot::initializeShaders(){
	program.addShaderFromSourceFile(QOpenGLShader::Vertex, "./shaders/passthrough.vert");
	program.addShaderFromSourceFile(QOpenGLShader::Fragment, "./shaders/heatmap.frag");
	for(int i = 0; i < program.shaders().length();i++)
		if(program.shaders()[i]->log().length() != 0)
			std::cout << program.shaders()[i]->log().toStdString();
	program.link();
	if(program.log().length() != 0)
		std::cout << program.log().toStdString();
	program.bind();
}

void QCstmGLPlot::initializeTextures(){
	dataTex->create(); //glGenTexture
	dataTex->setFormat(QOpenGLTexture::R32I);
	dataTex->setWrapMode(QOpenGLTexture::ClampToEdge);
	dataTex->bind(0); //bind to unit 0;
	dataTex->setMagnificationFilter(QOpenGLTexture::Nearest);//Do not interpolate when zooming in.
	dataTex->setMinificationFilter(QOpenGLTexture::Nearest);

	gradientTex->create();
	gradientTex->bind(1);
	gradientTex->setFormat(QOpenGLTexture::RGB32F); //TODO: use u8 for color elements?
	gradientTex->setMagnificationFilter(QOpenGLTexture::Linear);
	gradientTex->setMinificationFilter(QOpenGLTexture::Linear);
}

void QCstmGLPlot::initializeVAOsAndVBOs(){
	glGenVertexArrays (1, &vao);
	glBindVertexArray(vao);
	glGenBuffers (4, vbo);

	GLfloat points[2*4];
	points[0] = -1.0f; points[1] = -1.0f;
	points[2] = -1.0f; points[3] = +1.0f;
	points[4] = +1.0f; points[5] = -1.0f;
	points[6]  = +1.0f; points[7] = +1.0f;
	glEnableVertexAttribArray (squareLoc);
	glBindBuffer (GL_ARRAY_BUFFER, vbo[0]);
	glBufferData (GL_ARRAY_BUFFER, sizeof(points), points, GL_STATIC_DRAW);
	glVertexAttribPointer (squareLoc, 2, GL_FLOAT, GL_FALSE, 0, NULL);
	glVertexAttribDivisor(squareLoc, 0);

	glEnableVertexAttribArray(offsetAttributeLoc);
	glBindBuffer (GL_ARRAY_BUFFER, vbo[1]);
	glVertexAttribPointer (offsetAttributeLoc, 2, GL_FLOAT, GL_FALSE, 0, NULL);
	glVertexAttribDivisor(offsetAttributeLoc, 1);

	GLfloat textureCoordinates[2*4];
	//LtRTtB
	textureCoordinates[0] = 0; textureCoordinates[1] = 0;
	textureCoordinates[2] = 0; textureCoordinates[3] =1;
	textureCoordinates[4] = 1; textureCoordinates[5] = 0;
	textureCoordinates[6] =1; textureCoordinates[7] = 1;
	glEnableVertexAttribArray(textureCoordsLoc);
	glBindBuffer (GL_ARRAY_BUFFER, vbo[2]);
	//glBufferData(GL_ARRAY_BUFFER, sizeof(textureCoordinates), textureCoordinates, GL_STATIC_DRAW);
	glBufferData (GL_ARRAY_BUFFER, sizeof(textureCoordinates), textureCoordinates, GL_STATIC_DRAW);
	glVertexAttribPointer (textureCoordsLoc, 2, GL_FLOAT, GL_FALSE, 0, NULL);
	glVertexAttribDivisor(textureCoordsLoc, 0);

	glEnableVertexAttribArray(orientationLoc);
	glBindBuffer(GL_ARRAY_BUFFER, vbo[3]);
	glVertexAttribPointer (orientationLoc, 4, GL_FLOAT, GL_FALSE, 0, NULL);
	glVertexAttribDivisor(orientationLoc, 1);
}

void QCstmGLPlot::initializeGL(){
	initializeOpenGLFunctions();
	QColor bgColor = this->palette().color(this->backgroundRole());
	glClearColor((GLfloat)(bgColor.red()/255.), (GLfloat)(bgColor.green()/255.), (GLfloat)(bgColor.blue()/255.),0.0); //Sets the background Color to match Qt.
	//glEnable (GL_BLEND);
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	initializeShaders();
	initializeLocations();
	initializeTextures();
	initializeVAOsAndVBOs();

	//set the initial uniform values.
	program.setUniformValue(clampLoc, QSizeF(-100, 100));
	program.setUniformValue(texLoc, 0); //set "tex" to  unit 0.
	program.setUniformValue(gradientLoc, 1); //set "gradient" to  unit 1
	setZoom(1);
	initialized = true;
}

void QCstmGLPlot::paintGL(){
	if(gradientChanged){
		gradientChanged = false;
		loadGradient();
	}
	glClear(GL_COLOR_BUFFER_BIT);
	program.bind();
	dataTex->bind(0);
	glBindVertexArray (vao);
	glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4,offsets.size());
}

void QCstmGLPlot::resizeGL(int w, int h){
	program.bind();
	double ratioX = ((double)nx)/w;
	double ratioY = ((double)ny)/h;
	double scaleFactor = 1.0/(ratioX >  ratioY? ratioX:ratioY);
	baseSizeX = ratioX*scaleFactor; baseSizeY = ratioY*scaleFactor;
	program.setUniformValue(aspectRatioLoc, baseSizeX, baseSizeY);
	//program.setUniformValue(resolutionLoc, (float)w, (float)h);
	recomputeBounds();
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
}

void QCstmGLPlot::setGradient(Gradient *gradient){
	this->gradient = gradient;
	gradientChanged = true;
	update();
}

void QCstmGLPlot::setAlphaBlending(bool setOn){
	if(!initialized)
		return;
	this->makeCurrent();
	if(setOn)
		glEnable (GL_BLEND);
	else
		glDisable(GL_BLEND);
	this->update();
}

/*
void QCstmGLPlot::setSize(int nx, int ny){
	this->nx=nx; this->ny = ny;
	//emit size_changed(QPoint(nx, ny));
	recomputeBounds();
}
*/

void QCstmGLPlot::readData(Dataset &data){//TODO: only update textures.
	if(!initialized)
		return;
	QRectF bounding = data.computeBoundingBox();
	program.bind();
	float baseOffsetX = bounding.center().x();
	float baseOffsetY = bounding.center().y();
	this->setOffset(0.5-baseOffsetX, 0.5-baseOffsetY);
	this->setZoom(1.0/(bounding.width() > bounding.height()? bounding.width() : bounding.height()));

	readOrientations(data);
	readLayouts(data);
	populateTextures(data);
	this->update();
}

void QCstmGLPlot::populateTextures(Dataset &data){
	if(!initialized)
		return;
	this->makeCurrent();
	if(dataTex->isCreated()){
		dataTex->destroy();
	}
	dataTex->create();
	dataTex->bind(0); //bind to unit 0;
	dataTex->setFormat(QOpenGLTexture::R32I);
	dataTex->setWrapMode(QOpenGLTexture::Repeat);
	dataTex->setLayers(data.getFrameCount()*data.getLayerCount());
	nx = data.x();
	ny  = data.y();
	dataTex->setSize(nx, ny);//TODO: set to nx, ny
	dataTex->allocateStorage();
	for(int i = 0; i < data.getLayerCount();i++){
		for(int j = 0; j < data.getFrameCount();j++){
			int *frame =     data.getFrameAt(j,i);
			dataTex->setData(0,i*data.getFrameCount()+j,QOpenGLTexture::Red_Integer,QOpenGLTexture::Int32, frame);
		}
	}
	dataTex->setMagnificationFilter(QOpenGLTexture::Nearest);//Do not interpolate when zooming in.
	dataTex->setMinificationFilter(QOpenGLTexture::Nearest);
}

void QCstmGLPlot::readLayouts(Dataset &data){
	QVector<QPoint> layout = data.getLayoutVector();
	offsets.resize(layout.size());
	for(int i = 0; i < offsets.size();i++)
		offsets[i] = QVector2D(layout[i].x()*2, layout[i].y()*2);

	glBindVertexArray (vao);
	//glEnableVertexAttribArray (positionLoc);
	glBindBuffer (GL_ARRAY_BUFFER, vbo[1]);
	glBufferData (GL_ARRAY_BUFFER, offsets.size()*sizeof(*offsets.data()), offsets.data(), GL_STATIC_DRAW);
	//setZoom(1);
	//glVertexAttribPointer (positionLoc, 2, GL_FLOAT, GL_FALSE, 0, NULL);
}

void QCstmGLPlot::readOrientations(Dataset &data){
	QVector<int> orientations = data.getOrientationVector();
	QVector<GLfloat> orientationsGL(orientations.size()*4);
	for(int i = 0; i < orientations.size();i++){
		GLfloat x = 2*(orientations[i]&1)-1;
		GLfloat y = 1-(orientations[i]&2);//      QVector2D(1,1/*1-2*(orientations[i]&1), 1-orientations[i]&2*/ );//little hack: first bit of orientations RtL or LtR, second if TtB or BtT.
		if(orientations[i]&4){
			orientationsGL[i*4+0]  = 0;
			orientationsGL[i*4+1]  = y;
			orientationsGL[i*4+2]  = x;
			orientationsGL[i*4+3]  = 0;
		}else{
			orientationsGL[i*4+0]  = x;
			orientationsGL[i*4+1]  = 0;
			orientationsGL[i*4+2]  = 0;
			orientationsGL[i*4+3]  = y;
		}
	}
	glBindVertexArray (vao);
	glBindBuffer (GL_ARRAY_BUFFER, vbo[3]);
	glBufferData (GL_ARRAY_BUFFER, orientationsGL.size()*sizeof(*orientationsGL.data()), orientationsGL.data(), GL_STATIC_DRAW);

}

void QCstmGLPlot::setRange(QCPRange range){
	if(!initialized)
		return;
	this->makeCurrent();
	program.bind();
	program.setUniformValue(clampLoc, QSizeF(range.lower, range.upper));
	this->update();
}

void QCstmGLPlot::setActive(int layer){
	this->makeCurrent();
	program.bind();
	program.setUniformValue(layerLoc, layer*offsets.length());
	this->update();
}

void QCstmGLPlot::setZoom(float newZoom){
	zoom = newZoom;
	program.bind();
	program.setUniformValue(zoomLoc, zoom/scaleFactor);//TODO:scalefactor.
	this->update();
	emit zoom_changed(zoom);
}

void QCstmGLPlot::setRange(int min, int max){
	program.bind();
	program.setUniformValue(clampLoc, QSize(min, max));
	this->update();
}

void QCstmGLPlot::setOffset(GLfloat x, GLfloat y){
	offsetX = x; offsetY = y;
	program.bind();
	//qDebug() << "Offset =" << offsetX << "," << offsetY;
	program.setUniformValue(offsetLoc, offsetX,  offsetY);
	update();
	recomputeBounds();
	//emit offset_changed(QPointF(offsetX, offsetY));
}

void QCstmGLPlot::recomputeBounds(){
	GLfloat x_low = -1.0, x_high = 1.0;
	GLfloat y_low= -1.0, y_high = 1.0;
	x_low -= offsetX;x_high -= offsetX;
	y_low  -= offsetY; y_high -= offsetY;
	x_low /= (zoom*baseSizeX); x_high /=  (zoom*baseSizeX);
	y_low /= (zoom*baseSizeY);y_high /=  (zoom*baseSizeY);
	x_low  = (1.0+x_low)*0.5*nx;x_high  = (1.0+x_high)*0.5*nx;
	y_low  = (1.0+y_low)*0.5*ny;y_high  = (1.0+y_high)*0.5*ny;
	emit bounds_changed(QRectF(x_low, y_low, x_high, y_high));
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
	//truePixelX = truePixelX >= nx? nx-1 : truePixelX < 0? 0 : truePixelX;
	//truePixelY = truePixelY >= ny? ny-1 : truePixelY < 0? 0 : truePixelY;
	return QPoint(truePixelX, truePixelY);
}

void QCstmGLPlot::wheelEvent(QWheelEvent *event){
	QPoint angle = event->angleDelta(); //Can use high-res tracking on e.g. macbooks, see documentation.
	double deltaZoom = (1+0.125*angle.y()/120.);
	float newZoom = zoom*deltaZoom;
	double oldX = 2*((double)event->x())/this->width()-1, oldY = -(2*((double)event->y())/this->height()-1);
	oldX -= offsetX; oldY -= offsetY;
	if(newZoom < .1){
		newZoom = .1;
		deltaZoom = .1/zoom;
	}
	GLfloat displacementX = oldX*(1-deltaZoom); GLfloat displacementY = oldY*(1-deltaZoom);
	//qDebug() << "got zoom event, newZoom = " <<  newZoom;
	setZoom(newZoom);
	addOffset(displacementX, displacementY);
	event->accept();
	//paintGL();
	update();
}

void QCstmGLPlot::mousePressEvent(QMouseEvent *event){


	// Change to the hand cursor
	if(event->buttons()== Qt::LeftButton){
		this->setCursor(Qt::ClosedHandCursor);
		clickedLocation = event->pos();
		clicked = true;
	} else if ( event->buttons()== Qt::RightButton ) { // Describe an area
		// Start point
		clickedLocation = event->pos();
		rightClicked = true;
		//qDebug() << "click: "<< pixelAt(clickedLocation).x() << "," << pixelAt(clickedLocation).y();
	}

}

void QCstmGLPlot::mouseReleaseEvent(QMouseEvent * event){

	this->setCursor(Qt::ArrowCursor);

	// End point
	clickReleaseLocation = event->pos();

	if(clicked) clicked = false;

	if(rightClicked) {

		// if in the same pixel
		if (clickedLocation == clickReleaseLocation) {
			emit( pixel_selected( pixelAt(event->pos()), event->globalPos()) );
		} else {
			emit( region_selected( pixelAt(clickedLocation), pixelAt(clickReleaseLocation), event->globalPos()) );
		}

		rightClicked = false;
		event->accept(); // Done with the event, this has been handled and nobody else gets it.
	}

	//qDebug() << "click release: " << pixelAt(clickReleaseLocation).x() << "," << pixelAt(clickReleaseLocation).y();

}

void QCstmGLPlot::keyPressEvent(QKeyEvent *event){//Doesn't really work that well for controls.
	//Can't seem to handle multiple presses at once and stutters quite badly.
	const GLfloat speed = zoom*0.05;
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

/*
void QCstmGLPlot::contextMenuEvent(QContextMenuEvent *event){
	emit(pixel_selected(pixelAt(event->pos()), event->globalPos()));
	event->accept();
}
*/

void QCstmGLPlot::mouseMoveEvent(QMouseEvent *event){//TODO: verify dragspeed should be so that the same pixel stays under the cursor at all times.

	if(clicked){

		float translationX = 2*((double)event->x()-clickedLocation.x())/this->width();
		float translationY = -2*((double)event->y()-clickedLocation.y())/this->height();
		clickedLocation = event->pos();
		addOffset(translationX, translationY);

	} else if(rightClicked) {

		// If dragging the mouse draw the rectangle

	}
	//QPointF pixelHovered = pixelAt(event->pos());
	emit(hovered_pixel_changed(pixelAt(event->pos())));// QString("%1, %2, ??").arg(pixelHovered.x()).arg(pixelHovered.y())));
}
