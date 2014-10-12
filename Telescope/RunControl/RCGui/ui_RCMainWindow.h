/********************************************************************************
** Form generated from reading UI file 'RCMainWindow.ui'
**
** Created: Sat Oct 11 19:09:09 2014
**      by: Qt User Interface Compiler version 4.6.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_RCMAINWINDOW_H
#define UI_RCMAINWINDOW_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QCheckBox>
#include <QtGui/QGridLayout>
#include <QtGui/QGroupBox>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QMainWindow>
#include <QtGui/QPushButton>
#include <QtGui/QSpinBox>
#include <QtGui/QStatusBar>
#include <QtGui/QTextBrowser>
#include <QtGui/QToolBar>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class Ui_RCMainWindow
{
public:
    QWidget *centralWidget;
    QGridLayout *gridLayout;
    QGroupBox *b_output;
    QGridLayout *gridLayout_2;
    QTextBrowser *t_output;
    QGroupBox *b_settings;
    QGridLayout *gridLayout_4;
    QLabel *label;
    QSpinBox *b_runNumber;
    QLabel *label_2;
    QTextBrowser *t_header;
    QLabel *label_3;
    QWidget *widget;
    QGridLayout *gridLayout_6;
    QSpinBox *b_z0;
    QSpinBox *b_z1;
    QSpinBox *b_z2;
    QSpinBox *b_z3;
    QSpinBox *b_z4;
    QSpinBox *b_z6;
    QSpinBox *b_z5;
    QSpinBox *b_z7;
    QSpinBox *b_z8;
    QSpinBox *b_z9;
    QCheckBox *b_includeZposns;
    QPushButton *b_forceSaveRecords;
    QLabel *l_logo;
    QGroupBox *b_actions;
    QGridLayout *gridLayout_3;
    QPushButton *b_configure;
    QPushButton *b_init;
    QPushButton *b_startRun;
    QPushButton *b_endRun;
    QLabel *label_4;
    QGroupBox *b_monitoring;
    QGridLayout *gridLayout_5;
    QPushButton *b_startMonitoring;
    QPushButton *b_endMonitoring;
    QPushButton *b_quit;
    QPushButton *b_DQM;
    QPushButton *b_FQM;
    QToolBar *mainToolBar;
    QStatusBar *statusBar;

    void setupUi(QMainWindow *RCMainWindow)
    {
        if (RCMainWindow->objectName().isEmpty())
            RCMainWindow->setObjectName(QString::fromUtf8("RCMainWindow"));
        RCMainWindow->resize(500, 1000);
        centralWidget = new QWidget(RCMainWindow);
        centralWidget->setObjectName(QString::fromUtf8("centralWidget"));
        gridLayout = new QGridLayout(centralWidget);
        gridLayout->setSpacing(6);
        gridLayout->setContentsMargins(6, 6, 6, 6);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        b_output = new QGroupBox(centralWidget);
        b_output->setObjectName(QString::fromUtf8("b_output"));
        gridLayout_2 = new QGridLayout(b_output);
        gridLayout_2->setSpacing(6);
        gridLayout_2->setContentsMargins(6, 6, 6, 6);
        gridLayout_2->setObjectName(QString::fromUtf8("gridLayout_2"));
        t_output = new QTextBrowser(b_output);
        t_output->setObjectName(QString::fromUtf8("t_output"));
        QFont font;
        font.setFamily(QString::fromUtf8("Monospace"));
        font.setPointSize(9);
        t_output->setFont(font);

        gridLayout_2->addWidget(t_output, 0, 0, 1, 1);


        gridLayout->addWidget(b_output, 4, 0, 1, 1);

        b_settings = new QGroupBox(centralWidget);
        b_settings->setObjectName(QString::fromUtf8("b_settings"));
        b_settings->setEnabled(true);
        gridLayout_4 = new QGridLayout(b_settings);
        gridLayout_4->setSpacing(6);
        gridLayout_4->setContentsMargins(6, 6, 6, 6);
        gridLayout_4->setObjectName(QString::fromUtf8("gridLayout_4"));
        label = new QLabel(b_settings);
        label->setObjectName(QString::fromUtf8("label"));
        QFont font1;
        font1.setFamily(QString::fromUtf8("Sans Serif"));
        font1.setPointSize(9);
        label->setFont(font1);

        gridLayout_4->addWidget(label, 0, 0, 1, 1);

        b_runNumber = new QSpinBox(b_settings);
        b_runNumber->setObjectName(QString::fromUtf8("b_runNumber"));
        b_runNumber->setEnabled(true);
        b_runNumber->setFont(font1);
        b_runNumber->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
        b_runNumber->setMaximum(50000);

        gridLayout_4->addWidget(b_runNumber, 0, 1, 1, 1);

        label_2 = new QLabel(b_settings);
        label_2->setObjectName(QString::fromUtf8("label_2"));
        label_2->setFont(font1);

        gridLayout_4->addWidget(label_2, 1, 0, 1, 1);

        t_header = new QTextBrowser(b_settings);
        t_header->setObjectName(QString::fromUtf8("t_header"));
        t_header->setEnabled(true);
        t_header->setMaximumSize(QSize(16777215, 75));
        t_header->setFont(font);

        gridLayout_4->addWidget(t_header, 2, 0, 1, 2);

        label_3 = new QLabel(b_settings);
        label_3->setObjectName(QString::fromUtf8("label_3"));
        label_3->setFont(font1);

        gridLayout_4->addWidget(label_3, 3, 0, 1, 2);

        widget = new QWidget(b_settings);
        widget->setObjectName(QString::fromUtf8("widget"));
        widget->setFont(font1);
        gridLayout_6 = new QGridLayout(widget);
        gridLayout_6->setSpacing(3);
        gridLayout_6->setContentsMargins(0, 0, 0, 0);
        gridLayout_6->setObjectName(QString::fromUtf8("gridLayout_6"));
        b_z0 = new QSpinBox(widget);
        b_z0->setObjectName(QString::fromUtf8("b_z0"));
        b_z0->setFont(font1);
        b_z0->setMinimum(-1);
        b_z0->setMaximum(10000);

        gridLayout_6->addWidget(b_z0, 0, 0, 1, 1);

        b_z1 = new QSpinBox(widget);
        b_z1->setObjectName(QString::fromUtf8("b_z1"));
        b_z1->setMinimum(-1);
        b_z1->setMaximum(10000);
        b_z1->setValue(100);

        gridLayout_6->addWidget(b_z1, 0, 1, 1, 1);

        b_z2 = new QSpinBox(widget);
        b_z2->setObjectName(QString::fromUtf8("b_z2"));
        b_z2->setFont(font1);
        b_z2->setMinimum(-1);
        b_z2->setMaximum(10000);
        b_z2->setValue(200);

        gridLayout_6->addWidget(b_z2, 0, 2, 1, 1);

        b_z3 = new QSpinBox(widget);
        b_z3->setObjectName(QString::fromUtf8("b_z3"));
        b_z3->setFont(font1);
        b_z3->setMinimum(-1);
        b_z3->setMaximum(10000);
        b_z3->setValue(300);

        gridLayout_6->addWidget(b_z3, 0, 3, 1, 1);

        b_z4 = new QSpinBox(widget);
        b_z4->setObjectName(QString::fromUtf8("b_z4"));
        b_z4->setFont(font1);
        b_z4->setMinimum(-1);
        b_z4->setMaximum(10000);
        b_z4->setValue(400);

        gridLayout_6->addWidget(b_z4, 0, 4, 1, 1);

        b_z6 = new QSpinBox(widget);
        b_z6->setObjectName(QString::fromUtf8("b_z6"));
        b_z6->setFont(font1);
        b_z6->setMinimum(-1);
        b_z6->setMaximum(10000);
        b_z6->setValue(600);

        gridLayout_6->addWidget(b_z6, 1, 1, 1, 1);

        b_z5 = new QSpinBox(widget);
        b_z5->setObjectName(QString::fromUtf8("b_z5"));
        b_z5->setFont(font1);
        b_z5->setMinimum(-1);
        b_z5->setMaximum(10000);
        b_z5->setValue(500);

        gridLayout_6->addWidget(b_z5, 1, 0, 1, 1);

        b_z7 = new QSpinBox(widget);
        b_z7->setObjectName(QString::fromUtf8("b_z7"));
        b_z7->setMinimum(-1);
        b_z7->setMaximum(10000);
        b_z7->setValue(700);

        gridLayout_6->addWidget(b_z7, 1, 2, 1, 1);

        b_z8 = new QSpinBox(widget);
        b_z8->setObjectName(QString::fromUtf8("b_z8"));
        b_z8->setFont(font1);
        b_z8->setMinimum(-1);
        b_z8->setMaximum(10000);
        b_z8->setValue(-1);

        gridLayout_6->addWidget(b_z8, 1, 3, 1, 1);

        b_z9 = new QSpinBox(widget);
        b_z9->setObjectName(QString::fromUtf8("b_z9"));
        b_z9->setFont(font1);
        b_z9->setMinimum(-1);
        b_z9->setMaximum(10000);
        b_z9->setValue(-1);

        gridLayout_6->addWidget(b_z9, 1, 4, 1, 1);

        b_includeZposns = new QCheckBox(widget);
        b_includeZposns->setObjectName(QString::fromUtf8("b_includeZposns"));
        b_includeZposns->setFont(font1);
        b_includeZposns->setLayoutDirection(Qt::LeftToRight);
        b_includeZposns->setChecked(true);

        gridLayout_6->addWidget(b_includeZposns, 2, 0, 1, 4);

        b_forceSaveRecords = new QPushButton(widget);
        b_forceSaveRecords->setObjectName(QString::fromUtf8("b_forceSaveRecords"));
        b_forceSaveRecords->setLayoutDirection(Qt::LeftToRight);

        gridLayout_6->addWidget(b_forceSaveRecords, 2, 4, 1, 1);


        gridLayout_4->addWidget(widget, 4, 0, 1, 2);


        gridLayout->addWidget(b_settings, 3, 0, 1, 1);

        l_logo = new QLabel(centralWidget);
        l_logo->setObjectName(QString::fromUtf8("l_logo"));

        gridLayout->addWidget(l_logo, 0, 0, 1, 1);

        b_actions = new QGroupBox(centralWidget);
        b_actions->setObjectName(QString::fromUtf8("b_actions"));
        gridLayout_3 = new QGridLayout(b_actions);
        gridLayout_3->setSpacing(5);
        gridLayout_3->setContentsMargins(6, 6, 6, 6);
        gridLayout_3->setObjectName(QString::fromUtf8("gridLayout_3"));
        b_configure = new QPushButton(b_actions);
        b_configure->setObjectName(QString::fromUtf8("b_configure"));
        b_configure->setEnabled(false);
        b_configure->setFont(font1);

        gridLayout_3->addWidget(b_configure, 0, 1, 1, 1);

        b_init = new QPushButton(b_actions);
        b_init->setObjectName(QString::fromUtf8("b_init"));
        b_init->setFont(font1);

        gridLayout_3->addWidget(b_init, 0, 0, 1, 1);

        b_startRun = new QPushButton(b_actions);
        b_startRun->setObjectName(QString::fromUtf8("b_startRun"));
        b_startRun->setEnabled(false);
        b_startRun->setFont(font1);

        gridLayout_3->addWidget(b_startRun, 0, 2, 1, 1);

        b_endRun = new QPushButton(b_actions);
        b_endRun->setObjectName(QString::fromUtf8("b_endRun"));
        b_endRun->setEnabled(false);
        b_endRun->setFont(font1);

        gridLayout_3->addWidget(b_endRun, 0, 3, 1, 1);


        gridLayout->addWidget(b_actions, 2, 0, 1, 1);

        label_4 = new QLabel(centralWidget);
        label_4->setObjectName(QString::fromUtf8("label_4"));
        label_4->setFont(font1);
        label_4->setWordWrap(true);

        gridLayout->addWidget(label_4, 1, 0, 1, 1);

        b_monitoring = new QGroupBox(centralWidget);
        b_monitoring->setObjectName(QString::fromUtf8("b_monitoring"));
        gridLayout_5 = new QGridLayout(b_monitoring);
        gridLayout_5->setSpacing(6);
        gridLayout_5->setContentsMargins(6, 6, 6, 6);
        gridLayout_5->setObjectName(QString::fromUtf8("gridLayout_5"));
        b_startMonitoring = new QPushButton(b_monitoring);
        b_startMonitoring->setObjectName(QString::fromUtf8("b_startMonitoring"));
        b_startMonitoring->setEnabled(false);
        b_startMonitoring->setFont(font1);

        gridLayout_5->addWidget(b_startMonitoring, 0, 0, 1, 2);

        b_endMonitoring = new QPushButton(b_monitoring);
        b_endMonitoring->setObjectName(QString::fromUtf8("b_endMonitoring"));
        b_endMonitoring->setEnabled(false);
        b_endMonitoring->setFont(font1);

        gridLayout_5->addWidget(b_endMonitoring, 0, 2, 1, 2);

        b_quit = new QPushButton(b_monitoring);
        b_quit->setObjectName(QString::fromUtf8("b_quit"));

        gridLayout_5->addWidget(b_quit, 2, 0, 1, 6);

        b_DQM = new QPushButton(b_monitoring);
        b_DQM->setObjectName(QString::fromUtf8("b_DQM"));
        b_DQM->setFont(font1);

        gridLayout_5->addWidget(b_DQM, 0, 4, 1, 1);

        b_FQM = new QPushButton(b_monitoring);
        b_FQM->setObjectName(QString::fromUtf8("b_FQM"));

        gridLayout_5->addWidget(b_FQM, 0, 5, 1, 1);


        gridLayout->addWidget(b_monitoring, 5, 0, 1, 1);

        RCMainWindow->setCentralWidget(centralWidget);
        mainToolBar = new QToolBar(RCMainWindow);
        mainToolBar->setObjectName(QString::fromUtf8("mainToolBar"));
        RCMainWindow->addToolBar(Qt::TopToolBarArea, mainToolBar);
        statusBar = new QStatusBar(RCMainWindow);
        statusBar->setObjectName(QString::fromUtf8("statusBar"));
        RCMainWindow->setStatusBar(statusBar);

        retranslateUi(RCMainWindow);

        QMetaObject::connectSlotsByName(RCMainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *RCMainWindow)
    {
        RCMainWindow->setWindowTitle(QApplication::translate("RCMainWindow", "RCMainWindow", 0, QApplication::UnicodeUTF8));
        b_output->setTitle(QApplication::translate("RCMainWindow", "Ouput", 0, QApplication::UnicodeUTF8));
        b_settings->setTitle(QApplication::translate("RCMainWindow", "Run Settings", 0, QApplication::UnicodeUTF8));
        label->setText(QApplication::translate("RCMainWindow", "Next run number:", 0, QApplication::UnicodeUTF8));
        label_2->setText(QApplication::translate("RCMainWindow", "Header text:", 0, QApplication::UnicodeUTF8));
        t_header->setHtml(QApplication::translate("RCMainWindow", "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">\n"
"<html><head><meta name=\"qrichtext\" content=\"1\" /><style type=\"text/css\">\n"
"p, li { white-space: pre-wrap; }\n"
"</style></head><body style=\" font-family:'Monospace'; font-size:9pt; font-weight:400; font-style:normal;\">\n"
"<p style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><span style=\" font-family:'.Lucida Grande UI'; font-size:13pt;\">Default header text.</span></p></body></html>", 0, QApplication::UnicodeUTF8));
        label_3->setText(QApplication::translate("RCMainWindow", "Plane positions in increasing Z /mm (for file header. Use -1 for not present):", 0, QApplication::UnicodeUTF8));
        b_includeZposns->setText(QApplication::translate("RCMainWindow", "Include in file headers (~20/128 characters extra)", 0, QApplication::UnicodeUTF8));
        b_forceSaveRecords->setText(QApplication::translate("RCMainWindow", "Save", 0, QApplication::UnicodeUTF8));
        l_logo->setText(QApplication::translate("RCMainWindow", "Timepix3 Telescope Run Control", 0, QApplication::UnicodeUTF8));
        b_actions->setTitle(QApplication::translate("RCMainWindow", "Telescope Commands", 0, QApplication::UnicodeUTF8));
        b_configure->setText(QApplication::translate("RCMainWindow", "Configure", 0, QApplication::UnicodeUTF8));
        b_init->setText(QApplication::translate("RCMainWindow", "Initialise", 0, QApplication::UnicodeUTF8));
        b_startRun->setText(QApplication::translate("RCMainWindow", "Start Run", 0, QApplication::UnicodeUTF8));
        b_endRun->setText(QApplication::translate("RCMainWindow", "Stop Run", 0, QApplication::UnicodeUTF8));
        label_4->setText(QApplication::translate("RCMainWindow", "Notes: the DAQ processes should be started prior to initialising the Run Control. The run number will auto increment. The Z positions should be set if changed.", 0, QApplication::UnicodeUTF8));
        b_monitoring->setTitle(QApplication::translate("RCMainWindow", "Monitoring", 0, QApplication::UnicodeUTF8));
        b_startMonitoring->setText(QApplication::translate("RCMainWindow", "Start Monitoring", 0, QApplication::UnicodeUTF8));
        b_endMonitoring->setText(QApplication::translate("RCMainWindow", "End Monitoring", 0, QApplication::UnicodeUTF8));
        b_quit->setText(QApplication::translate("RCMainWindow", "Quit", 0, QApplication::UnicodeUTF8));
        b_DQM->setText(QApplication::translate("RCMainWindow", "DQM", 0, QApplication::UnicodeUTF8));
        b_FQM->setText(QApplication::translate("RCMainWindow", "FQM", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class RCMainWindow: public Ui_RCMainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_RCMAINWINDOW_H
