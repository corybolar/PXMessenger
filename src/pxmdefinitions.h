#ifndef PXMDEFINITIONS_H
#define PXMDEFINITIONS_H

#include <event2/util.h>
#include <event2/bufferevent.h>

#include <QUuid>
#include <QString>
#include <QStringBuilder>
#include <QSize>
#include <QLinkedList>
#include <QtAlgorithms>
#include <QMutex>
#include <QDebug>
#include <arpa/inet.h>

#ifdef _WIN32
#include <winsock2.h>
Q_DECLARE_METATYPE(intptr_t)
#endif
Q_DECLARE_METATYPE(sockaddr_in)
Q_DECLARE_METATYPE(size_t)
Q_DECLARE_OPAQUE_POINTER(bufferevent*)
Q_DECLARE_METATYPE(bufferevent*)

namespace PXMConsts {
const int BACKLOG = 200;
const char * const DEFAULT_MULTICAST_ADDRESS = "239.192.13.13";
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
const int SYNC_TIMEOUT_MSECS = 2000;
const int SYNC_TIMER = 900000;
enum MESSAGE_TYPE : const uint8_t {MSG_UUID = 1, MSG_TEXT, MSG_SYNC,
                                   MSG_SYNC_REQUEST, MSG_GLOBAL};
}
Q_DECLARE_METATYPE(PXMConsts::MESSAGE_TYPE)

class BevWrapper {
public:
    BevWrapper(bufferevent *buf) : bev(buf), locker(new QMutex) {}
    BevWrapper() : bev(nullptr), locker(new QMutex) {}
    ~BevWrapper() {delete locker;}
    BevWrapper(const BevWrapper& b) : bev(b.bev) {}

    void setBev(bufferevent *buf) {bev = buf;}
    bufferevent* getBev() {return bev;}
    void lockBev() {locker->lock();}
    void unlockBev() {locker->unlock();}

private:
    bufferevent *bev;
    QMutex *locker;
};

struct peerDetails{
    QUuid identifier;
    sockaddr_in ipAddressRaw;
    QString hostname;
    QLinkedList<QString*> messages;
    BevWrapper *bw;
    evutil_socket_t socket;
    bool connectTo;
    bool isAuthenticated;

    //Default Constructor
    peerDetails() : identifier(QUuid()), ipAddressRaw(sockaddr_in()),
            hostname(QString()), messages(QLinkedList<QString*>()),
            bw(nullptr), socket(-1), connectTo(false),
            isAuthenticated(false) {}

    //Copy
    peerDetails (const peerDetails& p) : identifier(p.identifier),
            ipAddressRaw(p.ipAddressRaw), hostname(p.hostname),
            messages(p.messages), bw(p.bw), socket(p.socket),
            connectTo(p.connectTo), isAuthenticated(p.isAuthenticated) {}

    //Move
    peerDetails (peerDetails&& p) noexcept : identifier(p.identifier),
            ipAddressRaw(p.ipAddressRaw), hostname(p.hostname),
            messages(p.messages), bw(p.bw), socket(p.socket),
            connectTo(p.connectTo), isAuthenticated(p.isAuthenticated)
    {
        p.bw = nullptr;
    }

    //Destructor
    ~peerDetails() noexcept {}

    //Move assignment
    peerDetails& operator= (peerDetails&& p) noexcept;

    //Copy assignment
    peerDetails& operator= (const peerDetails& p);

    //Return data of this struct as a string padded with the value in 'pad'
    QString toString(QString pad);
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
        multicast = QString(PXMConsts::DEFAULT_MULTICAST_ADDRESS);
    }
};

#endif
