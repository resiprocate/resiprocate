/****************************************************************************
** Meta object code from reading C++ file 'Connection.hxx'
**
** Created by: The Qt Meta Object Compiler version 63 (Qt 4.8.6)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "Connection.hxx"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'Connection.hxx' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.6. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_tr__Connection[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       5,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: signature, parameters, type, tag, flags
      22,   16,   15,   15, 0x08,
      48,   15,   15,   15, 0x08,
      62,   15,   15,   15, 0x08,
      94,   77,   15,   15, 0x08,
     137,  119,   15,   15, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_tr__Connection[] = {
    "tr::Connection\0\0error\0doConnect(Tp::DBusError*)\0"
    "onConnected()\0doDisconnect()\0"
    "newStatus,reason\0setStatusSlot(uint,uint)\0"
    "caller,callHandle\0onIncomingCall(QString,uint)\0"
};

void tr::Connection::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        Connection *_t = static_cast<Connection *>(_o);
        switch (_id) {
        case 0: _t->doConnect((*reinterpret_cast< Tp::DBusError*(*)>(_a[1]))); break;
        case 1: _t->onConnected(); break;
        case 2: _t->doDisconnect(); break;
        case 3: _t->setStatusSlot((*reinterpret_cast< uint(*)>(_a[1])),(*reinterpret_cast< uint(*)>(_a[2]))); break;
        case 4: _t->onIncomingCall((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< uint(*)>(_a[2]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData tr::Connection::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject tr::Connection::staticMetaObject = {
    { &Tp::BaseConnection::staticMetaObject, qt_meta_stringdata_tr__Connection,
      qt_meta_data_tr__Connection, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &tr::Connection::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *tr::Connection::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *tr::Connection::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_tr__Connection))
        return static_cast<void*>(const_cast< Connection*>(this));
    typedef Tp::BaseConnection QMocSuperClass;
    return QMocSuperClass::qt_metacast(_clname);
}

int tr::Connection::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    typedef Tp::BaseConnection QMocSuperClass;
    _id = QMocSuperClass::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 5)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 5;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
