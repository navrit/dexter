#include "spidrmpx3eq.h"
#include <QApplication>

//#include "ui_spidrmpx3eq.h"

int main(int argc, char *argv[])
{

	QApplication a(argc, argv);

    // At this level there's a difference between
    // SpidrMpx3Eq and Ui::SpidrMpx3Eq.  Not to be confused.
    SpidrMpx3Eq w;

    w.show();

    return a.exec();
}
