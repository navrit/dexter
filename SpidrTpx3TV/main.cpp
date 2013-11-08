#include <QApplication>
#include "SpidrTpx3Tv.h"

int main(int argc, char *argv[])
{
  QApplication app( argc, argv );
  SpidrTpx3Tv tv;
  tv.show();
  return app.exec();  
}
