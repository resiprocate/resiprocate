/****************************************************************************
** Meta object code from reading C++ file 'SipCallChannel.hxx'
**
** Created by: The Qt Meta Object Compiler version 63 (Qt 4.8.6)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "SipCallChannel.hxx"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'SipCallChannel.hxx' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.6. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_tr__SipCallChannel[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       4,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: signature, parameters, type, tag, flags
      26,   20,   19,   19, 0x08,
      48,   19,   19,   19, 0x08,
      63,   55,   19,   19, 0x08,
      86,   55,   19,   19, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_tr__SipCallChannel[] = {
    "tr::SipCallChannel\0\0state\0"
    "setCallState(QString)\0init()\0success\0"
    "onAnswerComplete(bool)\0onHangupComplete(bool)\0"
};

void tr::SipCallChannel::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        SipCallChannel *_t = static_cast<SipCallChannel *>(_o);
        switch (_id) {
        case 0: _t->setCallState((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 1: _t->init(); break;
        case 2: _t->onAnswerComplete((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 3: _t->onHangupComplete((*reinterpret_cast< bool(*)>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData tr::SipCallChannel::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject tr::SipCallChannel::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_tr__SipCallChannel,
      qt_meta_data_tr__SipCallChannel, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &tr::SipCallChannel::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *tr::SipCallChannel::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *tr::SipCallChannel::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_tr__SipCallChannel))
        return static_cast<void*>(const_cast< SipCallChannel*>(this));
    return QObject::qt_metacast(_clname);
}

int tr::SipCallChannel::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 4)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 4;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
