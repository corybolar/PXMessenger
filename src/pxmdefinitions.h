#ifndef PXMDEFINITIONS_H
#define PXMDEFINITIONS_H

#include <event2/util.h>
#include <event2/bufferevent.h>

#include <QUuid>
#include <QString>
#include <QSize>
#include <QLinkedList>
#include <QtAlgorithms>

#ifdef _WIN32
Q_DECLARE_METATYPE(intptr_t);
#endif
Q_DECLARE_METATYPE(sockaddr_in);
Q_DECLARE_METATYPE(size_t);
Q_DECLARE_OPAQUE_POINTER(bufferevent*);
Q_DECLARE_METATYPE(bufferevent*);

#define BACKLOG 20
#define MULTICAST_ADDRESS "239.192.13.13"
#define PACKED_UUID_BYTE_LENGTH 16
#define MESSAGE_HISTORY_LENGTH 500
#define MIDNIGHT_TIMER_INTERVAL_MINUTES 1

struct peerDetails{
    bool isConnected;
    bool isAuthenticated;
    evutil_socket_t socketDescriptor;
    sockaddr_in ipAddressRaw;
    QString hostname;
    QLinkedList<QString*> messages;
    QUuid identifier;
    bufferevent *bev;
    peerDetails()
    {
        isConnected = false;
        isAuthenticated = false;
        socketDescriptor = -1;
        memset(&ipAddressRaw, 0, sizeof(sockaddr_in));
        messages = QLinkedList<QString*>();
        hostname = QString();
        identifier = QUuid();
        bev = nullptr;
    }
};
struct initialSettings{
    int uuidNum;
    unsigned short tcpPort;
    unsigned short udpPort;
    bool mute;
    bool preventFocus;
    QString username;
    QSize windowSize;
    QUuid uuid;
    initialSettings()
    {
        uuidNum = 0;
        tcpPort = 0;
        udpPort = 0;
        mute = false;
        preventFocus = false;
        username = QString();
        windowSize = QSize(700,500);
        uuid = QUuid();
    }
};
#endif
