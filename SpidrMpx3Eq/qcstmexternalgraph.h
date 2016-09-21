#ifndef QCSTMEXTERNALGRAPH_H
#define QCSTMEXTERNALGRAPH_H

#include <QDialog>
#include "mpx3gui.h"
#include "gradient.h"

namespace Ui {
class QCstmExternalGraph;
}

class QCstmExternalGraph : public QDialog
{
    Q_OBJECT

public:
    explicit QCstmExternalGraph(QWidget *parent = 0);
    ~QCstmExternalGraph();
    Ui::QCstmExternalGraph *GetUI(){return ui;}

    void SetMpx3GUI(Mpx3GUI *p);
    void setGradient(int index);

private:
    Ui::QCstmExternalGraph *ui;
    Mpx3GUI *_mpx3gui;
};

#endif // QCSTMEXTERNALGRAPH_H
