#include "mpx3gui.h"
#include <QApplication>
#include <QtGui/QOpenGLFunctions>
int main(int argc, char *argv[])
{

	QApplication a(argc, argv);
	QSurfaceFormat format;
	    //format.setDepthBufferSize(24);
	    //format.setStencilBufferSize(8);
	    format.setVersion(3, 1 );
	QSurfaceFormat::setDefaultFormat(format);
	Mpx3GUI w( &a );
	w.show();
	return a.exec();
}
