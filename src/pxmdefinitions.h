#ifndef PXMDEFINITIONS_H
#define PXMDEFINITIONS_H

#include <event2/util.h>
#include <event2/bufferevent.h>

#include <QUuid>
#include <QString>
#include <QSize>

#define BACKLOG 20
#define MULTICAST_ADDRESS "239.192.15.15"
#define PACKED_UUID_BYTE_LENGTH 16
#define MESSAGE_HISTORY_LENGTH 1000
#define MIDNIGHT_TIMER_INTERVAL 60000

struct peerDetails{
    bool isConnected = false;
    bool isAuthenticated = false;
    evutil_socket_t socketDescriptor = 0;
    sockaddr_in ipAddressRaw;
    QString hostname = "";
    QVector<QString*> messages;
    QUuid identifier;
    bufferevent *bev = nullptr;
};
struct initialSettings{
    int uuidNum = 0;
    unsigned short tcpPort = 0;
    unsigned short udpPort = 0;
    bool mute = false;
    bool preventFocus = false;
    QString username = "";
    QSize windowSize = QSize(700, 500);
    QUuid uuid = "";
};
#endif
