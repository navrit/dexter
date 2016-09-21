#include "qcstmexternalgraph.h"
#include "ui_qcstmexternalgraph.h"

QCstmExternalGraph::QCstmExternalGraph(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::QCstmExternalGraph)
{
    ui->setupUi(this);
}

QCstmExternalGraph::~QCstmExternalGraph()
{
    delete ui;
}

void QCstmExternalGraph::SetMpx3GUI(Mpx3GUI *p)
{
    _mpx3gui = p;
    setGradient(0);
}

void QCstmExternalGraph::setGradient(int index)
{
    ui->glPlot->setGradient( _mpx3gui->getGradient(index));
}
