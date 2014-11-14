#include <QApplication>
#include "SpidrDacsScan.h"

int main(int argc, char *argv[])
{
  QApplication app( argc, argv );
  SpidrDacsScan scan;
  scan.show();
  return app.exec();  
}
