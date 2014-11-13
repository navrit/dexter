/********************************************************************************
** Form generated from reading UI file 'spidrmpx3eq.ui'
**
** Created by: Qt User Interface Compiler version 4.8.6
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_SPIDRMPX3EQ_H
#define UI_SPIDRMPX3EQ_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QFrame>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QMainWindow>
#include <QtGui/QMenu>
#include <QtGui/QMenuBar>
#include <QtGui/QPushButton>
#include <QtGui/QStatusBar>
#include <QtGui/QToolBar>
#include <QtGui/QVBoxLayout>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class Ui_SpidrMpx3Eq
{
public:
    QAction *actionConnect;
    QAction *actionExit;
    QWidget *centralWidget;
    QWidget *verticalLayoutWidget;
    QVBoxLayout *verticalLayout;
    QPushButton *_connect;
    QPushButton *_startEq;
    QLabel *_statusLabel;
    QFrame *_histoFrame;
    QMenuBar *menuBar;
    QMenu *menuMpx3Eq;
    QToolBar *mainToolBar;
    QStatusBar *statusBar;

    void setupUi(QMainWindow *SpidrMpx3Eq)
    {
        if (SpidrMpx3Eq->objectName().isEmpty())
            SpidrMpx3Eq->setObjectName(QString::fromUtf8("SpidrMpx3Eq"));
        SpidrMpx3Eq->resize(858, 485);
        actionConnect = new QAction(SpidrMpx3Eq);
        actionConnect->setObjectName(QString::fromUtf8("actionConnect"));
        actionExit = new QAction(SpidrMpx3Eq);
        actionExit->setObjectName(QString::fromUtf8("actionExit"));
        centralWidget = new QWidget(SpidrMpx3Eq);
        centralWidget->setObjectName(QString::fromUtf8("centralWidget"));
        verticalLayoutWidget = new QWidget(centralWidget);
        verticalLayoutWidget->setObjectName(QString::fromUtf8("verticalLayoutWidget"));
        verticalLayoutWidget->setGeometry(QRect(600, 330, 241, 85));
        verticalLayout = new QVBoxLayout(verticalLayoutWidget);
        verticalLayout->setSpacing(6);
        verticalLayout->setContentsMargins(11, 11, 11, 11);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        verticalLayout->setContentsMargins(0, 0, 0, 0);
        _connect = new QPushButton(verticalLayoutWidget);
        _connect->setObjectName(QString::fromUtf8("_connect"));

        verticalLayout->addWidget(_connect);

        _startEq = new QPushButton(verticalLayoutWidget);
        _startEq->setObjectName(QString::fromUtf8("_startEq"));

        verticalLayout->addWidget(_startEq);

        _statusLabel = new QLabel(verticalLayoutWidget);
        _statusLabel->setObjectName(QString::fromUtf8("_statusLabel"));

        verticalLayout->addWidget(_statusLabel);

        _histoFrame = new QFrame(centralWidget);
        _histoFrame->setObjectName(QString::fromUtf8("_histoFrame"));
        _histoFrame->setGeometry(QRect(50, 20, 441, 381));
        _histoFrame->setFrameShape(QFrame::StyledPanel);
        _histoFrame->setFrameShadow(QFrame::Raised);
        SpidrMpx3Eq->setCentralWidget(centralWidget);
        menuBar = new QMenuBar(SpidrMpx3Eq);
        menuBar->setObjectName(QString::fromUtf8("menuBar"));
        menuBar->setGeometry(QRect(0, 0, 858, 25));
        menuMpx3Eq = new QMenu(menuBar);
        menuMpx3Eq->setObjectName(QString::fromUtf8("menuMpx3Eq"));
        SpidrMpx3Eq->setMenuBar(menuBar);
        mainToolBar = new QToolBar(SpidrMpx3Eq);
        mainToolBar->setObjectName(QString::fromUtf8("mainToolBar"));
        SpidrMpx3Eq->addToolBar(Qt::TopToolBarArea, mainToolBar);
        statusBar = new QStatusBar(SpidrMpx3Eq);
        statusBar->setObjectName(QString::fromUtf8("statusBar"));
        SpidrMpx3Eq->setStatusBar(statusBar);

        menuBar->addAction(menuMpx3Eq->menuAction());
        menuMpx3Eq->addSeparator();
        menuMpx3Eq->addAction(actionConnect);
        menuMpx3Eq->addAction(actionExit);

        retranslateUi(SpidrMpx3Eq);

        QMetaObject::connectSlotsByName(SpidrMpx3Eq);
    } // setupUi

    void retranslateUi(QMainWindow *SpidrMpx3Eq)
    {
        SpidrMpx3Eq->setWindowTitle(QApplication::translate("SpidrMpx3Eq", "SpidrMpx3Eq", 0, QApplication::UnicodeUTF8));
        actionConnect->setText(QApplication::translate("SpidrMpx3Eq", "Connect", 0, QApplication::UnicodeUTF8));
        actionExit->setText(QApplication::translate("SpidrMpx3Eq", "Exit", 0, QApplication::UnicodeUTF8));
        _connect->setText(QApplication::translate("SpidrMpx3Eq", "Connect", 0, QApplication::UnicodeUTF8));
        _startEq->setText(QApplication::translate("SpidrMpx3Eq", "Start Eq", 0, QApplication::UnicodeUTF8));
        _statusLabel->setText(QApplication::translate("SpidrMpx3Eq", "Standby ...", 0, QApplication::UnicodeUTF8));
        menuMpx3Eq->setTitle(QApplication::translate("SpidrMpx3Eq", "File", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class SpidrMpx3Eq: public Ui_SpidrMpx3Eq {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_SPIDRMPX3EQ_H
