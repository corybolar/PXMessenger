TEMPLATE = app
TARGET = PXMessenger
VERSION = 1.6.0
QMAKE_TARGET_COMPANY = Bolar Code Solutions
QMAKE_TARGET_PRODUCT = PXMessenger
QMAKE_TARGET_DESCRIPTION = Instant Messenger
QMAKE_TARGET_COPYRIGHT = GPLv3

target.path 	= /usr/local/bin
desktop.path 	= /usr/share/applications
desktop.files 	+= $$PWD/resources/pxmessenger.desktop
icon.path 	= /usr/share/pixmaps
icon.files 	+= $$PWD/resources/PXMessenger.png
INSTALLS += target desktop icon

QT = core gui widgets multimedia
QT -= network
CONFIG += DEBUG \
          RELEASE
win32: CONFIG += windows

unix: LIBS += -levent -levent_pthreads -levent_openssl -lssl -lcrypto

win32 {
LIBS += -L$$PWD/../libevent/build/lib -levent -levent_core
INCLUDEPATH += $$PWD/../libevent/include \
               $$PWD/../libevent/build/include
}

INCLUDEPATH += $$PWD/include

win32 {
LIBS += -lws2_32
RC_ICONS = $$PWD/resources/PXM_Icon.ico
}

win32 {
contains(QMAKE_HOST.arch, x86_64):{
TARGET =$$TARGET-x86_64
BUILDDIR = build-win64
}
contains(QMAKE_HOST.arch, x86):{
TARGET =$$TARGET-x86
BUILDDIR = build-win32
}
}

QMAKE_CXXFLAGS += -Wall \
                -std=c++14

SOURCES += \
    $$PWD/src/client.cpp \
    $$PWD/src/peerworker.cpp \
    $$PWD/src/sync.cpp \
    $$PWD/src/inireader.cpp \
    $$PWD/src/server.cpp \
    $$PWD/src/mainwindow.cpp \
    $$PWD/src/pxmessenger.cpp \
    $$PWD/src/netcompression.cpp \
    $$PWD/src/stackwidget.cpp \
    $$PWD/src/console.cpp \
    $$PWD/src/peers.cpp \
    $$PWD/src/agent.cpp

HEADERS += \
    $$PWD/include/peerworker.h \
    $$PWD/include/mainwindow.h \
    $$PWD/include/sync.h \
    $$PWD/include/inireader.h \
    $$PWD/include/server.h \
    $$PWD/include/client.h \
    $$PWD/include/netcompression.h \
    $$PWD/include/timedvector.h \
    $$PWD/include/stackwidget.h \
    $$PWD/include/console.h \
    $$PWD/include/consts.h \
    $$PWD/include/peers.h \
    $$PWD/include/agent.h

FORMS += \
    $$PWD/ui/mainwindow.ui \
    $$PWD/ui/aboutdialog.ui \
    $$PWD/ui/settingsdialog.ui \
    ui/manualconnect.ui

DISTFILES += \
    resources/updates.json

RESOURCES += 	$$PWD/resources/resources.qrc

win32 {
release:DESTDIR = $$PWD/$$BUILDDIR/
debug:DESTDIR = $$PWD/$$BUILDDIR/
OBJECTS_DIR = $$PWD/$$BUILDDIR/obj
MOC_DIR = $$PWD/$$BUILDDIR/moc
RCC_DIR = $$PWD/$$BUILDDIR/rcc
UI_DIR = $$PWD/$$BUILDDIR/ui
QMAKE_CLEAN += $$PWD/object_script.* \
               $$PWD/PXMessenger_resource.rc
}
unix {
release:DESTDIR = $$PWD/
debug:DESTDIR = $$PWD/
OBJECTS_DIR = $$PWD/build-unix/obj
MOC_DIR = $$PWD/build-unix/moc
RCC_DIR = $$PWD/build-unix/rcc
UI_DIR = $$PWD/build-unix/ui
}

INCLUDEPATH += MOC_DIR

#win32 {
#include($$PWD/include/QSimpleUpdater/QSimpleUpdater.pri)
#}
#QMAKE_CXXFLAGS+="-fsanitize=undefined -fno-omit-frame-pointer"
#QMAKE_CFLAGS+="-fsanitize=address -fno-omit-frame-pointer"
#QMAKE_LFLAGS+="-fsanitize=undefined"
