#ifndef PXMDEFINITIONS_H
#define PXMDEFINITIONS_H

#include <event2/util.h>
#include <event2/bufferevent.h>

#include <QUuid>
#include <QString>
#include <QSize>
#include <QLinkedList>
#include <QtAlgorithms>
#include <QMutex>

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
#define DEBUG_PADDING 25
#define DEFAULT_UDP_PORT 53273
#define TEXT_EDIT_MAX_LENGTH 2000
#define MAX_HOSTNAME_LENGTH 24

class BevWrapper {
public:
    BevWrapper(bufferevent *buf) : bev(buf) {}
    BevWrapper() : bev(nullptr) {}
    void setBev(bufferevent *buf) {bev = buf;}
    bufferevent* getBev() {return bev;}
    void lockBev() {locker.lock();}
    void unlockBev() {locker.unlock();}
    bufferevent *bev;

private:
    QMutex locker;
};

struct peerDetails{
    bool isConnected;
    bool isAuthenticated;
    evutil_socket_t socketDescriptor;
    sockaddr_in ipAddressRaw;
    QString hostname;
    QLinkedList<QString*> messages;
    QUuid identifier;
    BevWrapper *bw;
    peerDetails()
    {
        isConnected = false;
        isAuthenticated = false;
        socketDescriptor = -1;
        memset(&ipAddressRaw, 0, sizeof(sockaddr_in));
        messages = QLinkedList<QString*>();
        hostname = QString();
        identifier = QUuid();
        bw = new BevWrapper();
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
    QString multicast;
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
        multicast = QString();
    }
};

#endif
