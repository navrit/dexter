#include "heatmapDockWidgetWrapper.h"

heatmapDockWidgetWrapper::heatmapDockWidgetWrapper() {

}

void heatmapDockWidgetWrapper::createDockWindow(){
    QDockWidget *dockWidget = new QDockWidget(tr("Dock widget"));
    dockWidget->setObjectName("Dock widget test");
    QLineEdit* l = new QLineEdit(0);
    dockWidget->setWidget(l);
    dockWidget->setAllowedAreas(Qt::LeftDockWidgetArea| Qt::RightDockWidgetArea);
    addDockWidget(Qt::BottomDockWidgetArea, dockWidget);
}
