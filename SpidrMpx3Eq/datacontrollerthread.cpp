#include "datacontrollerthread.h"
#include <QDebug>
#include "TiffFile.h"

DataControllerThread::DataControllerThread(Mpx3GUI *, QObject *parent) : QThread(parent)
{ }

DataControllerThread::~DataControllerThread()
{ }

void DataControllerThread::saveTIFFParallel(QString filename, const uint imageWidth, const int *pixels)
{
//    qDebug() << "[INFO]\tSaving in parallel - TIFF" << filename << imageWidth;
    if (!TiffFile::saveToTiff32(filename.toUtf8().data(), pixels, imageWidth, imageWidth)) {
        qDebug() << "[ERROR]\tFailed to save TIFF:" << filename;
    }
}

void DataControllerThread::run()
{
//    qDebug() << "[INFO]\tStarted data controller thread";
}
