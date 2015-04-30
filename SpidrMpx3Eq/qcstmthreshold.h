#ifndef QCSTMTHRESHOLD_H
#define QCSTMTHRESHOLD_H

#include "mpx3gui.h"

#include <QWidget>
#include <QPointF>

namespace Ui {
class QCstmThreshold;
}

class QCstmThreshold : public QWidget
{
	Q_OBJECT

public:
	explicit QCstmThreshold(QWidget *parent = 0);
	~QCstmThreshold();
	void SetMpx3GUI(Mpx3GUI * p) { _mpx3gui = p; }
	void SetupSignalsAndSlots();
	void GUIDefaults();
	int ExtractScanInfo(int * data, int size_in_bytes, int thl);

private:

	Ui::QCstmThreshold *ui;
  // Connectivity between modules
  Mpx3GUI * _mpx3gui;
  void addFrame(QPoint offset, int layer, int*data);
  int getActiveTargetCode();
  void setPoint(QPointF data, int plot);
  void addPoint(QPointF data, int plot);
  double getPoint(int x, int plot);

	int * _data;

private slots:

void StartCalibration();
void UpdateHeatMap();
void UpdateChart(int setId, int thlValue);

signals:
void slideAndSpin(int, int);
void UpdateHeatMapSignal();
void UpdateChartSignal(int, int);
void fillText(QString);

};

#endif // QCSTMTHRESHOLD_H
