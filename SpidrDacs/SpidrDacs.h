#include "ui_SpidrDacs.h"
#include <QDialog>
#include <QPalette>
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
  void setDacsDefaults();
  void setDac( int index );
  void adjustLayout();
  void setDeviceIndex( int index );
  void setDeviceType( int index );
  void storeDacs();
  void eraseDacs();
  void hideOkay();

 private:
  SpidrController    *_spidrController;
  int _deviceIndex;

  int _minWidth;

  QVector<QSlider *>  _slidrs;
  QVector<QSpinBox *> _spboxs;
  QVector<QLabel *>   _nameLabels;
  QVector<QLabel *>   _maxLabels;

  QSignalMapper      *_signalMapper;

  QIntValidator      *_ipAddrValidator;
  QIntValidator      *_ipPortValidator;

  int                 _timerId;

  bool                _disableSetDac;

  QPalette            _qpOkay, _qpError;

  void timerEvent( QTimerEvent * );
  void initDacWidgets( bool enable );
};
