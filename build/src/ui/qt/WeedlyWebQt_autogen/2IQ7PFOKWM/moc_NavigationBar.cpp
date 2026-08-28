/****************************************************************************
** Meta object code from reading C++ file 'NavigationBar.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.11.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../../../src/ui/qt/include/qt/NavigationBar.h"
#include <QtGui/qtextcursor.h>
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'NavigationBar.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 69
#error "This file was generated using the moc from 6.11.1. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
QT_WARNING_DISABLE_GCC("-Wuseless-cast")
namespace {
struct qt_meta_tag_ZN13NavigationBarE_t {};
} // unnamed namespace

template <> constexpr inline auto NavigationBar::qt_create_metaobjectdata<qt_meta_tag_ZN13NavigationBarE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "NavigationBar",
        "navigateBack",
        "",
        "navigateForward",
        "refresh",
        "goHome",
        "urlActivated",
        "url",
        "toggleDarkMode",
        "enabled",
        "toggleReaderMode",
        "starClicked"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'navigateBack'
        QtMocHelpers::SignalData<void()>(1, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'navigateForward'
        QtMocHelpers::SignalData<void()>(3, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'refresh'
        QtMocHelpers::SignalData<void()>(4, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'goHome'
        QtMocHelpers::SignalData<void()>(5, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'urlActivated'
        QtMocHelpers::SignalData<void(const QString &)>(6, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 7 },
        }}),
        // Signal 'toggleDarkMode'
        QtMocHelpers::SignalData<void(bool)>(8, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Bool, 9 },
        }}),
        // Signal 'toggleReaderMode'
        QtMocHelpers::SignalData<void(bool)>(10, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Bool, 9 },
        }}),
        // Signal 'starClicked'
        QtMocHelpers::SignalData<void()>(11, 2, QMC::AccessPublic, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<NavigationBar, qt_meta_tag_ZN13NavigationBarE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject NavigationBar::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN13NavigationBarE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN13NavigationBarE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN13NavigationBarE_t>.metaTypes,
    nullptr
} };

void NavigationBar::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<NavigationBar *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->navigateBack(); break;
        case 1: _t->navigateForward(); break;
        case 2: _t->refresh(); break;
        case 3: _t->goHome(); break;
        case 4: _t->urlActivated((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 5: _t->toggleDarkMode((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1]))); break;
        case 6: _t->toggleReaderMode((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1]))); break;
        case 7: _t->starClicked(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (NavigationBar::*)()>(_a, &NavigationBar::navigateBack, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (NavigationBar::*)()>(_a, &NavigationBar::navigateForward, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (NavigationBar::*)()>(_a, &NavigationBar::refresh, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (NavigationBar::*)()>(_a, &NavigationBar::goHome, 3))
            return;
        if (QtMocHelpers::indexOfMethod<void (NavigationBar::*)(const QString & )>(_a, &NavigationBar::urlActivated, 4))
            return;
        if (QtMocHelpers::indexOfMethod<void (NavigationBar::*)(bool )>(_a, &NavigationBar::toggleDarkMode, 5))
            return;
        if (QtMocHelpers::indexOfMethod<void (NavigationBar::*)(bool )>(_a, &NavigationBar::toggleReaderMode, 6))
            return;
        if (QtMocHelpers::indexOfMethod<void (NavigationBar::*)()>(_a, &NavigationBar::starClicked, 7))
            return;
    }
}

const QMetaObject *NavigationBar::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *NavigationBar::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN13NavigationBarE_t>.strings))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int NavigationBar::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 8)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 8;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 8)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 8;
    }
    return _id;
}

// SIGNAL 0
void NavigationBar::navigateBack()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void NavigationBar::navigateForward()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void NavigationBar::refresh()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void NavigationBar::goHome()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void NavigationBar::urlActivated(const QString & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 4, nullptr, _t1);
}

// SIGNAL 5
void NavigationBar::toggleDarkMode(bool _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 5, nullptr, _t1);
}

// SIGNAL 6
void NavigationBar::toggleReaderMode(bool _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 6, nullptr, _t1);
}

// SIGNAL 7
void NavigationBar::starClicked()
{
    QMetaObject::activate(this, &staticMetaObject, 7, nullptr);
}
QT_WARNING_POP
