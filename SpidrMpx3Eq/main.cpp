#include "spidrmpx3eq.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    SpidrMpx3Eq w;
    w.show();

    return a.exec();
}
