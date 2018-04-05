#include "datacontrollerthread.h"
#include <QDebug>
#include "TiffFile.h"

DataControllerThread::DataControllerThread(Mpx3GUI *, QObject *parent) : QThread(parent)
{

}

DataControllerThread::~DataControllerThread()
{

}

void DataControllerThread::saveTIFFParallel(QString filename, const int imageWidth, const QVector<int>& pixels)
{
//    qDebug() << "[INFO] Saving in parallel - TIFF";
    if ( !TiffFile::saveToTiff32( filename.toUtf8().data(), imageWidth, &pixels[0] ) )
    {
        qDebug() << "[ERROR] Failed to save TIFF:" << filename;
    }
}

void DataControllerThread::run()
{
    qDebug() << "[INFO] Started data controller thread";
}
