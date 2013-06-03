#include <QApplication>
#include "SpidrTv.h"

int main(int argc, char *argv[])
{
  QApplication app( argc, argv );
  SpidrTv tv;
  tv.show();
  return app.exec();  
}
