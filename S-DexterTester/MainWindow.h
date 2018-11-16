#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    QString _welcomeText = "WELCOME TO S-DEXTER TESTER PROGRAM";

private:
    void _connectGuiSignalsToSlots(void);

private slots:
    void on_connect_pushButton_released(void);
    void on_disconnect_pushButton_released(void);
    void on_setTh_pushButton_released(void);
    void on_getTh_pushButton_released(void);
};

#endif // MAINWINDOW_H
