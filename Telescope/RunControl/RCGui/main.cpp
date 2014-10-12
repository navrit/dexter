#include "RCMainWindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    RCMainWindow w;
    w.show();

    return a.exec();
}
