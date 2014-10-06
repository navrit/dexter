#include <QApplication>
#include "DummyGen.h"

int main(int argc, char *argv[])
{
  QApplication app( argc, argv );
  DummyGen gen;
  gen.show();
  return app.exec();  
}
