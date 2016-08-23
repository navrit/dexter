#include "mpx3gui.h"
#include <QApplication>
#include <QtGui/QOpenGLFunctions>

int main(int argc, char *argv[])
{

    //
    QApplication a(argc, argv);

    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    //format.setStencilBufferSize(8);
    format.setVersion(3, 3 );
    QSurfaceFormat::setDefaultFormat(format);

    // Instantiate the main class
    Mpx3GUI w;
    // status for startup
    w.setWindowWidgetsStatus();

    // If the configuration was not loaded properly this won't let the program run
    if ( ! w.isArmedOk() ) return EXIT_FAILURE;

    // If all ok go into the event loop
    w.show();

    QObject::connect( &a, &QApplication::applicationStateChanged,
                      &w, &Mpx3GUI::on_applicationStateChanged);
    QObject::connect( &w, &Mpx3GUI::exitApp,
                      &a, &QApplication::exit);

    //qRegisterMetaType<QVector<int> >("QVector<int>");

    return a.exec();
}
