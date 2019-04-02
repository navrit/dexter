#include "datacontrollerthread.h"
#include <QDebug>

DataControllerThread::DataControllerThread(Mpx3GUI *, QObject *parent) : QThread(parent)
{ }

DataControllerThread::~DataControllerThread()
{ }

void DataControllerThread::saveTIFFParallel(QString filename, Canvas pixels)
{
    if (!pixels.saveToTiff(filename.toUtf8().data())) {
        qDebug() << "[ERROR] Failed to save TIFF:" << filename;
    }
}

void DataControllerThread::savePGMParallel(QString filename, Canvas pixels)
{
    if (!pixels.saveToPGM16(filename.toUtf8().data())) {
        qDebug() << "[ERROR] Failed to save PGM16:" << filename;
    }
}

void DataControllerThread::run()
{
//    qDebug() << "[INFO]\tStarted data controller thread";
}
