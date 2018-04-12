#include "mpx3gui.h"
#include <QApplication>
#include <QtGui/QOpenGLFunctions>
#include <QLoggingCategory>

int main(int argc, char *argv[])
{

    QApplication a(argc, argv);

    QPalette p;
    p = qApp->palette();
    p.setColor(QPalette::Window, QColor(65,65,65));
    p.setColor(QPalette::Highlight, QColor(15,90,140));
    p.setColor(QPalette::ButtonText, QColor(255,255,255));
    p.setColor(QPalette::Text, QColor(255,255,255));
    p.setColor(QPalette::WindowText, QColor(255,255,255));
    p.setColor(QPalette::Base, QColor(40,40,40));

    QLoggingCategory::setFilterRules("*.debug=true\nqt.*.debug=false");

#ifdef QT_DEBUG
    qDebug() << "[INFO]\tDEBUGGING BUILD";
    p.setColor(QPalette::Button, QColor(150,0,0));
    p.setColor(QPalette::Base, QColor(120,0,0));
#else
    qDebug() << "[INFO]\tRelease mode";
    p.setColor(QPalette::Button, QColor(60,60,70));
#endif

    qApp->setPalette(p);

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
