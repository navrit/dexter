#ifndef THRESHOLDSCANDELEGATE_H
#define THRESHOLDSCANDELEGATE_H

#include <QObject>
#include <QModelIndex>
#include <QObject>
#include <QSize>
#include <QSpinBox>
#include <QStyleOption>
#include <QItemDelegate>
#include <QCheckBox>
#include <QLabel>
#include <QDebug>


class ThresholdScanDelegate : public QItemDelegate
{
    Q_OBJECT
public:
    explicit ThresholdScanDelegate(QObject *parent = nullptr);
    virtual ~ThresholdScanDelegate(){ }

    // Create Editor when we construct MyDelegate
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const;

    // Then, we set the Editor
    void setEditorData(QWidget *editor, const QModelIndex &index) const;

    // When we modify data, this model reflect the change
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const;

    // Give the SpinBox the info on size and location
    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const;

signals:

public slots:
};

#endif // THRESHOLDSCANDELEGATE_H
