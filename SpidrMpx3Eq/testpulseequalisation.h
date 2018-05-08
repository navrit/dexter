#ifndef TESTPULSEEQUALISATION_H
#define TESTPULSEEQUALISATION_H

#include <QDialog>

namespace Ui {
class testPulseEqualisation;
}

class testPulseEqualisation : public QDialog
{
    Q_OBJECT

public:
    explicit testPulseEqualisation(QWidget *parent = 0);
    ~testPulseEqualisation();

private:
    Ui::testPulseEqualisation *ui;
};

#endif // TESTPULSEEQUALISATION_H
