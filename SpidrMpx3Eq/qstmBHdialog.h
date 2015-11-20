#ifndef QSTMBHDIALOG_H
#define QSTMBHDIALOG_H

#include <QDialog>
#include <QTextItem>


namespace Ui {
class qstmBHdialog;
}

class qstmBHdialog : public QDialog
{
    Q_OBJECT

public:
    explicit qstmBHdialog(QWidget *parent = 0);
    ~qstmBHdialog();

private slots:
    void on_pushButton_clicked();

    void on_pushButton_2_clicked();

private:
    Ui::qstmBHdialog *ui;
	QTextItem thicknessBox;
};

#endif // QSTMBHDIALOG_H
