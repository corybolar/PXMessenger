TEMPLATE = app
TARGET = PXMessenger
VERSION = 0.6
QMAKE_TARGET_COMPANY = Bolar Code Solutions
QMAKE_TARGET_PRODUCT = PXMessenger
QMAKE_TARGET_DESCRIPTION = Instant Messenger

QT = core gui widgets multimedia

unix: LIBS += -levent

win32: LIBS += -L$$PWD/../../libevent-2.0.22-stable/.libs/ -levent

INCLUDEPATH += $$PWD/../../libevent-2.0.22-stable/include
DEPENDPATH += $$PWD/../../libevent-2.0.22-stable/include

win32 {
LIBS += -lws2_32 \
}

win32 {
CONFIG += c++11 \
        debug \
        release 
}
unix {
CONFIG += c++11 \
		release \
		debug
}
QMAKE_CXXFLAGS += -Wall

SOURCES += \
    main.cpp \
    window.cpp \
    mess_client.cpp \
    mess_serv.cpp \
    mess_textedit.cpp \
    peerlist.cpp \
    messinireader.cpp

HEADERS += \
    window.h \
    mess_client.h \
    mess_serv.h \
    peerlist.h \
    mess_textedit.h \
    messinireader.h \
    settingsDialog.h

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
