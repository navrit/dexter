#include "thresholdscandelegate.h"


ThresholdScanDelegate::ThresholdScanDelegate( QObject *parent ) : QItemDelegate(parent)
{

}

// TableView need to create an Editor
// Create Editor when we construct ThresholdScanDelegate and return the Editor
QWidget *ThresholdScanDelegate::createEditor(QWidget *parent,
                                             const QStyleOptionViewItem &option,
                                             const QModelIndex &index) const
{
    if (index.column() == 0) {
        QSpinBox *editor = new QSpinBox(parent);
        editor->setMinimum(0);
        editor->setMaximum(511);
        return editor;

    } else if (index.column() == 1) {
        if (index.row() >= 0 && index.row() <= 7) {
            QCheckBox *editor = new QCheckBox(parent);
            return editor;
        } else {
            QLabel *editor = new QLabel(parent);
            return editor;
        }

    } else if (index.column() == 2) {
        QLabel *editor = new QLabel(parent);
        return editor;
    }
}

// Then, we set the Editor
// Gets the data from Model and feeds the data to Editor
void ThresholdScanDelegate::setEditorData(QWidget *editor,
                                          const QModelIndex &index) const
{


    if (index.column() == 0) {
        // Get the value via index of the Model
        int value = index.model()->data(index, Qt::EditRole).toInt();
        // Put the value into the SpinBox
        QSpinBox *spinbox = static_cast<QSpinBox*>(editor);
        spinbox->setValue(value);

    } else if (index.column() == 1) {
        if (index.row() >= 0 && index.row() <= 7) {
            // Get the value via index of the Model
            bool value = index.model()->data(index, Qt::EditRole).toBool();
            // Put the value into the SpinBox
            QCheckBox *checkbox = static_cast<QCheckBox*>(editor);
            checkbox->setChecked(value);
        } else {
            return;
        }

    } else if (index.column() == 2) {
        return;
    }
}

// When we modify data, this model reflect the changer
void ThresholdScanDelegate::setModelData(QWidget *editor,
                                         QAbstractItemModel *model,
                                         const QModelIndex &index) const
{
    if (index.column() == 0) {
        QSpinBox *spinbox = static_cast<QSpinBox*>(editor);
        spinbox->interpretText();
        int value = spinbox->value();
        model->setData(index, value, Qt::EditRole);

    } else if (index.column() == 1) {
        if (index.row() >= 0 && index.row() <= 7) {
            QCheckBox *checkbox = static_cast<QCheckBox*>(editor);
            bool value = checkbox->isChecked();
            model->setData(index, value, Qt::EditRole);
        } else {
            model->setData(index, "N/A", Qt::EditRole);
        }

    } else if (index.column() == 2) {
        return;
    }
}

// Give the SpinBox the info on size and location
void ThresholdScanDelegate::updateEditorGeometry(QWidget *editor,
                                      const QStyleOptionViewItem &option,
                                      const QModelIndex &index) const
{
    editor->setGeometry(option.rect);
}
