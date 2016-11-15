#ifndef PXMDEFINITIONS_H
#define PXMDEFINITIONS_H

#include <event2/util.h>
#include <event2/bufferevent.h>

#include <QUuid>
#include <QString>
#include <QSize>

#define BACKLOG 20
#define TCP_BUFFER_WATERMARK 10000
#define MULTICAST_ADDRESS "239.192.14.14"

struct peerDetails{
    bool isConnected = false;
    bool attemptingToConnect = false;
    evutil_socket_t socketDescriptor = 0;
    int listWidgetIndex = -1;
    sockaddr_in ipAddressRaw;
    QString hostname = "";
    QString textBox = "";
    QUuid identifier;
    bufferevent *bev = nullptr;
};
struct initialSettings{
    int uuidNum = 0;
    unsigned short tcpPort = 0;
    unsigned short udpPort = 0;
    bool mute = false;
    bool preventFocus = false;
    QString username;
    QSize windowSize;
    QUuid uuid;
};
#endif
