TEMPLATE = app
TARGET = PXMessenger

QT = core gui widgets multimedia

win32 {
LIBS = -lws2_32
}

CONFIG += c++11 \
	release

SOURCES += \
    main.cpp \
    window.cpp \
    mess_client.cpp \
    mess_serv.cpp \
    mess_discover.cpp \
    mess_textedit.cpp

HEADERS += \
    window.h \
    mess_client.h \
    mess_serv.h \
    mess_discover.h \
    peerlist.h \
    mess_textedit.h

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
