#ifndef DATACONTROLLERTHREAD_H
#define DATACONTROLLERTHREAD_H

#include <QObject>
#include <QThread>
#include "canvas.h"

class Mpx3GUI;

class DataControllerThread : public QThread
{
    Q_OBJECT

public:
    explicit DataControllerThread(Mpx3GUI *, QObject * parent = nullptr);
    virtual ~DataControllerThread() override;
    void saveTIFFParallel(QString filename, Canvas pixels);
    void savePGMParallel(QString filename, Canvas pixels);

protected:
    void run() Q_DECL_OVERRIDE;
};

#endif
