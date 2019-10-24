#include "EnergyConfiguration.h"
 #include "ui_EnergyConfiguration.h"
 #include "mpx3gui.h"
 #include "qcstmglvisualization.h"
 #include "qcstmdacs.h"
 #include "EnergyCalibrator.h"

 EnergyConfiguration::EnergyConfiguration(QWidget *parent) : QWidget(parent),
     ui(new Ui::EnergyConfiguration)
 {
     ui->setupUi(this);

     setMinimumEnergy_sliders(int(_minimumEnergy_keV));
     setMaximumEnergy_sliders(int(_maximumEnergy_keV));

     setMinimumEnergy_spinBox(_minimumEnergy_keV);
     setMaximumEnergy_spinBox(_maximumEnergy_keV);
 }

 EnergyConfiguration::~EnergyConfiguration()
 {
     delete ui;
 }

 void EnergyConfiguration::setWindowWidgetsStatus(win_status s)
 {
     switch (s) {

     case win_status::startup:
         this->setDisabled( true );
         break;
     case win_status::connected:
         this->setEnabled( true );

         enablePossibleSlidersAndCheckboxes();

         break;
     case win_status::disconnected:
         this->setDisabled( true );
         break;
     default:
         break;
     }
 }

 void EnergyConfiguration::ConnectionStatusChanged(bool conn)
 {
     if ( conn ) {
         setWindowWidgetsStatus( win_status::connected );
         _mpx3gui->getEnergyConfiguration()->updateDACsFromConfig();
     } else {
         setWindowWidgetsStatus( win_status::disconnected );
     }
 }

 /*! @brief NOTE: Not used, left to show the implementation if the DACs are considered the be
   * more reliable for some reason than the target energies
   */
 void EnergyConfiguration::updateEnergyFromConfig()
 {
     /* DACs are the master, set Target energies based on them */

     /* Calculate average energy from all chips for every threshold */
     QVector<double> averageEnergies = {0.0};

     for (int chip = 0; chip < __max_number_of_chips; chip++) {
         double averageEnergyPerThreshold = 0;

         for (int threshold = 0; threshold < __max_number_of_thresholds; threshold++) {
             int DAC_value = _mpx3gui->getConfig()->getDACValue(chip, threshold);
             double E = _mpx3gui->getEnergyCalibrator()->calcEnergy(chip, threshold, DAC_value);
             averageEnergyPerThreshold += E;
         }
         averageEnergyPerThreshold /= __max_number_of_chips;
         averageEnergies.push_back(averageEnergyPerThreshold);
     }

     //qDebug() << _mpx3gui->getEnergyCalibrator()->calcEnergy(chip, threshold, DAC_value);

     /* Update the Energy Configuration GUI based on these energies */
     on_spinBox_th0_valueChanged(averageEnergies[0]);
     on_spinBox_th1_valueChanged(averageEnergies[1]);
     on_spinBox_th2_valueChanged(averageEnergies[2]);
     on_spinBox_th3_valueChanged(averageEnergies[3]);
     on_spinBox_th4_valueChanged(averageEnergies[4]);
     on_spinBox_th5_valueChanged(averageEnergies[5]);
     on_spinBox_th6_valueChanged(averageEnergies[6]);
     on_spinBox_th7_valueChanged(averageEnergies[7]);
 }

 void EnergyConfiguration::updateDACsFromConfig()
 {
     if (_mpx3gui->GetSpidrController() == nullptr || !_mpx3gui->GetSpidrController()->isConnected()) {
         return;
     }

#ifdef QT_DEBUG
    for (int th = 0; th < __max_number_of_thresholds; ++th) {
        qDebug() << "[DEBUG]\tEnergyConfiguration::updateDACsFromConfig --> th =" << th << "Target E =" << _mpx3gui->getConfig()->getTargetEnergies(th);
    }
#endif

    /* Target energies are the master, set DACs based on them */
    ui->spinBox_th0->setValue(_mpx3gui->getConfig()->getTargetEnergies(0));
    ui->spinBox_th1->setValue(_mpx3gui->getConfig()->getTargetEnergies(1));
    ui->spinBox_th2->setValue(_mpx3gui->getConfig()->getTargetEnergies(2));
    ui->spinBox_th3->setValue(_mpx3gui->getConfig()->getTargetEnergies(3));
    ui->spinBox_th4->setValue(_mpx3gui->getConfig()->getTargetEnergies(4));
    ui->spinBox_th5->setValue(_mpx3gui->getConfig()->getTargetEnergies(5));
    ui->spinBox_th6->setValue(_mpx3gui->getConfig()->getTargetEnergies(6));
    ui->spinBox_th7->setValue(_mpx3gui->getConfig()->getTargetEnergies(7));

    /* Set the minimum and maxmimum possible energies in spinboxes and sliders
      *    based on what is possible */

    //! NOTE: This is wrong currently

    for (int threshold = 0; threshold < __max_number_of_thresholds; ++threshold) {
        uint minimumPossibleEnergy = _minimumEnergy_keV;
        uint maximumPossibleEnergy = _maximumEnergy_keV;

        for (int chip = 0; chip < __max_number_of_chips; ++chip) {

            uint minEnergy = uint(_mpx3gui->getEnergyCalibrator()->calcEnergy(chip, threshold, 0));
            uint maxEnergy = uint(_mpx3gui->getEnergyCalibrator()->calcEnergy(chip, threshold, __max_DAC_range-1));

            if (minEnergy > minimumPossibleEnergy && minEnergy < maximumPossibleEnergy) {
                minimumPossibleEnergy = minEnergy;

                switch (threshold) {
                case 0:
                    ui->slider_th0->setMinimum(int(minimumPossibleEnergy));
                    ui->spinBox_th0->setMinimum(int(minimumPossibleEnergy));
                    break;
                case 1:
                    ui->slider_th1->setMinimum(int(minimumPossibleEnergy));
                    ui->spinBox_th1->setMinimum(int(minimumPossibleEnergy));
                    break;
                case 2:
                    ui->slider_th2->setMinimum(int(minimumPossibleEnergy));
                    ui->spinBox_th2->setMinimum(int(minimumPossibleEnergy));
                    break;
                case 3:
                    ui->slider_th3->setMinimum(int(minimumPossibleEnergy));
                    ui->spinBox_th3->setMinimum(int(minimumPossibleEnergy));
                    break;
                case 4:
                    ui->slider_th4->setMinimum(int(minimumPossibleEnergy));
                    ui->spinBox_th4->setMinimum(int(minimumPossibleEnergy));
                    break;
                case 5:
                    ui->slider_th5->setMinimum(int(minimumPossibleEnergy));
                    ui->spinBox_th5->setMinimum(int(minimumPossibleEnergy));
                    break;
                case 6:
                    ui->slider_th6->setMinimum(int(minimumPossibleEnergy));
                    ui->spinBox_th6->setMinimum(int(minimumPossibleEnergy));
                    break;
                case 7:
                    ui->slider_th7->setMinimum(int(minimumPossibleEnergy));
                    ui->spinBox_th7->setMinimum(int(minimumPossibleEnergy));
                    break;
                }

                qDebug() << "[INFO]\tSet minimum energy in GUI elements to:" << minimumPossibleEnergy << " threshold =" << threshold;
            }
            if (maxEnergy < maximumPossibleEnergy && maxEnergy > minimumPossibleEnergy) {
                maximumPossibleEnergy = maxEnergy;

                switch (threshold) {
                case 0:
                    ui->slider_th0->setMaximum(int(maximumPossibleEnergy));
                    ui->spinBox_th0->setMaximum(int(maximumPossibleEnergy));
                    break;
                case 1:
                    ui->slider_th1->setMaximum(int(maximumPossibleEnergy));
                    ui->spinBox_th1->setMaximum(int(maximumPossibleEnergy));
                    break;
                case 2:
                    ui->slider_th2->setMaximum(int(maximumPossibleEnergy));
                    ui->spinBox_th2->setMaximum(int(maximumPossibleEnergy));
                    break;
                case 3:
                    ui->slider_th3->setMaximum(int(maximumPossibleEnergy));
                    ui->spinBox_th3->setMaximum(int(maximumPossibleEnergy));
                    break;
                case 4:
                    ui->slider_th4->setMaximum(int(maximumPossibleEnergy));
                    ui->spinBox_th4->setMaximum(int(maximumPossibleEnergy));
                    break;
                case 5:
                    ui->slider_th5->setMaximum(int(maximumPossibleEnergy));
                    ui->spinBox_th5->setMaximum(int(maximumPossibleEnergy));
                    break;
                case 6:
                    ui->slider_th6->setMaximum(int(maximumPossibleEnergy));
                    ui->spinBox_th6->setMaximum(int(maximumPossibleEnergy));
                    break;
                case 7:
                    ui->slider_th7->setMaximum(int(maximumPossibleEnergy));
                    ui->spinBox_th7->setMaximum(int(maximumPossibleEnergy));
                    break;
                }

                qDebug() << "[INFO]\tSet maximum energy in GUI elements to:" << maximumPossibleEnergy << " threshold =" << threshold;
            }
        }
    }
 }

 void EnergyConfiguration::setDACs_all_chips_and_thresholds(int threshold, double energy)
 {
     _mpx3gui->getDACs()->getAllDACSimultaneousCheckBox()->setChecked(false);

     for (int chip = 0; chip < __max_number_of_chips; ++chip) {

         /* Update local and config target energies at the same time */
         targetEnergies[threshold] = energy;
         _mpx3gui->getConfig()->setTargetEnergy(threshold, energy);

         /* TH = (E - offset)/slope */
         int DAC_value = int(_mpx3gui->getEnergyCalibrator()->calcDac(chip, threshold, energy) + 0.5);
         double E = _mpx3gui->getEnergyCalibrator()->calcEnergy(chip, threshold, DAC_value);

         /* I.e. ERROR code*/
         if (DAC_value > __max_DAC_range || DAC_value <= 0) {
             setActualEnergiesLabel(threshold, -1);
             break;
         } else {
             setActualEnergiesLabel(threshold, E);
         }

         /* qDebug() << "Chip =" << chip
                      << "| Th =" << threshold
                      << "| DAC =" << DAC_value
                      << "| Energy =" << E << "keV"; */


         _mpx3gui->getDACs()->setDAC_index(nullptr, chip, threshold, DAC_value);
     }

     /* Reset back to Chip 0 and simultaneous DAC selection */
     _mpx3gui->getDACs()->ChangeDeviceIndex(0);
     _mpx3gui->getDACs()->getAllDACSimultaneousCheckBox()->setChecked(true);
 }

 void EnergyConfiguration::setMinimumEnergy_sliders(int val)
 {
     ui->slider_th0->setMinimum(val);
     ui->slider_th1->setMinimum(val);
     ui->slider_th2->setMinimum(val);
     ui->slider_th3->setMinimum(val);
     ui->slider_th4->setMinimum(val);
     ui->slider_th5->setMinimum(val);
     ui->slider_th6->setMinimum(val);
     ui->slider_th7->setMinimum(val);
 }

 void EnergyConfiguration::setMaximumEnergy_sliders(int val)
 {
     ui->slider_th0->setMaximum(val);
     ui->slider_th1->setMaximum(val);
     ui->slider_th2->setMaximum(val);
     ui->slider_th3->setMaximum(val);
     ui->slider_th4->setMaximum(val);
     ui->slider_th5->setMaximum(val);
     ui->slider_th6->setMaximum(val);
     ui->slider_th7->setMaximum(val);
 }

 void EnergyConfiguration::setMinimumEnergy_spinBox(double val)
 {
     ui->spinBox_th0->setMinimum(val);
     ui->spinBox_th1->setMinimum(val);
     ui->spinBox_th2->setMinimum(val);
     ui->spinBox_th3->setMinimum(val);
     ui->spinBox_th4->setMinimum(val);
     ui->spinBox_th5->setMinimum(val);
     ui->spinBox_th6->setMinimum(val);
     ui->spinBox_th7->setMinimum(val);
 }

 void EnergyConfiguration::setMaximumEnergy_spinBox(double val)
 {
     ui->spinBox_th0->setMaximum(val);
     ui->spinBox_th1->setMaximum(val);
     ui->spinBox_th2->setMaximum(val);
     ui->spinBox_th3->setMaximum(val);
     ui->spinBox_th4->setMaximum(val);
     ui->spinBox_th5->setMaximum(val);
     ui->spinBox_th6->setMaximum(val);
     ui->spinBox_th7->setMaximum(val);
 }

 void EnergyConfiguration::setActualEnergiesLabel(int threshold, double E)
 {
     QString text;
     /* I.e. ERROR code because the requested energy was too high */
     if (E < 0) {
         text = "ERROR";
     } else {
         text = QString::number(E) + " keV";
     }

     switch (threshold) {
     case 0:
         ui->label_actualEnergies_th0->setText(text);
         break;
     case 1:
         ui->label_actualEnergies_th1->setText(text);
         break;
     case 2:
         ui->label_actualEnergies_th2->setText(text);
         break;
     case 3:
         ui->label_actualEnergies_th3->setText(text);
         break;
     case 4:
         ui->label_actualEnergies_th4->setText(text);
         break;
     case 5:
         ui->label_actualEnergies_th5->setText(text);
         break;
     case 6:
         ui->label_actualEnergies_th6->setText(text);
         break;
     case 7:
         ui->label_actualEnergies_th7->setText(text);
         break;
     default:
         qDebug() << "[ERROR]\tCannot set this actual energy label, th =" << threshold;
         break;
     }
 }

 /*! @brief Enable/disable energy sliders and checkboxes that can be changed.
  *          4 or 8 sliders and checkboxes will be enabled after this
  */
 void EnergyConfiguration::slot_colourModeChanged(bool)
 {
     enablePossibleSlidersAndCheckboxes();
 }

 /*! @brief Enable/disable energy sliders and checkboxes that can be changed.
  *          1 or 2 sliders and checkboxes will be enabled after this
  */
 void EnergyConfiguration::slot_readBothCounters(bool)
 {
     enablePossibleSlidersAndCheckboxes();
 }

 void EnergyConfiguration::enablePossibleSlidersAndCheckboxes()
 {
     if ( _mpx3gui->getConfig()->getColourMode() ) {
         if ( _mpx3gui->getConfig()->getReadBothCounters() ) {
             // first 8 (all) enabled
             ui->slider_th0->setEnabled(1);
             ui->slider_th1->setEnabled(1);
             ui->slider_th2->setEnabled(1);
             ui->slider_th3->setEnabled(1);
             ui->slider_th4->setEnabled(1);
             ui->slider_th5->setEnabled(1);
             ui->slider_th6->setEnabled(1);
             ui->slider_th7->setEnabled(1);

             ui->spinBox_th0->setEnabled(1);
             ui->spinBox_th1->setEnabled(1);
             ui->spinBox_th2->setEnabled(1);
             ui->spinBox_th3->setEnabled(1);
             ui->spinBox_th4->setEnabled(1);
             ui->spinBox_th5->setEnabled(1);
             ui->spinBox_th6->setEnabled(1);
             ui->spinBox_th7->setEnabled(1);

         } else {
             // first 4 enabled
             ui->slider_th0->setEnabled(1);
             ui->slider_th1->setEnabled(1);
             ui->slider_th2->setEnabled(1);
             ui->slider_th3->setEnabled(1);
             ui->slider_th4->setEnabled(0);
             ui->slider_th5->setEnabled(0);
             ui->slider_th6->setEnabled(0);
             ui->slider_th7->setEnabled(0);

             ui->spinBox_th0->setEnabled(1);
             ui->spinBox_th1->setEnabled(1);
             ui->spinBox_th2->setEnabled(1);
             ui->spinBox_th3->setEnabled(1);
             ui->spinBox_th4->setEnabled(0);
             ui->spinBox_th5->setEnabled(0);
             ui->spinBox_th6->setEnabled(0);
             ui->spinBox_th7->setEnabled(0);
         }
     } else {
         if ( _mpx3gui->getConfig()->getReadBothCounters() ) {
             // first 2 enabled
             ui->slider_th0->setEnabled(1);
             ui->slider_th1->setEnabled(1);
             ui->slider_th2->setEnabled(0);
             ui->slider_th3->setEnabled(0);
             ui->slider_th4->setEnabled(0);
             ui->slider_th5->setEnabled(0);
             ui->slider_th6->setEnabled(0);
             ui->slider_th7->setEnabled(0);

             ui->spinBox_th0->setEnabled(1);
             ui->spinBox_th1->setEnabled(1);
             ui->spinBox_th2->setEnabled(0);
             ui->spinBox_th3->setEnabled(0);
             ui->spinBox_th4->setEnabled(0);
             ui->spinBox_th5->setEnabled(0);
             ui->spinBox_th6->setEnabled(0);
             ui->spinBox_th7->setEnabled(0);

         } else {
             // first 1 enabled
             ui->slider_th0->setEnabled(1);
             ui->slider_th1->setEnabled(0);
             ui->slider_th2->setEnabled(0);
             ui->slider_th3->setEnabled(0);
             ui->slider_th4->setEnabled(0);
             ui->slider_th5->setEnabled(0);
             ui->slider_th6->setEnabled(0);
             ui->slider_th7->setEnabled(0);

             ui->spinBox_th0->setEnabled(1);
             ui->spinBox_th1->setEnabled(0);
             ui->spinBox_th2->setEnabled(0);
             ui->spinBox_th3->setEnabled(0);
             ui->spinBox_th4->setEnabled(0);
             ui->spinBox_th5->setEnabled(0);
             ui->spinBox_th6->setEnabled(0);
             ui->spinBox_th7->setEnabled(0);
         }
     }
 }

 void EnergyConfiguration::on_slider_th0_valueChanged(int value)
 {
     ui->spinBox_th0->setValue(value);
 }

 void EnergyConfiguration::on_spinBox_th0_valueChanged(double energy)
 {
     setDACs_all_chips_and_thresholds(0, energy);
     ui->slider_th0->setValue(int(energy));
 }

 void EnergyConfiguration::on_slider_th1_valueChanged(int value)
 {
     ui->spinBox_th1->setValue(value);
 }

 void EnergyConfiguration::on_spinBox_th1_valueChanged(double energy)
 {
     setDACs_all_chips_and_thresholds(1, energy);
     ui->slider_th1->setValue(int(energy));
 }

 void EnergyConfiguration::on_slider_th2_valueChanged(int value)
 {
     ui->spinBox_th2->setValue(value);
 }

 void EnergyConfiguration::on_spinBox_th2_valueChanged(double energy)
 {
     setDACs_all_chips_and_thresholds(2, energy);
     ui->slider_th2->setValue(int(energy));
 }

 void EnergyConfiguration::on_slider_th3_valueChanged(int value)
 {
     ui->spinBox_th3->setValue(value);
 }

 void EnergyConfiguration::on_spinBox_th3_valueChanged(double energy)
 {
     setDACs_all_chips_and_thresholds(3, energy);
     ui->slider_th3->setValue(int(energy));
 }

 void EnergyConfiguration::on_slider_th4_valueChanged(int value)
 {
     ui->spinBox_th4->setValue(value);
 }

 void EnergyConfiguration::on_spinBox_th4_valueChanged(double energy)
 {
     setDACs_all_chips_and_thresholds(4, energy);
     ui->slider_th4->setValue(int(energy));
 }

 void EnergyConfiguration::on_slider_th5_valueChanged(int value)
 {
     ui->spinBox_th5->setValue(value);
 }

 void EnergyConfiguration::on_spinBox_th5_valueChanged(double energy)
 {
     setDACs_all_chips_and_thresholds(5, energy);
     ui->slider_th5->setValue(int(energy));
 }

 void EnergyConfiguration::on_slider_th6_valueChanged(int value)
 {
     ui->spinBox_th6->setValue(value);
 }

 void EnergyConfiguration::on_spinBox_th6_valueChanged(double energy)
 {
     setDACs_all_chips_and_thresholds(6, energy);
     ui->slider_th6->setValue(int(energy));
 }

 void EnergyConfiguration::on_slider_th7_valueChanged(int value)
 {
     ui->spinBox_th7->setValue(value);
 }

 void EnergyConfiguration::on_spinBox_th7_valueChanged(double energy)
 {
     setDACs_all_chips_and_thresholds(7, energy);

     ui->slider_th7->setValue(int(energy));
 }
