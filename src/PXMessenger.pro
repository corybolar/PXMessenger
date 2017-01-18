TEMPLATE = app
TARGET = PXMessenger
VERSION = 1.4.0
QMAKE_TARGET_COMPANY = Bolar Code Solutions
QMAKE_TARGET_PRODUCT = PXMessenger
QMAKE_TARGET_DESCRIPTION = Instant Messenger

QT = core gui widgets multimedia webkitwidgets
CONFIG += c++11

unix: LIBS += -levent -levent_pthreads

win32 { LIBS += -L$$PWD/../../libevent/build/lib -levent -levent_core

INCLUDEPATH += $$PWD/../../libevent/include \
                $$PWD/../../libevent/build/include

DEPENDPATH += $$PWD/../../libevent/include
}

win32 {
LIBS += -lws2_32
#RC_FILE = $$PWD/resources/icon.rc
RC_ICONS = $$PWD/resources/PXM_Icon.ico
}

QMAKE_CXXFLAGS += -Wall

SOURCES += \
    pxmclient.cpp \
    pxmpeerworker.cpp \
    pxmsync.cpp \
    pxminireader.cpp \
    pxmtextedit.cpp \
    pxmserver.cpp \
    pxmmainwindow.cpp \
    pxmessenger.cpp \
    pxmsettingsdialog.cpp \
    uuidcompression.cpp \
    pxmmessageviewer.cpp \
    pxmconsole.cpp \
    pxmpeers.cpp

HEADERS += \
    pxmpeerworker.h \
    pxmmainwindow.h \
    pxmsync.h \
    pxminireader.h \
    pxmtextedit.h \
    pxmserver.h \
    pxmclient.h \
    uuidcompression.h \
    timedvector.h \
    pxmmessageviewer.h \
    pxmconsole.h \
    pxmconsts.h \
    pxmpeers.h

DISTFILES +=

RESOURCES += \
    resources.qrc
win32 {
Release:DESTDIR = ../
Debug:DESTDIR = ../
OBJECTS_DIR = ../build-win32/obj
MOC_DIR = ../build-win32/moc
RCC_DIR = ../build-win32/rcc
UI_DIR = ../build-win32/ui
}
unix {
release:DESTDIR = ../
debug:DESTDIR = ../
OBJECTS_DIR = ../build-unix/obj
MOC_DIR = ../build-unix/moc
RCC_DIR = ../build-unix/rcc
UI_DIR = ../build-unix/ui
}

FORMS += \
    pxmmainwindow.ui \
    pxmaboutdialog.ui \
    pxmsettingsdialog.ui
