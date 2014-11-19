/********************************************************************************
** Form generated from reading UI file 'spidrmpx3eq.ui'
**
** Created by: Qt User Interface Compiler version 5.2.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_SPIDRMPX3EQ_H
#define UI_SPIDRMPX3EQ_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QFrame>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSlider>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_SpidrMpx3Eq
{
public:
    QAction *actionConnect;
    QAction *actionExit;
    QWidget *centralWidget;
    QTabWidget *_tabDACs;
    QWidget *tab;
    QWidget *verticalLayoutWidget;
    QVBoxLayout *verticalLayout;
    QPushButton *_connect;
    QPushButton *_startEq;
    QLabel *_statusLabel;
    QFrame *_histoFrame;
    QWidget *tab_2;
    QFrame *_DACScanFrame;
    QWidget *horizontalLayoutWidget;
    QHBoxLayout *horizontalLayout;
    QVBoxLayout *verticalLayout_2;
    QSlider *dac0hSlider;
    QSlider *dac1hSlider;
    QSlider *dac2hSlider;
    QSlider *dac3hSlider;
    QSlider *dac4hSlider;
    QSlider *dac5hSlider;
    QSlider *dac6hSlider;
    QSlider *dac7hSlider;
    QSlider *dac8hSlider;
    QSlider *dac9hSlider;
    QSlider *dac10hSlider;
    QSlider *dac11hSlider;
    QSlider *dac12hSlider;
    QSlider *dac13hSlider;
    QSlider *dac14hSlider;
    QSlider *dac15hSlider;
    QSlider *dac16hSlider;
    QSlider *dac17hSlider;
    QSlider *dac18hSlider;
    QSlider *dac19hSlider;
    QSlider *dac20hSlider;
    QSlider *dac21hSlider;
    QSlider *dac22hSlider;
    QSlider *dac23hSlider;
    QSlider *dac24hSlider;
    QSlider *dac25hSlider;
    QSlider *dac26hSlider;
    QVBoxLayout *verticalLayout_5;
    QLineEdit *lineEdit_2;
    QLineEdit *lineEdit_3;
    QLineEdit *lineEdit;
    QLineEdit *lineEdit_4;
    QLineEdit *lineEdit_5;
    QLineEdit *lineEdit_6;
    QLineEdit *lineEdit_7;
    QLineEdit *lineEdit_8;
    QLineEdit *lineEdit_9;
    QLineEdit *lineEdit_10;
    QLineEdit *lineEdit_11;
    QLineEdit *lineEdit_12;
    QLineEdit *lineEdit_13;
    QLineEdit *lineEdit_14;
    QLineEdit *lineEdit_15;
    QLineEdit *lineEdit_16;
    QLineEdit *lineEdit_17;
    QLineEdit *lineEdit_18;
    QLineEdit *lineEdit_19;
    QLineEdit *lineEdit_20;
    QLineEdit *lineEdit_21;
    QMenuBar *menuBar;
    QMenu *menuMpx3Eq;
    QToolBar *mainToolBar;
    QStatusBar *statusBar;

    void setupUi(QMainWindow *SpidrMpx3Eq)
    {
        if (SpidrMpx3Eq->objectName().isEmpty())
            SpidrMpx3Eq->setObjectName(QStringLiteral("SpidrMpx3Eq"));
        SpidrMpx3Eq->resize(1051, 710);
        actionConnect = new QAction(SpidrMpx3Eq);
        actionConnect->setObjectName(QStringLiteral("actionConnect"));
        actionExit = new QAction(SpidrMpx3Eq);
        actionExit->setObjectName(QStringLiteral("actionExit"));
        centralWidget = new QWidget(SpidrMpx3Eq);
        centralWidget->setObjectName(QStringLiteral("centralWidget"));
        _tabDACs = new QTabWidget(centralWidget);
        _tabDACs->setObjectName(QStringLiteral("_tabDACs"));
        _tabDACs->setGeometry(QRect(0, 10, 1051, 641));
        tab = new QWidget();
        tab->setObjectName(QStringLiteral("tab"));
        verticalLayoutWidget = new QWidget(tab);
        verticalLayoutWidget->setObjectName(QStringLiteral("verticalLayoutWidget"));
        verticalLayoutWidget->setGeometry(QRect(760, 470, 241, 85));
        verticalLayout = new QVBoxLayout(verticalLayoutWidget);
        verticalLayout->setSpacing(6);
        verticalLayout->setContentsMargins(11, 11, 11, 11);
        verticalLayout->setObjectName(QStringLiteral("verticalLayout"));
        verticalLayout->setContentsMargins(0, 0, 0, 0);
        _connect = new QPushButton(verticalLayoutWidget);
        _connect->setObjectName(QStringLiteral("_connect"));

        verticalLayout->addWidget(_connect);

        _startEq = new QPushButton(verticalLayoutWidget);
        _startEq->setObjectName(QStringLiteral("_startEq"));

        verticalLayout->addWidget(_startEq);

        _statusLabel = new QLabel(verticalLayoutWidget);
        _statusLabel->setObjectName(QStringLiteral("_statusLabel"));

        verticalLayout->addWidget(_statusLabel);

        _histoFrame = new QFrame(tab);
        _histoFrame->setObjectName(QStringLiteral("_histoFrame"));
        _histoFrame->setGeometry(QRect(10, 20, 491, 361));
        _histoFrame->setFrameShape(QFrame::StyledPanel);
        _histoFrame->setFrameShadow(QFrame::Raised);
        _tabDACs->addTab(tab, QString());
        tab_2 = new QWidget();
        tab_2->setObjectName(QStringLiteral("tab_2"));
        _DACScanFrame = new QFrame(tab_2);
        _DACScanFrame->setObjectName(QStringLiteral("_DACScanFrame"));
        _DACScanFrame->setGeometry(QRect(590, 20, 431, 331));
        _DACScanFrame->setFrameShape(QFrame::StyledPanel);
        _DACScanFrame->setFrameShadow(QFrame::Raised);
        horizontalLayoutWidget = new QWidget(tab_2);
        horizontalLayoutWidget->setObjectName(QStringLiteral("horizontalLayoutWidget"));
        horizontalLayoutWidget->setGeometry(QRect(20, 20, 381, 607));
        horizontalLayout = new QHBoxLayout(horizontalLayoutWidget);
        horizontalLayout->setSpacing(6);
        horizontalLayout->setContentsMargins(11, 11, 11, 11);
        horizontalLayout->setObjectName(QStringLiteral("horizontalLayout"));
        horizontalLayout->setContentsMargins(0, 0, 0, 0);
        verticalLayout_2 = new QVBoxLayout();
        verticalLayout_2->setSpacing(6);
        verticalLayout_2->setObjectName(QStringLiteral("verticalLayout_2"));
        dac0hSlider = new QSlider(horizontalLayoutWidget);
        dac0hSlider->setObjectName(QStringLiteral("dac0hSlider"));
        dac0hSlider->setOrientation(Qt::Horizontal);

        verticalLayout_2->addWidget(dac0hSlider);

        dac1hSlider = new QSlider(horizontalLayoutWidget);
        dac1hSlider->setObjectName(QStringLiteral("dac1hSlider"));
        dac1hSlider->setOrientation(Qt::Horizontal);

        verticalLayout_2->addWidget(dac1hSlider);

        dac2hSlider = new QSlider(horizontalLayoutWidget);
        dac2hSlider->setObjectName(QStringLiteral("dac2hSlider"));
        dac2hSlider->setOrientation(Qt::Horizontal);

        verticalLayout_2->addWidget(dac2hSlider);

        dac3hSlider = new QSlider(horizontalLayoutWidget);
        dac3hSlider->setObjectName(QStringLiteral("dac3hSlider"));
        dac3hSlider->setOrientation(Qt::Horizontal);

        verticalLayout_2->addWidget(dac3hSlider);

        dac4hSlider = new QSlider(horizontalLayoutWidget);
        dac4hSlider->setObjectName(QStringLiteral("dac4hSlider"));
        dac4hSlider->setOrientation(Qt::Horizontal);

        verticalLayout_2->addWidget(dac4hSlider);

        dac5hSlider = new QSlider(horizontalLayoutWidget);
        dac5hSlider->setObjectName(QStringLiteral("dac5hSlider"));
        dac5hSlider->setOrientation(Qt::Horizontal);

        verticalLayout_2->addWidget(dac5hSlider);

        dac6hSlider = new QSlider(horizontalLayoutWidget);
        dac6hSlider->setObjectName(QStringLiteral("dac6hSlider"));
        dac6hSlider->setOrientation(Qt::Horizontal);

        verticalLayout_2->addWidget(dac6hSlider);

        dac7hSlider = new QSlider(horizontalLayoutWidget);
        dac7hSlider->setObjectName(QStringLiteral("dac7hSlider"));
        dac7hSlider->setOrientation(Qt::Horizontal);

        verticalLayout_2->addWidget(dac7hSlider);

        dac8hSlider = new QSlider(horizontalLayoutWidget);
        dac8hSlider->setObjectName(QStringLiteral("dac8hSlider"));
        dac8hSlider->setOrientation(Qt::Horizontal);

        verticalLayout_2->addWidget(dac8hSlider);

        dac9hSlider = new QSlider(horizontalLayoutWidget);
        dac9hSlider->setObjectName(QStringLiteral("dac9hSlider"));
        dac9hSlider->setOrientation(Qt::Horizontal);

        verticalLayout_2->addWidget(dac9hSlider);

        dac10hSlider = new QSlider(horizontalLayoutWidget);
        dac10hSlider->setObjectName(QStringLiteral("dac10hSlider"));
        dac10hSlider->setOrientation(Qt::Horizontal);

        verticalLayout_2->addWidget(dac10hSlider);

        dac11hSlider = new QSlider(horizontalLayoutWidget);
        dac11hSlider->setObjectName(QStringLiteral("dac11hSlider"));
        dac11hSlider->setOrientation(Qt::Horizontal);

        verticalLayout_2->addWidget(dac11hSlider);

        dac12hSlider = new QSlider(horizontalLayoutWidget);
        dac12hSlider->setObjectName(QStringLiteral("dac12hSlider"));
        dac12hSlider->setOrientation(Qt::Horizontal);

        verticalLayout_2->addWidget(dac12hSlider);

        dac13hSlider = new QSlider(horizontalLayoutWidget);
        dac13hSlider->setObjectName(QStringLiteral("dac13hSlider"));
        dac13hSlider->setOrientation(Qt::Horizontal);

        verticalLayout_2->addWidget(dac13hSlider);

        dac14hSlider = new QSlider(horizontalLayoutWidget);
        dac14hSlider->setObjectName(QStringLiteral("dac14hSlider"));
        dac14hSlider->setOrientation(Qt::Horizontal);

        verticalLayout_2->addWidget(dac14hSlider);

        dac15hSlider = new QSlider(horizontalLayoutWidget);
        dac15hSlider->setObjectName(QStringLiteral("dac15hSlider"));
        dac15hSlider->setOrientation(Qt::Horizontal);

        verticalLayout_2->addWidget(dac15hSlider);

        dac16hSlider = new QSlider(horizontalLayoutWidget);
        dac16hSlider->setObjectName(QStringLiteral("dac16hSlider"));
        dac16hSlider->setOrientation(Qt::Horizontal);

        verticalLayout_2->addWidget(dac16hSlider);

        dac17hSlider = new QSlider(horizontalLayoutWidget);
        dac17hSlider->setObjectName(QStringLiteral("dac17hSlider"));
        dac17hSlider->setOrientation(Qt::Horizontal);

        verticalLayout_2->addWidget(dac17hSlider);

        dac18hSlider = new QSlider(horizontalLayoutWidget);
        dac18hSlider->setObjectName(QStringLiteral("dac18hSlider"));
        dac18hSlider->setOrientation(Qt::Horizontal);

        verticalLayout_2->addWidget(dac18hSlider);

        dac19hSlider = new QSlider(horizontalLayoutWidget);
        dac19hSlider->setObjectName(QStringLiteral("dac19hSlider"));
        dac19hSlider->setOrientation(Qt::Horizontal);

        verticalLayout_2->addWidget(dac19hSlider);

        dac20hSlider = new QSlider(horizontalLayoutWidget);
        dac20hSlider->setObjectName(QStringLiteral("dac20hSlider"));
        dac20hSlider->setOrientation(Qt::Horizontal);

        verticalLayout_2->addWidget(dac20hSlider);

        dac21hSlider = new QSlider(horizontalLayoutWidget);
        dac21hSlider->setObjectName(QStringLiteral("dac21hSlider"));
        dac21hSlider->setOrientation(Qt::Horizontal);

        verticalLayout_2->addWidget(dac21hSlider);

        dac22hSlider = new QSlider(horizontalLayoutWidget);
        dac22hSlider->setObjectName(QStringLiteral("dac22hSlider"));
        dac22hSlider->setOrientation(Qt::Horizontal);

        verticalLayout_2->addWidget(dac22hSlider);

        dac23hSlider = new QSlider(horizontalLayoutWidget);
        dac23hSlider->setObjectName(QStringLiteral("dac23hSlider"));
        dac23hSlider->setOrientation(Qt::Horizontal);

        verticalLayout_2->addWidget(dac23hSlider);

        dac24hSlider = new QSlider(horizontalLayoutWidget);
        dac24hSlider->setObjectName(QStringLiteral("dac24hSlider"));
        dac24hSlider->setOrientation(Qt::Horizontal);

        verticalLayout_2->addWidget(dac24hSlider);

        dac25hSlider = new QSlider(horizontalLayoutWidget);
        dac25hSlider->setObjectName(QStringLiteral("dac25hSlider"));
        dac25hSlider->setOrientation(Qt::Horizontal);

        verticalLayout_2->addWidget(dac25hSlider);

        dac26hSlider = new QSlider(horizontalLayoutWidget);
        dac26hSlider->setObjectName(QStringLiteral("dac26hSlider"));
        dac26hSlider->setOrientation(Qt::Horizontal);

        verticalLayout_2->addWidget(dac26hSlider);


        horizontalLayout->addLayout(verticalLayout_2);

        verticalLayout_5 = new QVBoxLayout();
        verticalLayout_5->setSpacing(6);
        verticalLayout_5->setObjectName(QStringLiteral("verticalLayout_5"));
        lineEdit_2 = new QLineEdit(horizontalLayoutWidget);
        lineEdit_2->setObjectName(QStringLiteral("lineEdit_2"));

        verticalLayout_5->addWidget(lineEdit_2);

        lineEdit_3 = new QLineEdit(horizontalLayoutWidget);
        lineEdit_3->setObjectName(QStringLiteral("lineEdit_3"));

        verticalLayout_5->addWidget(lineEdit_3);

        lineEdit = new QLineEdit(horizontalLayoutWidget);
        lineEdit->setObjectName(QStringLiteral("lineEdit"));

        verticalLayout_5->addWidget(lineEdit);

        lineEdit_4 = new QLineEdit(horizontalLayoutWidget);
        lineEdit_4->setObjectName(QStringLiteral("lineEdit_4"));

        verticalLayout_5->addWidget(lineEdit_4);

        lineEdit_5 = new QLineEdit(horizontalLayoutWidget);
        lineEdit_5->setObjectName(QStringLiteral("lineEdit_5"));

        verticalLayout_5->addWidget(lineEdit_5);

        lineEdit_6 = new QLineEdit(horizontalLayoutWidget);
        lineEdit_6->setObjectName(QStringLiteral("lineEdit_6"));

        verticalLayout_5->addWidget(lineEdit_6);

        lineEdit_7 = new QLineEdit(horizontalLayoutWidget);
        lineEdit_7->setObjectName(QStringLiteral("lineEdit_7"));

        verticalLayout_5->addWidget(lineEdit_7);

        lineEdit_8 = new QLineEdit(horizontalLayoutWidget);
        lineEdit_8->setObjectName(QStringLiteral("lineEdit_8"));

        verticalLayout_5->addWidget(lineEdit_8);

        lineEdit_9 = new QLineEdit(horizontalLayoutWidget);
        lineEdit_9->setObjectName(QStringLiteral("lineEdit_9"));

        verticalLayout_5->addWidget(lineEdit_9);

        lineEdit_10 = new QLineEdit(horizontalLayoutWidget);
        lineEdit_10->setObjectName(QStringLiteral("lineEdit_10"));

        verticalLayout_5->addWidget(lineEdit_10);

        lineEdit_11 = new QLineEdit(horizontalLayoutWidget);
        lineEdit_11->setObjectName(QStringLiteral("lineEdit_11"));

        verticalLayout_5->addWidget(lineEdit_11);

        lineEdit_12 = new QLineEdit(horizontalLayoutWidget);
        lineEdit_12->setObjectName(QStringLiteral("lineEdit_12"));

        verticalLayout_5->addWidget(lineEdit_12);

        lineEdit_13 = new QLineEdit(horizontalLayoutWidget);
        lineEdit_13->setObjectName(QStringLiteral("lineEdit_13"));

        verticalLayout_5->addWidget(lineEdit_13);

        lineEdit_14 = new QLineEdit(horizontalLayoutWidget);
        lineEdit_14->setObjectName(QStringLiteral("lineEdit_14"));

        verticalLayout_5->addWidget(lineEdit_14);

        lineEdit_15 = new QLineEdit(horizontalLayoutWidget);
        lineEdit_15->setObjectName(QStringLiteral("lineEdit_15"));

        verticalLayout_5->addWidget(lineEdit_15);

        lineEdit_16 = new QLineEdit(horizontalLayoutWidget);
        lineEdit_16->setObjectName(QStringLiteral("lineEdit_16"));

        verticalLayout_5->addWidget(lineEdit_16);

        lineEdit_17 = new QLineEdit(horizontalLayoutWidget);
        lineEdit_17->setObjectName(QStringLiteral("lineEdit_17"));

        verticalLayout_5->addWidget(lineEdit_17);

        lineEdit_18 = new QLineEdit(horizontalLayoutWidget);
        lineEdit_18->setObjectName(QStringLiteral("lineEdit_18"));

        verticalLayout_5->addWidget(lineEdit_18);

        lineEdit_19 = new QLineEdit(horizontalLayoutWidget);
        lineEdit_19->setObjectName(QStringLiteral("lineEdit_19"));

        verticalLayout_5->addWidget(lineEdit_19);

        lineEdit_20 = new QLineEdit(horizontalLayoutWidget);
        lineEdit_20->setObjectName(QStringLiteral("lineEdit_20"));

        verticalLayout_5->addWidget(lineEdit_20);

        lineEdit_21 = new QLineEdit(horizontalLayoutWidget);
        lineEdit_21->setObjectName(QStringLiteral("lineEdit_21"));

        verticalLayout_5->addWidget(lineEdit_21);


        horizontalLayout->addLayout(verticalLayout_5);

        _tabDACs->addTab(tab_2, QString());
        SpidrMpx3Eq->setCentralWidget(centralWidget);
        menuBar = new QMenuBar(SpidrMpx3Eq);
        menuBar->setObjectName(QStringLiteral("menuBar"));
        menuBar->setGeometry(QRect(0, 0, 1051, 20));
        menuMpx3Eq = new QMenu(menuBar);
        menuMpx3Eq->setObjectName(QStringLiteral("menuMpx3Eq"));
        SpidrMpx3Eq->setMenuBar(menuBar);
        mainToolBar = new QToolBar(SpidrMpx3Eq);
        mainToolBar->setObjectName(QStringLiteral("mainToolBar"));
        SpidrMpx3Eq->addToolBar(Qt::TopToolBarArea, mainToolBar);
        statusBar = new QStatusBar(SpidrMpx3Eq);
        statusBar->setObjectName(QStringLiteral("statusBar"));
        SpidrMpx3Eq->setStatusBar(statusBar);

        menuBar->addAction(menuMpx3Eq->menuAction());
        menuMpx3Eq->addSeparator();
        menuMpx3Eq->addAction(actionConnect);
        menuMpx3Eq->addAction(actionExit);

        retranslateUi(SpidrMpx3Eq);

        _tabDACs->setCurrentIndex(1);


        QMetaObject::connectSlotsByName(SpidrMpx3Eq);
    } // setupUi

    void retranslateUi(QMainWindow *SpidrMpx3Eq)
    {
        SpidrMpx3Eq->setWindowTitle(QApplication::translate("SpidrMpx3Eq", "SpidrMpx3Eq", 0));
        actionConnect->setText(QApplication::translate("SpidrMpx3Eq", "Connect", 0));
        actionExit->setText(QApplication::translate("SpidrMpx3Eq", "Exit", 0));
#ifndef QT_NO_TOOLTIP
        _tabDACs->setToolTip(QApplication::translate("SpidrMpx3Eq", "<html><head/><body><p>Equalization</p></body></html>", 0));
#endif // QT_NO_TOOLTIP
        _connect->setText(QApplication::translate("SpidrMpx3Eq", "Connect", 0));
        _startEq->setText(QApplication::translate("SpidrMpx3Eq", "Start Eq", 0));
        _statusLabel->setText(QApplication::translate("SpidrMpx3Eq", "Standby ...", 0));
        _tabDACs->setTabText(_tabDACs->indexOf(tab), QApplication::translate("SpidrMpx3Eq", "Equalization", 0));
        _tabDACs->setTabText(_tabDACs->indexOf(tab_2), QApplication::translate("SpidrMpx3Eq", "DACs", 0));
        menuMpx3Eq->setTitle(QApplication::translate("SpidrMpx3Eq", "File", 0));
    } // retranslateUi

};

namespace Ui {
    class SpidrMpx3Eq: public Ui_SpidrMpx3Eq {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_SPIDRMPX3EQ_H
