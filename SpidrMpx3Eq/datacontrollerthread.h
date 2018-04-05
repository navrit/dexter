#ifndef DATACONTROLLERTHREAD_H
#define DATACONTROLLERTHREAD_H

#include <QObject>
#include <QThread>
#include "mpx3gui.h"

class DataControllerThread : public QThread
{
    Q_OBJECT
public:
    explicit DataControllerThread(Mpx3GUI *, QObject * parent = 0);
    ~DataControllerThread();
    void saveTIFFParallel(QString filename, const int imageWidth, const QVector<int>& pixels);

protected:

    void run() Q_DECL_OVERRIDE;

private:


};

#endif DATACONTROLLERTHREAD_H
