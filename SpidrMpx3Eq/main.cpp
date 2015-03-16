#include "mpx3gui.h"
#include <QApplication>
int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	Mpx3GUI w( &a );
	w.show();
	return a.exec();
}
