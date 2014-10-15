#include <QApplication>
#include "SpidrConfig.h"

int main(int argc, char *argv[])
{
  QApplication app( argc, argv );
  SpidrConfig conf;
  conf.show();
  return app.exec();  
}
