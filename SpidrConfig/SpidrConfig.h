#include "ui_SpidrConfig.h"
#include <QDialog>

class SpidrController;
class QIntValidator;

class SpidrConfig : public QDialog, Ui_SpidrConfigDialog
{
  Q_OBJECT

 public:
  SpidrConfig();
  ~SpidrConfig();

 private slots:
  void connectOrDisconnect();
  void readStartupOptions();
  void eraseStartupOptions();
  void storeStartupOptions();
  void myResize();

 private:
  SpidrController *_spidrController;

  QIntValidator   *_ipAddrValidator;
  QIntValidator   *_ipPortValidator;

  int              _startupOptions;
};
