TEMPLATE = app
TARGET = formtest

QT = core gui widgets

CONFIG += c++11

SOURCES += \
    main.cpp \
    window.cpp \
    mess_client.cpp \
    mess_serv.cpp \
    mess_recieve.cpp \
    mess_discover.cpp

HEADERS += \
    window.h \
    mess_client.h \
    mess_serv.h \
    mess_recieve.h \
    mess_discover.h \
    peerlist.h

