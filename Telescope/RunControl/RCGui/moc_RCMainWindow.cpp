/****************************************************************************
** Meta object code from reading C++ file 'RCMainWindow.h'
**
** Created: Sat Oct 11 20:57:19 2014
**      by: The Qt Meta Object Compiler version 62 (Qt 4.6.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "RCMainWindow.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'RCMainWindow.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 62
#error "This file was generated using the moc from 4.6.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_RCMainWindow[] = {

 // content:
       4,       // revision
       0,       // classname
       0,    0, // classinfo
      16,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: signature, parameters, type, tag, flags
      14,   13,   13,   13, 0x0a,
      34,   13,   13,   13, 0x0a,
      58,   13,   13,   13, 0x0a,
      80,   13,   13,   13, 0x0a,
     105,   13,   13,   13, 0x0a,
     127,   13,   13,   13, 0x0a,
     147,   13,   13,   13, 0x0a,
     169,   13,   13,   13, 0x0a,
     189,   13,   13,   13, 0x0a,
     220,   13,   13,   13, 0x0a,
     249,   13,   13,   13, 0x0a,
     262,   13,   13,   13, 0x0a,
     275,   13,   13,   13, 0x0a,
     295,   13,   13,   13, 0x0a,
     327,   13,   13,   13, 0x0a,
     346,   13,   13,   13, 0x0a,

       0        // eod
};

static const char qt_meta_stringdata_RCMainWindow[] = {
    "RCMainWindow\0\0on_b_init_clicked()\0"
    "on_b_startRun_clicked()\0on_b_endRun_clicked()\0"
    "on_b_configure_clicked()\0b_tempStart_clicked()\0"
    "b_tempEnd_clicked()\0b_fileStart_clicked()\0"
    "b_fileEnd_clicked()\0on_b_startMonitoring_clicked()\0"
    "on_b_endMonitoring_clicked()\0tempUpdate()\0"
    "fileUpdate()\0on_b_quit_clicked()\0"
    "on_b_forceSaveRecords_clicked()\0"
    "on_b_DQM_clicked()\0on_b_FQM_clicked()\0"
};

const QMetaObject RCMainWindow::staticMetaObject = {
    { &QMainWindow::staticMetaObject, qt_meta_stringdata_RCMainWindow,
      qt_meta_data_RCMainWindow, 0 }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &RCMainWindow::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *RCMainWindow::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *RCMainWindow::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_RCMainWindow))
        return static_cast<void*>(const_cast< RCMainWindow*>(this));
    return QMainWindow::qt_metacast(_clname);
}

int RCMainWindow::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QMainWindow::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: on_b_init_clicked(); break;
        case 1: on_b_startRun_clicked(); break;
        case 2: on_b_endRun_clicked(); break;
        case 3: on_b_configure_clicked(); break;
        case 4: b_tempStart_clicked(); break;
        case 5: b_tempEnd_clicked(); break;
        case 6: b_fileStart_clicked(); break;
        case 7: b_fileEnd_clicked(); break;
        case 8: on_b_startMonitoring_clicked(); break;
        case 9: on_b_endMonitoring_clicked(); break;
        case 10: tempUpdate(); break;
        case 11: fileUpdate(); break;
        case 12: on_b_quit_clicked(); break;
        case 13: on_b_forceSaveRecords_clicked(); break;
        case 14: on_b_DQM_clicked(); break;
        case 15: on_b_FQM_clicked(); break;
        default: ;
        }
        _id -= 16;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
