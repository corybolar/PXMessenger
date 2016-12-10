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

namespace PXMConsts {
const int BACKLOG = 200;
const char* const DEFAULT_MULTICAST_ADDRESS = "239.192.13.13";
const size_t PACKED_UUID_BYTE_LENGTH = 16;
const int MESSAGE_HISTORY_LENGTH = 500;
const int MIDNIGHT_TIMER_INTERVAL_MINUTES = 1;
#ifdef QT_DEBUG
const int DEBUG_PADDING = 25;
#else
const int DEBUG_PADDING = 0;
#endif
const unsigned short DEFAULT_UDP_PORT = 53273;
const int TEXT_EDIT_MAX_LENGTH = 2000;
const int MAX_HOSTNAME_LENGTH = 24;
enum MESSAGE_TYPE : uint8_t {UUID = 1, MSG, SYNC, SYNC_REQUEST, GLOBAL};
}

class BevWrapper {
public:
    BevWrapper(bufferevent *buf) : bev(buf) {}
    BevWrapper() : bev(nullptr) {}
    inline void setBev(bufferevent *buf) {bev = buf;}
    inline bufferevent* getBev() {return bev;}
    inline void lockBev() {locker.lock();}
    inline void unlockBev() {locker.unlock();}
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
    peerDetails(bool iC, bool iA, evutil_socket_t sD,
                sockaddr_in iAR, QString h, QLinkedList<QString*> m,
                QUuid iD, BevWrapper *bw)
    {
        isConnected = iC;
        isAuthenticated = iA;
        socketDescriptor = sD;
        ipAddressRaw = iAR;
        hostname = h;
        messages = m;
        identifier = iD;
        this->bw = bw;
    }
    peerDetails(const peerDetails& p) : isConnected(p.isConnected),
      isAuthenticated(p.isAuthenticated),
      socketDescriptor(p.socketDescriptor),
      ipAddressRaw(p.ipAddressRaw),
      hostname(p.hostname),
      messages(p.messages),
      identifier(p.identifier),
      bw(p.bw) {}
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
