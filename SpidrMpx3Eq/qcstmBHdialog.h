#ifndef QSTMBHDIALOG_H
#define QSTMBHDIALOG_H

#include <QDialog>
#include <QTextItem>


namespace Ui {
class qcstmBHdialog;
}

class qcstmBHdialog : public QDialog
{
    Q_OBJECT

public:
	explicit qcstmBHdialog(QWidget *parent = 0);
    ~qcstmBHdialog();

private slots:
    void on_pushButton_clicked();

    void on_pushButton_2_clicked();

private:
    Ui::qcstmBHdialog *ui;
	QString * dialogResult;

signals:
	void talkToForm(double signalThickness);

};

#endif // QSTMBHDIALOG_H
