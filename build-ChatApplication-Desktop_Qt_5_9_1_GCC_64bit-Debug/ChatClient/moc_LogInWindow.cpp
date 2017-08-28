/****************************************************************************
** Meta object code from reading C++ file 'LogInWindow.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.9.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../ChatApplication/ChatClient/LogInWindow.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'LogInWindow.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.9.1. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_LogInWindow_t {
    QByteArrayData data[6];
    char stringdata0[96];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_LogInWindow_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_LogInWindow_t qt_meta_stringdata_LogInWindow = {
    {
QT_MOC_LITERAL(0, 0, 11), // "LogInWindow"
QT_MOC_LITERAL(1, 12, 18), // "logInButtonPressed"
QT_MOC_LITERAL(2, 31, 0), // ""
QT_MOC_LITERAL(3, 32, 17), // "quitButtonPressed"
QT_MOC_LITERAL(4, 50, 26), // "on_pushButtonLogIn_clicked"
QT_MOC_LITERAL(5, 77, 18) // "onMainWindowLogOff"

    },
    "LogInWindow\0logInButtonPressed\0\0"
    "quitButtonPressed\0on_pushButtonLogIn_clicked\0"
    "onMainWindowLogOff"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_LogInWindow[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       4,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       2,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    0,   34,    2, 0x06 /* Public */,
       3,    0,   35,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       4,    0,   36,    2, 0x08 /* Private */,
       5,    0,   37,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void LogInWindow::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        LogInWindow *_t = static_cast<LogInWindow *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->logInButtonPressed(); break;
        case 1: _t->quitButtonPressed(); break;
        case 2: _t->on_pushButtonLogIn_clicked(); break;
        case 3: _t->onMainWindowLogOff(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        void **func = reinterpret_cast<void **>(_a[1]);
        {
            typedef void (LogInWindow::*_t)();
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&LogInWindow::logInButtonPressed)) {
                *result = 0;
                return;
            }
        }
        {
            typedef void (LogInWindow::*_t)();
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&LogInWindow::quitButtonPressed)) {
                *result = 1;
                return;
            }
        }
    }
    Q_UNUSED(_a);
}

const QMetaObject LogInWindow::staticMetaObject = {
    { &QMainWindow::staticMetaObject, qt_meta_stringdata_LogInWindow.data,
      qt_meta_data_LogInWindow,  qt_static_metacall, nullptr, nullptr}
};


const QMetaObject *LogInWindow::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *LogInWindow::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_LogInWindow.stringdata0))
        return static_cast<void*>(const_cast< LogInWindow*>(this));
    return QMainWindow::qt_metacast(_clname);
}

int LogInWindow::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QMainWindow::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 4)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 4;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 4)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 4;
    }
    return _id;
}

// SIGNAL 0
void LogInWindow::logInButtonPressed()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void LogInWindow::quitButtonPressed()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
