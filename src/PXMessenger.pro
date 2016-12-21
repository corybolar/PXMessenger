TEMPLATE = app
TARGET = PXMessenger
VERSION = 1.2.3
QMAKE_TARGET_COMPANY = Bolar Code Solutions
QMAKE_TARGET_PRODUCT = PXMessenger
QMAKE_TARGET_DESCRIPTION = Instant Messenger

QT = core gui widgets multimedia
CONFIG += c++11

unix: LIBS += -levent -levent_pthreads

win32 { LIBS += -L$$PWD/../../libevent-2.0.22-stable/.libs/ -levent

INCLUDEPATH += $$PWD/../../libevent-2.0.22-stable/include
DEPENDPATH += $$PWD/../../libevent-2.0.22-stable/include
}

win32 {
LIBS += -lws2_32 \
}

target.path = /usr/bin

#INSTALLS += target

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
    pxmdebugwindow.cpp \
    pxmsettingsdialog.cpp \
    pxmtextbrowser.cpp \
    uuidcompression.cpp \
    pxmdefinitions.cpp

HEADERS += \
    pxmdefinitions.h \
    pxmpeerworker.h \
    pxmsettingsdialog.h \
    pxmmainwindow.h \
    pxmsync.h \
    pxminireader.h \
    pxmtextedit.h \
    pxmserver.h \
    pxmclient.h \
    pxmdebugwindow.h \
    pxmtextbrowser.h \
    uuidcompression.h \
    timedvector.h

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
