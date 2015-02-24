#include "mpx3gui.h"
#include <QApplication>
#include <QtOpenGL/QGLFormat>
//#include "ui_spidrmpx3eq.h"
//#include <QAbstractOpenGLFunctions>
#include <QOpenGLFunctions_3_3_Core>
int main(int argc, char *argv[])
{

  QApplication a(argc, argv);

  // At this level there's a difference between
  // SpidrMpx3Eq and Ui::SpidrMpx3Eq.  Not to be confused.
  Mpx3GUI w( &a );

    w.show();

    return a.exec();
}
