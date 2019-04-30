#include "mpx3gui.h"
#include <QApplication>
#include <QtGui/QOpenGLFunctions>
#include <QLoggingCategory>
#include "canvas.h"

int main(int argc, char *argv[])
{

    //QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling); // DPI support
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps); // HiDPI pixmaps
    qputenv("QT_SCALE_FACTOR", "1"); // Must be >=1

    QApplication a(argc, argv);
    qRegisterMetaType<Canvas>("Canvas");

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
    format.setVersion(3, 3);
    QSurfaceFormat::setDefaultFormat(format);

    //! Instantiate the main class
    Mpx3GUI w;
    //! Status for startup
    w.setWindowWidgetsStatus();


    int id = QFontDatabase::addApplicationFont(":/icons/HelveticaNeue.ttf");
    QString family = QFontDatabase::applicationFontFamilies(id).at(0);
    QFont helveticaNeue(family);

    QFile stylesheetFile("://stylesheet.qss");
    stylesheetFile.open(QFile::ReadOnly);
    const QString stylesheet = QLatin1String(stylesheetFile.readAll());
    stylesheetFile.close();
    w.setStyleSheet(stylesheet);

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
