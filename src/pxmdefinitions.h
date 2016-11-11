#ifndef PXMDEFINITIONS_H
#define PXMDEFINITIONS_H

#include <event2/util.h>
#include <event2/bufferevent.h>

#include <QUuid>
#include <QString>

#define BACKLOG 20
#define TCP_BUFFER_WATERMARK 10000
#define MULTICAST_ADDRESS "239.192.13.13"

struct peerDetails{
    bool isValid = false;
    bool isConnected = false;
    bool attemptingToConnect = false;
    bool messagePending = false;
    evutil_socket_t socketDescriptor = 0;
    QString portNumber = "-1";
    int listWidgetIndex = -1;
    QString ipAddress = "";
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
