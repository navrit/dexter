#include <QApplication>
#include "SpidrMpx3Tv.h"

int main(int argc, char *argv[])
{
  QApplication app( argc, argv );
  SpidrMpx3Tv tv;
  tv.show();
  return app.exec();  
}
