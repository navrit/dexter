#include "qcstmBHWindow.h"
#include "ui_qcstmBHWindow.h"
#include "qcstmglvisualization.h"
#include "qcstmcorrectionsdialog.h"

//! GUI for the beam hardening correction.

//! Allows the user to make a list of OB (Open Beam) corrections.
//! Can be saved to / loaded from JSON files.
//! User can start the correction from this screen, or from the "Corrections" window ( qcstmcorrectionsdialog ).

QCstmBHWindow::QCstmBHWindow(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::QCstmBHWindow)
{
    ui->setupUi(this);
    this->setWindowTitle("Beam hardening correction");

    connect(this,&QCstmBHWindow::loadSignal,this, &QCstmBHWindow::on_loadButton_clicked);
    connect(this,SIGNAL(loadData()), this, SLOT(on_loadButton_clicked()));

    _corr = dynamic_cast<QCstmCorrectionsDialog*>(parent); //!makes _corr object for signal purposes.

    //#44 Another corrections enhancement
    //setFileSaved(false);
}

QCstmBHWindow::~QCstmBHWindow(){
    //#44 Another corrections enhancement
    //setFileSaved(false);
    delete ui;
}

void QCstmBHWindow::SetMpx3GUI(Mpx3GUI *p){
    _mpx3gui = p;
    _mpx3gui->saveOriginalDataset();    //! Keep a copy of the original dataset
    connect(this, &QCstmBHWindow::openData2, _mpx3gui, &Mpx3GUI::open_data_with_path);
    connect(this, SIGNAL(reload()),_mpx3gui->getVisualization(),SLOT(reload_all_layers()));
    connect(_mpx3gui, SIGNAL(open_data_failed()), this, SLOT(on_open_data_failed())); //! Ignore no matching signal error - it's whiny
    connect(this,SIGNAL(applyCorrection()),this, SLOT(on_applyBHCorrection()));
    connect(_corr, SIGNAL(applyBHCorrection()), this, SLOT(on_applyBHCorrection()));

    connect(this, SIGNAL(sendFilename(QString)), _corr, SLOT(receiveFilename(QString)));
    connect(this, SIGNAL(sendChecked_BHCorrCheckbox(bool)), _corr, SLOT(setChecked_BHCorrCheckbox(bool)));
}

//! Creates dialog where user can add a correction item, specify its thickness and material.
void QCstmBHWindow::on_addButton_clicked() {
    _bhdialog = nullptr;
    if (!_bhdialog){
        _bhdialog = new qcstmBHdialog(this);
        _bhdialog->open();
        _bhdialog->raise();
        _bhdialog->activateWindow();
        connect(_bhdialog, SIGNAL(talkToForm(double, QString)), this, SLOT(on_talkToForm(double, QString)));
    }
}

//! Is called after user finishes with dialog that adds a correction item.
void QCstmBHWindow::on_talkToForm(double thickness, QString material){
    bool contained = false;

    for(int i = 0; i<thicknessvctr.size(); i++){
        if(thicknessvctr[i] == thickness){
            contained = true;
            QMessageBox msgBox;
            msgBox.setWindowTitle("Error");
            msgBox.setText("Please do not use duplicate thicknesses.");
            msgBox.exec();
            break;
        }
    }
    if(!contained){
        correctionMaterial.insert(thickness,material);
        material += " ";
        material.append(QString("%1").arg(thickness));
        material += " um";
        ui->list->addItem(material);
        thicknessvctr.push_back(thickness);
        emptyCorrectionCounter++;
        //ui->startButton->setEnabled(false); Disabled start button
    }
}

void QCstmBHWindow::on_clearButton_clicked() {
    //! Removes selected correction item from the list.
    if(!correctionMap.contains(thicknessvctr[selectedItemNo])) emptyCorrectionCounter--;
    delete ui->list->item(selectedItemNo);
    correctionMap.remove(thicknessvctr[selectedItemNo]);
    thicknessvctr.erase (thicknessvctr.begin()+selectedItemNo);
    if(selectedItemNo>1) {
        selectedItemNo--;
        ui->list->item(selectedItemNo)->setSelected(true);
    } else {
        selectedItemNo = 0;
        ui->clearButton->setEnabled(false);
        ui->loadButton->setEnabled(false);
        //ui->startButton->setEnabled(false); Disabled start button
    }
    // if(emptyCorrectionCounter == 0 && thicknessvctr.size()>2 )
    //ui->startButton->setEnabled(true); Disabled start button
}

void QCstmBHWindow::on_clearAllButton_clicked(){
    this->setWindowTitle(QString("BH Corrections"));

    //! Clears everything in the window.
    thicknessvctr.clear();
    correctionMap.clear();
    correctionMaterial.clear();
    correctionPaths.clear();
    ui->list->clear();
    ui->plotWidget->clearGraphs();
    ui->plotWidget->clearItems();
    ui->plotWidget->replot();
    //! UI update - Correction Dialog
    emit sendFilename(QString(""));
    emit sendChecked_BHCorrCheckbox(false);
}

void QCstmBHWindow::on_loadButton_clicked(){
    //! Loads dataset from memory.

    //! Calls slot in Mpx3Gui.
    //! When loading fails, dataOpened will be set to false and nothing will happen.
    //! Slot should always be called with first parameter false, so it doesn't override the OriginalSet of Mpx3Gui.

    if(correctionMap.contains(thicknessvctr[selectedItemNo])) return;
    dataOpened = true;
    emit openData2(false, usePath, correctionPath);
    if(!correctionMap.contains(thicknessvctr[selectedItemNo])&&dataOpened){
        ui->list->item(selectedItemNo)->setBackground(QBrush(Qt::cyan));
        correctionMap.insert(thicknessvctr[selectedItemNo], *_mpx3gui->getDataset());
        correctionPaths.insert(thicknessvctr[selectedItemNo], correctionPath);
        emptyCorrectionCounter--;
        ui->loadButton->setEnabled(false);
        on_plot();
    }
    //if(emptyCorrectionCounter == 0 && thicknessvctr.size()>2 )     Disabled start button
    //ui->startButton->setEnabled(true);                         Disabled start button
}

void QCstmBHWindow::on_plot(){
    //! Plots the average count of the first layer of each correction item.
    //! Has to start from scratch everytime, since items may not be sorted.

    QVector<double> yPlot, xPlot;
    QMap<double, double> plotMap;
    for(int i = 0; i<thicknessvctr.size(); i++){
        if(correctionMap.contains(thicknessvctr[i])) xPlot.push_back(thicknessvctr[i]);
    }
    for(int i = 0; i< xPlot.size(); i++){
        double count = 0;
        for(int j = 0; j< _mpx3gui->getDataset()->getPixelsPerLayer(); j++ ){
            count += correctionMap[xPlot[i]].getLayer(0)[j];
        }
        count /= _mpx3gui->getDataset()->getPixelsPerLayer(); //average of threshold 0
        plotMap.insert(xPlot[i], count);
    }

    std::sort(xPlot.begin(), xPlot.end());
    for(int i = 0; i<xPlot.size(); i++){
        yPlot.push_back(plotMap[xPlot[i]]);
    }
    double minX = 0, maxX = 0;
    double minY = 0, maxY = 0;
    for(int i = 0; i < xPlot.size(); i++ ){
        minX = std::min(minX, xPlot[i]);
        maxX = std::max(maxX, xPlot[i]);
        minY = std::min(minY, yPlot[i]);
        maxY = std::max(maxY, yPlot[i]);
    }
    ui->plotWidget->addGraph();
    ui->plotWidget->graph(0)->setData(xPlot, yPlot);
    ui->plotWidget->xAxis->setLabel("Thickness");
    ui->plotWidget->yAxis->setLabel("Signal");
    ui->plotWidget->xAxis->setRange(minX, maxX);
    ui->plotWidget->yAxis->setRange(minY, maxY);
    ui->plotWidget->replot();
    //! Do interpolation - Disabled because QPlot automatically does linear interpolation and spline cubic does not work well
    /*
        if(xPlot.size()>2){
            tk::spline s;
            //sort(xPlot.begin(), xPlot.end());
            //sort(yPlot.begin(), yPlot.end());
            //s.set_points(xPlot.toStdVector(),yPlot.toStdVector(),false);
            s.set_points(xPlot.toStdVector(),yPlot.toStdVector());
            QVector<double> sx;
            QVector<double> sy;
            for(int i = 0; i < maxX; i++){
                sx.push_back(i);
                sy.push_back(s(i));
            }
            ui->plotWidget->graph(1)->setData(sx, sy);
            ui->plotWidget->graph(1)->setPen(QPen(Qt::red));
        }
        */

}

// Disabled start button
/*void QCstmBHWindow::on_startButton_clicked(){
  //! Starts the correction (calls on_applyCorrection)

  emit applyCorrection();
  emit reload();
}*/

void QCstmBHWindow::on_list_itemClicked(QListWidgetItem *item){
    //! Enables / disables buttons that should or should not be used. Also loads the selected dataset into Mpx3Gui.
    //! selectedItemNo is used throughout the code to know which listitem is selected.

    selectedItemNo = item->listWidget()->row(item);
    ui->clearButton->setEnabled(true);
    if(!correctionMap.contains(thicknessvctr[selectedItemNo])){
        ui->loadButton->setEnabled(true);
    }else{
        ui->loadButton->setEnabled(false);
        if(_mpx3gui->getDataset()->getLayer(0)[0] != correctionMap[thicknessvctr[selectedItemNo]].getLayer(0)[0]){
            *(_mpx3gui->getDataset()) = correctionMap[thicknessvctr[selectedItemNo]];
            emit reload();
        }
    }
}

void QCstmBHWindow::on_applyBHCorrection()
//! Makes signal to thickness conversion.
{
    if(emptyCorrectionCounter != 0 || thicknessvctr.size()<3 )
        return;
    QList<int> keys = _mpx3gui->getDataset()->getThresholds();

    if(m_spline==nullptr)
        m_spline = new tk::spline;  // instantiate spline if not defined

    //! Loop over different layers in the Dataset.
    for (int i = 0; i < keys.length(); i++){
        //!Create a datastructure that can be analysed.
        QVector<QVector<double>> bhData(_mpx3gui->getDataset()->getPixelsPerLayer());
        std::sort(thicknessvctr.begin(), thicknessvctr.end(), cstmSortStruct);

        for(int j = 0; j<thicknessvctr.size(); j++){
            int * layer = correctionMap[thicknessvctr[j]].getLayer(keys[i]);
            for(unsigned int k = 0; k<_mpx3gui->getDataset()->getPixelsPerLayer(); k++){ bhData[k].push_back(layer[k]); }
        }

        //! Apply the correction.
        //! Makes some checks to ensure that the spline algorithm doesn't crash.
        int * currentLayer = _mpx3gui->getOriginalDataset()->getLayer(keys[i]); //Doesn't take into account other corrections.
        //int * currentLayer = _mpx3gui->getDataset()->getLayer(keys[i]); //Use this instead?

        for(unsigned int j = 0; j< _mpx3gui->getDataset()->getPixelsPerLayer(); j++){
            QVector<double> temp = bhData[j];
            bool ascending = true;

            for(int q = 0; q<temp.size()-1; q++){
                if(temp[q]>=temp[q+1]) ascending = false;
            }

            int a = currentLayer[j];

            if(temp[0]!= 0 && temp[0] < 50000 && ascending){
                m_spline->set_points(temp.toStdVector(),thicknessvctr.toStdVector(), false);
                currentLayer[j] = (*m_spline)(currentLayer[j]); //Do the interpolation
            }

            if(a == currentLayer[j])
                currentLayer[j] = 0;

        }

        for(unsigned int j = 0; j< _mpx3gui->getDataset()->getPixelsPerLayer(); j++){
            _mpx3gui->getDataset()->getLayer(keys[i])[j] = currentLayer[j];
        }
    }

    _mpx3gui->getDataset()->setCorrected(true);
}

void QCstmBHWindow::on_open_data_failed(){
    dataOpened = false;
}

void QCstmBHWindow::on_list_doubleClicked(const QModelIndex &index){
    emit loadSignal();
}

void QCstmBHWindow::on_okButton_clicked(){
    if(emptyCorrectionCounter != 0 || thicknessvctr.size() < 3 ){
        QMessageBox msgBox;
        msgBox.setWindowTitle("Error");
        msgBox.setText(tr("You haven't loaded all of the necessary corrections. The beam hardening will not operate. Please load more corrections."));
        //! UI update - Corrections Dialog - reset to blank
        emit sendFilename(QString(""));
        emit sendChecked_BHCorrCheckbox(false);
        msgBox.exec();
    }

    this->close();

    //#44 Another corrections enhancement
    /*

    //! If file not saved save a temporary JSON file (tmp.json), overwrite if already exists
    if(!getFileSaved()){
        QString tmpFileName = "tmp.json";
        //QFile tmpFile;
        //tmpFile.setFileName(tmpFileName);
        //if (!tmpFile.exists()){

        //}
        saveJSON(tmpFileName);
        //! UI update - Corrections Dialog - update filename with temporary filename and tick checkbox
        emit sendFilename(tmpFileName);
        emit sendChecked_BHCorrCheckbox(true);
    }
    */


}

void QCstmBHWindow::on_loadJsonButton_clicked(){
    //! Loads a .json file from which a number of correction items are made.
    //! json file contains material, thickness and path to dataset.

    fileName = QFileDialog::getOpenFileName(this,tr("Json files (*.JSON)"));
    QFile loadFile(fileName);

    if(!loadFile.open(QIODevice::ReadOnly)){
        printf("Couldn't open configuration file %s\n", fileName.toStdString().c_str());
        return;
    }

    this->setWindowTitle(QString(fileName));

    thicknessvctr.clear();
    correctionMap.clear();
    correctionMaterial.clear();
    correctionPaths.clear();
    ui->list->clear();

    QByteArray binaryData = loadFile.readAll();
    QJsonObject JSobjectParent = QJsonDocument::fromJson(binaryData).object();
    QJsonObject::iterator it, itParent;

    // Parse JSON file
    for(int i = 0; i<100; i++){
        QString correctionNo = "corr";
        correctionNo += QString::number(i);
        itParent = JSobjectParent.find(correctionNo);
        double thickness;

        if(itParent != JSobjectParent.end()){
            QJsonObject JSobject = itParent.value().toObject();

            it = JSobject.find("thickness");
            if(it != JSobject.end())
                thickness = it.value().toDouble();

            it = JSobject.find("mat");
            if(it != JSobject.end())
                on_talkToForm(thickness,it.value().toString());

            it = JSobject.find("path");
            if(it != JSobject.end()){
                usePath = true;
                correctionPath = it.value().toString();

                //! UI update - send file name to Corrections Dialog
                emit sendFilename(fileName);
                selectedItemNo = i;
                emit loadData();
                usePath = false; // set to false to prevent accidents further down the road.
            }
            ui->list->item(i)->setBackground(QBrush(Qt::cyan));
        } else {
            break;
        }
    }
}


//! Saves current list of correction items to .json file.
//! json file contains material, thickness and path to dataset.
void QCstmBHWindow::on_saveJsonButton_clicked()
{
    // Opens dialog, add .json to filename if not present
    fileName = QFileDialog::getSaveFileName(this, tr("Save to Json"),"",tr("Json files (*.JSON)"));
    if (!fileName.endsWith(".JSON") && !fileName.endsWith(".json") && !fileName.endsWith("Json"))
        fileName += ".json";

    // Saves JSON file - reused function in tmp.json on_okButton_clicked()
    saveJSON(fileName);
}

void QCstmBHWindow::saveJSON(QString fileName){

    // Basic permissions check
    QFile loadFile(fileName);
    if(!loadFile.open(QIODevice::WriteOnly)){
        printf("Couldn't open configuration file %s\n", fileName.toStdString().c_str());
        return;
    }

    // Makes JSON object and populates it
    QJsonObject JSobjectParent;
    for(int i = 0; i<thicknessvctr.length(); i++){
        QJsonObject objcorr;
        objcorr.insert("mat", correctionMaterial[thicknessvctr[i]]);
        objcorr.insert("thickness", thicknessvctr[i]);
        objcorr.insert("path", correctionPaths[thicknessvctr[i]]);
        QString objname = "corr";
        objname += QString::number(i);
        JSobjectParent.insert(objname, objcorr);
    }

    // Saves JSON file
    QJsonDocument doc;
    doc.setObject(JSobjectParent);
    loadFile.write(doc.toJson());  //Actually writes the file

    setFileSaved(true);
}

bool QCstmBHWindow::getFileSaved() const
{
    return fileSaved;
}

void QCstmBHWindow::setFileSaved(bool value)
{
    //#44 Another corrections enhancement
    //fileSaved = value;
}
