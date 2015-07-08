#include "dataset.h"
#include <QDataStream>
#include <QDebug>

#include <iostream>

using namespace std;


Dataset::Dataset(int x, int y, int framesPerLayer)
{
	m_nx = x; m_ny = y;
	m_nFrames = 0;
	setFramesPerLayer(framesPerLayer);
}

Dataset::Dataset() : Dataset(1,1,1){}

Dataset::~Dataset()
{
	delete correction;
	clear();
}

void Dataset::loadCorrection(QByteArray serialized) {
	delete correction;
	correction  = new Dataset(0,0,0);
	correction->fromByteArray(serialized);//TODO: add error checking on correction to see if it is relevant to the data.
}

int64_t Dataset::getTotal(int threshold){
	int index = thresholdToIndex(threshold);
	if(index == -1)
		return 0;
	int64_t count = 0;
	for(int j = 0; j < getPixelsPerLayer(); j++)
		count += m_layers[index][j];
	return count;
}

uint64_t Dataset::getActivePixels(int threshold){
	int index = thresholdToIndex(threshold);
	if(index == -1)
		return 0;
	uint64_t count  =0;
	for(int j = 0; j <getPixelsPerLayer(); j++){
		if(0 != m_layers[index][j])
			count++;
	}
	return count;
}

Dataset::Dataset( const Dataset& other ): m_boundingBox(other.m_boundingBox), m_frameLayouts(other.m_frameLayouts), m_frameOrientation(other.m_frameOrientation), m_thresholdsToIndices(other.m_thresholdsToIndices), m_layers(other.m_layers){
	m_nx = other.x(); m_ny = other.y();
	m_nFrames = other.getFrameCount();
	for(int i = 0; i < m_layers.size(); i++){
		m_layers[i] = new int[getPixelsPerLayer()];
		for(int j = 0; j < getPixelsPerLayer(); j++)
			m_layers[i][j] = other.m_layers[i][j];
	}
}

Dataset& Dataset::operator =( const Dataset& rhs){
	Dataset copy(rhs);
	std::swap(this->m_layers, copy.m_layers);
	return *this;
}

void Dataset::zero(){
	for(int i = 0; i < m_layers.size(); i++){
		for(int j = 0; j < getPixelsPerLayer(); j++)
			m_layers[i][j] = 0;
	}
}

int Dataset::getLayerIndex(int threshold){
	int layerIndex = thresholdToIndex(threshold);
	if(layerIndex == -1)
		layerIndex = newLayer(threshold);
	return layerIndex;
}

QByteArray Dataset::toByteArray(){
	QByteArray ret(0);
	ret += QByteArray::fromRawData((const char*)&m_nx, (int)sizeof(m_nx));
	ret += QByteArray::fromRawData((const char*)&m_ny, (int)sizeof(m_ny));
	ret += QByteArray::fromRawData((const char*)&m_nFrames, (int)sizeof(m_nFrames));
	int layerCount = m_layers.size();
	ret += QByteArray::fromRawData((const char*)&layerCount, (int)sizeof(layerCount));
	ret += QByteArray::fromRawData((const char*)m_frameLayouts.data(),(int)(m_nFrames*sizeof(*m_frameLayouts.data())));
	ret += QByteArray::fromRawData((const char*)m_frameOrientation.data(), (int)(m_nFrames*sizeof(*m_frameOrientation.data())));
	QList<int> keys = m_thresholdsToIndices.keys();
	ret += QByteArray::fromRawData((const char*)keys.toVector().data(),(int)(keys.size()*sizeof(int))); //thresholds
	for(int i = 0; i < keys.length(); i++)
		ret += QByteArray::fromRawData((const char*)this->getLayer(keys[i]), (int)(sizeof(float)*getLayerSize()));
	return ret;
}

void Dataset::calcBasicStats(QPoint pixel_init, QPoint pixel_end) {

	QList<int> keys = m_thresholdsToIndices.keys();

	QSize isize = QSize(computeBoundingBox().size().width()*this->x(), computeBoundingBox().size().height()*this->y());


	// Region of interest
	QRectF RoI;
	RoI.setRect(pixel_init.x(), pixel_init.y(), pixel_end.x() - pixel_init.x(),  pixel_end.y() - pixel_init.y() );

	cout << "-- Basic stats -- ";
	for(int i = 0; i < keys.length(); i++) {
		cout << "\t" << keys[i];
	}
	cout << endl;

	// Mean
	cout << "   mean\t\t";
	for(int i = 0; i < keys.length(); i++) {

		int* currentLayer = getLayer(keys[i]);

		// Calc mean on the interesting pixels
		double mean = 0.;
		int nMean = 0.;
		for(int j = 0; j < getPixelsPerLayer(); j++) {
			// See if the pixel is inside the region
			QPointF pix = XtoXY(j, isize.width());
			if ( RoI.contains( pix ) ) {
				mean += currentLayer[j];
				nMean++;
			}
		}
		if(nMean != 0) mean /= nMean;

		//cout.precision(1);
		cout << "\t" << mean;

	}

	cout << endl;

}

QPointF Dataset::XtoXY(int X, int dimX){
	return QPointF(X % dimX, X/dimX);
}

bool Dataset::isBorderPixel(int p, QSize isize) {

	if (
			p < isize.width() 	// lower edge: 0 --> width-1
			||
			p >= ( isize.width()*isize.height() - isize.width() ) // upper edge: width*height-width --> width*height
			||
			( ( p % (isize.width()-1) ) == 0 ) // right edge
			||
			( ( p %  isize.width()    ) == 0 ) // left edge
	) return true;

	return false;
}

bool Dataset::isBorderPixel(int x, int y, QSize isize) {

	if ( y <= 0 ) return true;					// lower edge
	if ( y >= isize.height() - 1 ) return true;	// upper edge
	if ( x <= 0 ) return true;					// left edge
	if ( x >= isize.width()  - 1 ) return true;	// right edge

	return false;
}

void Dataset::applyHighPixelsInterpolation(){

	QList<int> keys = m_thresholdsToIndices.keys();
	// computeBoundingBox().size().width() 	--> gives the number of chips
	// this->x() 							--> gives the number of pixels per chip
	QSize isize = QSize( computeBoundingBox().size().width()*this->x(), computeBoundingBox().size().height()*this->y() );

	double meanMultiplier = 2.0; // TODO
	for(int i = 0; i < keys.length(); i++) {

		int* currentLayer = getLayer(keys[i]);

		double mean = 0.;
		for(int j = 0; j < getPixelsPerLayer(); j++) {

			// skip the borders
			if ( isBorderPixel(j, isize) ) continue;

			// calculate the mean
			mean += currentLayer[j];

		}
		// no borders !
		mean /= ( getPixelsPerLayer() - 2*isize.width() - 2*isize.height() );

		cout << "[" << i << "]" << "mean = " << mean << endl;

		for(int j = 0; j < getPixelsPerLayer(); j++) {

			// skip the borders
			if ( isBorderPixel(j, isize) ) continue;

			if ( currentLayer[j] > meanMultiplier*mean
					&&
					currentLayer[j+1] < meanMultiplier*mean
					&&
					currentLayer[j-1] < meanMultiplier*mean
					&&
					currentLayer[j+isize.width()] < meanMultiplier*mean
					&&
					currentLayer[j-isize.width()] < meanMultiplier*mean
			) {
				currentLayer[j] =
						(
								(currentLayer[j+1] + currentLayer[j-1]) +
								(currentLayer[j+isize.width()] + currentLayer[j-isize.width()])
						) / 4;
			}

		}


	}

}

void Dataset::applyDeadPixelsInterpolation(){

	// The keys are the thresholds
	QList<int> keys = m_thresholdsToIndices.keys();

	QSize isize = QSize(computeBoundingBox().size().width()*this->x(), computeBoundingBox().size().height()*this->y());

	for(int i = 0; i < 1 ; i++) { //keys.length(); i++) {

		int * currentLayer = getLayer(keys[i]);

		for(int y = 0 ; y < isize.height() ; y++) {

			for(int x = 0 ; x < isize.width() ; x++) {

				// is dead pixel
				if ( sample(x, y, keys[i]) == 0 ) {

					// Check first how many of it's neighbors are active
					vector<int> actives = activeNeighbors(x, y, keys[i], isize);

					// Request at least two active neighbors to consider filling up by averaging
					if ( actives.size() >= 2 ) {
						setPixel(x, y, keys[i], vectorAverage( actives ) );
					}

				}

			} // x
		} // y

	} // layers


}

int Dataset::vectorAverage(vector<int> v) {

	double av = 0;
	vector<int>::iterator i  = v.begin();
	vector<int>::iterator iE = v.end();
	for ( ; i != iE ; i++ ) {
		av += *i;
	}
	av /= ( (double)(v.size()) );

	return qRound(av);
}

vector<int> Dataset::activeNeighbors(int x, int y, int thl, QSize isize) {

	vector<int> activeNeighbors;

	// If the pixel is not in the border.  Most probable.
	if ( ! isBorderPixel(x, y, isize) ) {
		if ( sample(x, y+1, thl) > 0 ) activeNeighbors.push_back( sample(x, y+1, thl) );
		if ( sample(x, y-1, thl) > 0 ) activeNeighbors.push_back( sample(x, y-1, thl) );
		if ( sample(x+1, y, thl) > 0 ) activeNeighbors.push_back( sample(x+1, y, thl) );
		if ( sample(x-1, y, thl) > 0 ) activeNeighbors.push_back( sample(x-1, y, thl) );
		return activeNeighbors;
	}

	// Bottom edge
	if ( y == 0 && x > 0 && x < (isize.width()-1) ) {
		if ( sample(x-1,   y, thl) > 0 ) activeNeighbors.push_back( sample(x-1,   y, thl) );
		if ( sample(x  , y+1, thl) > 0 ) activeNeighbors.push_back( sample(x  , y+1, thl) );
		if ( sample(x+1,   y, thl) > 0 ) activeNeighbors.push_back( sample(x+1,   y, thl) );
		return activeNeighbors;
	}
	// Left edge
	if (  y > 0 && y < (isize.height()-1) && x == 0 ) {
		if ( sample(x  , y+1, thl) > 0 ) activeNeighbors.push_back( sample(x  , y+1, thl) );
		if ( sample(x+1,   y, thl) > 0 ) activeNeighbors.push_back( sample(x+1,   y, thl) );
		if ( sample(x  , y-1, thl) > 0 ) activeNeighbors.push_back( sample(x  , y-1, thl) );
		return activeNeighbors;
	}
	// Top edge
	if (  y == (isize.height()-1) && x > 0 && x < (isize.width()-1) ) {
		if ( sample(x-1,   y, thl) > 0 ) activeNeighbors.push_back( sample(x-1,   y, thl) );
		if ( sample(x+1,   y, thl) > 0 ) activeNeighbors.push_back( sample(x+1,   y, thl) );
		if ( sample(x  , y-1, thl) > 0 ) activeNeighbors.push_back( sample(x  , y-1, thl) );
		return activeNeighbors;
	}
	// Right edge
	if (  x == (isize.width()-1) && y > 0 && y < (isize.height()-1) ) {
		if ( sample(x  , y+1, thl) > 0 ) activeNeighbors.push_back( sample(x  , y+1, thl) );
		if ( sample(x  , y-1, thl) > 0 ) activeNeighbors.push_back( sample(x  , y-1, thl) );
		if ( sample(x-1,   y, thl) > 0 ) activeNeighbors.push_back( sample(x-1,   y, thl) );
		return activeNeighbors;
	}

	// Four corners
	if (  (x % (isize.width()-1) == 0)  &&  (y % (isize.height()-1) == 0)  ) {

		if ( x == 0 && y == 0) {  // left bottom
			activeNeighbors.push_back( sample(x+1,   y, thl) );
			activeNeighbors.push_back( sample(x,   y+1, thl) );
			activeNeighbors.push_back( sample(x+1, y+1, thl) );
		} else if ( x == 0 && y == (isize.height()-1) ) {  // left top
			activeNeighbors.push_back( sample(x+1,   y, thl) );
			activeNeighbors.push_back( sample(x,   y-1, thl) );
			activeNeighbors.push_back( sample(x+1, y-1, thl) );
		} else if ( x == (isize.width()-1) && y == (isize.height()-1) ) {  // right top
			activeNeighbors.push_back( sample(x-1,   y, thl) );
			activeNeighbors.push_back( sample(x-1, y-1, thl) );
			activeNeighbors.push_back( sample(x  , y-1, thl) );
		} else if ( x == (isize.width()-1) && y == 0 ) {  // right bottom
			activeNeighbors.push_back( sample(x-1,   y, thl) );
			activeNeighbors.push_back( sample(x-1, y+1, thl) );
			activeNeighbors.push_back( sample(x  , y+1, thl) );
		}
		cout << "+" << endl;
	}

	return activeNeighbors;
}

void Dataset::applyCorrection(){
	if(correction == nullptr)
		return;
	QList<int> keys = m_thresholdsToIndices.keys();
	for(int i = 0; i < keys.length(); i++){
		//double currentTotal = getTotal(keys[i]), correctionTotal = correction->getTotal(keys[i]);
		int* currentLayer = getLayer(keys[i]);
		int* correctionLayer = correction->getLayer(keys[i]);
		if(correctionLayer == nullptr){
			qDebug() << "[WARN] flat-field correction does not contain a threshold" << keys[i];
			continue;
		}

		// Obtain the average of the corrected image
		double averageCorr = 0.;
		double averageCurrent = 0.;
		for(int j = 0; j < getPixelsPerLayer(); j++) {
			if(correctionLayer[j] > 0) averageCorr += ((double)currentLayer[j])/((double)correctionLayer[j]);
			averageCurrent += (double)currentLayer[j];
		}
		averageCorr /= getPixelsPerLayer();
		averageCurrent /= getPixelsPerLayer();

		for(int j = 0; j < getPixelsPerLayer(); j++)
			if(0  != correctionLayer[j]) {
				double numer = averageCurrent*currentLayer[j];
				double den = averageCorr*correctionLayer[j];
				currentLayer[j] = round( numer/den );
			}
	}

}

void Dataset::fromByteArray(QByteArray serialized){
	QDataStream in(&serialized, QIODevice::ReadOnly);
	in.readRawData((char*)&m_nx, (int)sizeof(m_nx));
	in.readRawData((char*)&m_ny, (int)sizeof(m_ny));
	in.readRawData((char*)&m_nFrames, (int)sizeof(m_nFrames));
	setFramesPerLayer(m_nFrames);
	int layerCount;
	in.readRawData((char*)&layerCount, (int)sizeof(layerCount));
	//setLayerCount(layerCount);
	in.readRawData((char*)m_frameLayouts.data(), m_nFrames*(int)sizeof(*m_frameLayouts.data()));
	in.readRawData((char*)m_frameOrientation.data(), m_nFrames*(int)sizeof(*m_frameOrientation.data()));
	QVector<int> keys(layerCount);
	in.readRawData((char*)keys.data(), keys.size()*(int)sizeof(int));
	QVector<int> frameBuffer(m_nx*m_ny);
	for(int i = 0; i < keys.size(); i++){
		for(int j = 0; j < m_nFrames; j++){
			in.readRawData((char*)frameBuffer.data(), (int)sizeof(float)*frameBuffer.size());
			this->setFrame(frameBuffer.data(), j, keys[i]);
		}
	}
}

void Dataset::clear(){
	for(int i =0; i < m_layers.size(); i++){
		delete[] m_layers[i];
	}
	m_layers.clear();
	m_thresholdsToIndices.clear();
	//setFramesPerLayer(1);
}

QRectF Dataset::computeBoundingBox(){
	m_boundingBox.setRect(0,0,0,0);
	int min_x = INT_MAX, min_y = INT_MAX, max_x = INT_MIN, max_y = INT_MIN;
	for(int i =0; i < m_frameLayouts.length();i++){
		if(m_frameLayouts[i].x() < min_x)
			min_x = m_frameLayouts[i].x();
		if(m_frameLayouts[i].y() < min_y)
			min_y = m_frameLayouts[i].y();
		if(m_frameLayouts[i].x() > max_x)
			max_x = m_frameLayouts[i].x();
		if(m_frameLayouts[i].y() > max_y)
			max_y = m_frameLayouts[i].y();
	}
	m_boundingBox.setRect(min_x,min_y, max_x+1, max_y+1);

	return m_boundingBox;
}

int Dataset::newLayer(int threshold){
	m_thresholdsToIndices[threshold] = m_layers.size();
	m_layers.append(new int[getPixelsPerLayer()]);
	for(int j = 0; j < getLayerSize(); j++)
		m_layers.last()[j] = 0;
	return m_layers.size()-1;
}

void Dataset::setFrame(int *frame, int index, int threshold){
	if(!m_thresholdsToIndices.contains(threshold))
		newLayer(threshold);
	int *newFrame = getFrame(index, threshold);
	for(int i = 0 ; i < m_nx*m_ny;i++)
		newFrame[i]= frame[i];
}

void Dataset::sumFrame(int *frame, int index, int threshold){
	if(!m_thresholdsToIndices.contains(threshold))
		newLayer(threshold);
	int *newFrame = getFrame(index, threshold);
	for(int i = 0 ; i < m_nx*m_ny;i++)
		newFrame[i] += frame[i];
}

int* Dataset::getFrame(int index, int threshold){
	if(!m_thresholdsToIndices.contains(threshold))
		return nullptr;
	else
		return &m_layers[thresholdToIndex(threshold)][index*m_nx*m_ny];
}

int* Dataset::getFrameAt(int index, int layer){
	return &m_layers[layer][index*m_nx*m_ny];
}

int Dataset::getContainingFrame(QPoint pixel){
	QPoint layoutSample((pixel.x()+m_nx)/m_nx -1, (pixel.y()+m_ny)/m_ny-1);
	for(int i = 0; i < m_frameLayouts.length();i++){
		if(layoutSample == m_frameLayouts[i])//TODO: orientation messes up sampling!
			return i;
	}
	return -1;
}

QPoint Dataset::getNaturalCoordinates(QPoint pixel, int index){
	int x = pixel.x() % m_nx;
	int y = pixel.y() % m_ny;
	int orientation = m_frameOrientation[index];
	if(!(orientation&1))
		x = m_nx -x-1;
	if(orientation&2)
		y = m_ny -y-1;
	if(orientation&4){
		int tmp = x;
		x = y;
		y = tmp;
	}
	return QPoint(x,y);
}

int Dataset::sample(int x, int y, int threshold){
	int layerIndex = thresholdToIndex(threshold);
	if(layerIndex == -1)
		return 0;
	int frameIndex  = getContainingFrame(QPoint(x,y));
	if(frameIndex == -1)
		return 0;
	int* frame = getFrameAt(frameIndex, layerIndex);
	QPoint coordinate = getNaturalCoordinates(QPoint(x,y), frameIndex);
	return frame[coordinate.y()*m_nx+coordinate.x()];
}

void Dataset::setPixel(int x, int y, int threshold, int val) {

	int layerIndex = thresholdToIndex(threshold);
	if(layerIndex == -1)
		return; // couldn't find a layer, no changes applied.
	int frameIndex  = getContainingFrame(QPoint(x,y));
	if(frameIndex == -1)
		return; // couldn't find the frame, no changes applied.
	int * frame = getFrameAt(frameIndex, layerIndex);
	QPoint coordinate = getNaturalCoordinates(QPoint(x,y), frameIndex);
	// set the value
	frame[coordinate.y()*m_nx+coordinate.x()] = val;

}

int  Dataset::sampleFrameAt(int index, int layer, int x, int y){
	int* frame = getFrameAt(index, layer);
	int orientation = m_frameOrientation[index];
	if(!(orientation&1))
		x = m_nx -x-1;
	if(orientation&2)
		y = m_ny -y-1;
	if(orientation&4){
		int tmp = x;
		x = y;
		y = tmp;
	}
	return frame[y*m_nx+x];//TODO:check math
}

void Dataset::setFramesPerLayer(int newFrameCount){
	int oldFrameCount =m_nFrames;
	m_nFrames = newFrameCount;
	m_frameOrientation.resize(m_nFrames);
	m_frameLayouts.resize(m_nFrames);
	for(int i = oldFrameCount; i < newFrameCount; i++){
		m_frameOrientation[i] = Dataset::orientationLtRTtB;
		m_frameLayouts[i] = QPoint(0,0);
	}
}

void Dataset::setLayer(int *data, int threshold){
	int layerIndex = getLayerIndex(threshold);
	for(int i = 0; i < m_nFrames*m_nx*m_ny;i++)
		m_layers[layerIndex][i] = data[i];
}

void Dataset::addLayer(int *data, int threshold){
	int layerIndex = getLayerIndex(threshold);
	for(int i = 0; i < m_nFrames*m_nx*m_ny;i++)
		m_layers[layerIndex][i] += data[i];
}

int* Dataset::getLayer(int threshold){
	int layerIndex = thresholdToIndex(threshold);
	if(layerIndex == -1)
		return nullptr;
	return m_layers[layerIndex];
}
