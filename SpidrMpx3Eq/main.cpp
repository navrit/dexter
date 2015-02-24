#include "mpx3gui.h"
#include <QApplication>
#include <QGLFormat>
//#include "ui_spidrmpx3eq.h"
//#include <QAbstractOpenGLFunctions>
#include <QOpenGLFunctions_3_3_Core>
int main(int argc, char *argv[])
{

  QApplication a(argc, argv);
  QSurfaceFormat format;
      //format.setDepthBufferSize(24); //Not needed for just 2D work, auto-select for now.
      //format.setStencilBufferSize(8);
      format.setVersion(3, 3);
      format.setProfile(QSurfaceFormat::CoreProfile);
  QSurfaceFormat::setDefaultFormat(format);
  // At this level there's a difference between
  // SpidrMpx3Eq and Ui::SpidrMpx3Eq.  Not to be confused.
  Mpx3GUI w( &a );

    w.show();

    return a.exec();
}
