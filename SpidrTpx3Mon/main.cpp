#include <QApplication>
#include "SpidrMon.h"

int main(int argc, char *argv[])
{
  QApplication app( argc, argv );
  SpidrMon mon;
  mon.show();
  return app.exec();  
}
