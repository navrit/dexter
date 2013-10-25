#include <QApplication>
#include "SpidrDacs.h"

int main(int argc, char *argv[])
{
  QApplication app( argc, argv );
  SpidrDacs dacs;
  dacs.show();
  return app.exec();  
}
