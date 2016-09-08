#ifndef HEATMAPDOCKWIDGETWRAPPER_H
#define HEATMAPDOCKWIDGETWRAPPER_H

#include <QMainWindow>

class heatmapDockWidgetWrapper : public MainWindow
{
    Q_OBJECT

public:
    heatmapDockWidgetWrapper();

private:
    void createDockWindow();
};

#endif // HEATMAPDOCKWIDGETWRAPPER_H
