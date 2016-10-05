/**
 * \class HeatmapDisplay
 *
 * \brief A wrapper widget for displaying heatmaps. Uses QCstmGLPlot, GradientWidget and QCstmRuler.
 *
 */

#ifndef HEATMAPDISPLAY_H
#define HEATMAPDISPLAY_H

#include <QWidget>
#include "gradient.h"
#include "qcstmglplot.h"
namespace Ui {
class HeatmapDisplay;
}

class HeatmapDisplay : public QWidget
{
    Q_OBJECT

public:
    explicit HeatmapDisplay(QWidget *parent = 0);
    ~HeatmapDisplay();

private:
    Ui::HeatmapDisplay *ui;
    void setupSignalsAndSlots();

public slots:
    void setGradient(Gradient* gradient);
    void setSize(QPoint size);
    void setSize(int x, int y);
    void set_range(QCPRange range);

public:
    QCstmGLPlot *getPlot();
};

#endif // HEATMAPDISPLAY_H
