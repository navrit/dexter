#include "ui_SpidrDacs.h"
#include <QDialog>
#include <QVector>

class SpidrController;
class QIntValidator;
class QLabel;
class QSignalMapper;
class QSlider;
class QSpinBox;

class SpidrDacs : public QDialog, Ui_SpidrDacsDialog
{
  Q_OBJECT

 public:
  SpidrDacs();
  ~SpidrDacs();

 public slots:
  void connectOrDisconnect();
  void readDacs();
  void dacChanged( int index );
  void adjustLayout();

 private:
  SpidrController *_spidrController;

  QVector<QSlider *>  _slidrs;
  QVector<QSpinBox *> _spboxs;
  QVector<QLabel *>   _labels;

  QSignalMapper   *_signalMapper;

  QIntValidator   *_ipAddrValidator;
  QIntValidator   *_ipPortValidator;

  int              _timerId;

  void timerEvent( QTimerEvent * );
  void initDacs();
};
