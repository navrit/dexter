#include "datacontrollerthread.h"
#include <QDebug>

DataControllerThread::DataControllerThread(Mpx3GUI *, QObject *parent) : QThread(parent)
{

}

DataControllerThread::~DataControllerThread()
{

}

void DataControllerThread::saveTIFFParallel(QString filename, Canvas pixels)
{
//    qDebug() << "[INFO] Saving in parallel - TIFF" << filename << imageWidth;
    if (!pixels.saveToTiff(filename.toUtf8().data())) {
        qDebug() << "[ERROR] Failed to save TIFF:" << filename;
    }
}

void DataControllerThread::run()
{
//    qDebug() << "[INFO] Started data controller thread";
}
