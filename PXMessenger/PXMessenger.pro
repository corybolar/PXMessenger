TEMPLATE = app
TARGET = PXMessenger

QT = core gui widgets

CONFIG += c++11

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

