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

    void on_okButton_clicked();

    void on_closeButton_clicked();

private:
    Ui::qcstmBHdialog *ui;
	QString * dialogResult;

signals:
    void talkToForm(double signalThickness, QString material);

};

#endif // QSTMBHDIALOG_H
