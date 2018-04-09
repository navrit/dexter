#include "mpx3gui.h"
#include <QApplication>
#include <QtGui/QOpenGLFunctions>
#include <QLoggingCategory>

int main(int argc, char *argv[])
{

    QApplication a(argc, argv);

    QLoggingCategory::setFilterRules("*.debug=true\nqt.*.debug=false");

#ifdef QT_DEBUG
    qDebug() << "[INFO]\tDEBUGGING BUILD";
#else
    qDebug() << "[INFO]\tRelease mode";
#endif

    //! Set main application icon
    a.setWindowIcon( QIcon("://icons/ASI/ASI_sq_1100x1100.png"));

    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    format.setVersion(3, 3 );
    QSurfaceFormat::setDefaultFormat(format);

    //! Instantiate the main class
    Mpx3GUI w;
    //! Status for startup
    w.setWindowWidgetsStatus();

    //! If the configuration was not loaded properly this won't let the program run
    if ( ! w.isArmedOk() ) return EXIT_FAILURE;

    // If all ok go into the event loop
    w.show();

    QObject::connect( &a, &QApplication::applicationStateChanged,
                      &w, &Mpx3GUI::on_applicationStateChanged);
    QObject::connect( &w, &Mpx3GUI::exitApp,
                      &a, &QApplication::exit);


    return a.exec();
}
