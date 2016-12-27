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
#include <QSharedPointer>
#include <QDataStream>

#include "uuidcompression.h"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
Q_DECLARE_METATYPE(intptr_t)
#else
#include <arpa/inet.h>
#endif

Q_DECLARE_METATYPE(sockaddr_in)
Q_DECLARE_METATYPE(size_t)
Q_DECLARE_OPAQUE_POINTER(bufferevent*)
Q_DECLARE_METATYPE(bufferevent*)

namespace PXMConsts {
const int BACKLOG = 200;
const char* const DEFAULT_MULTICAST_ADDRESS = "239.192.13.13";
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
const int MAX_COMPUTER_NAME = 36;
const char* const PORT_SEPERATOR = ":::::";
enum MESSAGE_TYPE : const uint32_t {MSG_TEXT = 			0x11111111,
                                    MSG_GLOBAL =		0x22222222,
                                    MSG_SYNC = 			0x33333333,
                                    MSG_SYNC_REQUEST = 	0x44444444,
                                    MSG_AUTH = 			0x55555555,
                                    MSG_NAME = 			0x66666666,
                                    MSG_DISOVER = 		0x77777777,
                                    MSG_ID = 			0x88888888};

const int MAX_UUID_PACKET_LENGTH = 	sizeof(MESSAGE_TYPE) +
                                    UUIDCompression::PACKED_UUID_LENGTH +
                                    MAX_HOSTNAME_LENGTH +
                                    MAX_COMPUTER_NAME +
                                    strlen(PORT_SEPERATOR) +
                                    5/*port number*/ +
                                    1/*null*/;
}
Q_DECLARE_METATYPE(PXMConsts::MESSAGE_TYPE)

namespace Peers{
class BevWrapper {
public:
    BevWrapper(bufferevent *buf) : bev(buf), locker(new QMutex) {}
    BevWrapper() : bev(nullptr), locker(new QMutex) {}
    ~BevWrapper();
    BevWrapper(const BevWrapper& b) : bev(b.bev), locker(b.locker) {}
    BevWrapper(BevWrapper&& b) noexcept : bev(b.bev), locker(b.locker)
    {
        b.bev = nullptr;
        b.locker = nullptr;
    }
    BevWrapper &operator =(BevWrapper&& b) noexcept;
    bool operator ==(const BevWrapper& b) {return bev == b.bev;}
    bool operator !=(const BevWrapper& b) {return !(bev == b.bev);}

    void setBev(bufferevent *buf) {bev = buf;}
    bufferevent* getBev() {return bev;}
    void lockBev() {locker->lock();}
    void unlockBev() {locker->unlock();}
    int freeBev();

private:
    bufferevent *bev;
    QMutex *locker;
};

struct PeerData{
    QUuid identifier;
    sockaddr_in ipAddressRaw;
    QString hostname;
    QLinkedList<QString*> messages;
    QSharedPointer<BevWrapper> bw;
    evutil_socket_t socket;
    bool connectTo;
    bool isAuthenticated;

    //Default Constructor
    PeerData() : identifier(QUuid()), ipAddressRaw(sockaddr_in()),
            hostname(QString()), messages(QLinkedList<QString*>()),
            bw(QSharedPointer<BevWrapper>(new BevWrapper)), socket(-1), connectTo(false),
            isAuthenticated(false) {}

    //Copy
    PeerData (const PeerData& pd) : identifier(pd.identifier),
            ipAddressRaw(pd.ipAddressRaw), hostname(pd.hostname),
            messages(pd.messages), bw(pd.bw), socket(pd.socket),
            connectTo(pd.connectTo), isAuthenticated(pd.isAuthenticated) {}

    //Move
    PeerData (PeerData&& pd) noexcept : identifier(pd.identifier),
            ipAddressRaw(pd.ipAddressRaw), hostname(pd.hostname),
            messages(pd.messages), bw(pd.bw), socket(pd.socket),
            connectTo(pd.connectTo), isAuthenticated(pd.isAuthenticated)
    {
        pd.bw.clear();
    }

    //Destructor
    ~PeerData() noexcept {}

    //Move assignment
    PeerData& operator= (PeerData&& pd) noexcept;

    //Copy assignment
    PeerData& operator= (const PeerData& pd);

    //Return data of this struct as a string padded with the value in 'pad'
    QString toInfoString();
};

}
Q_DECLARE_METATYPE(QSharedPointer<Peers::BevWrapper>)

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
        windowSize = QSize(800,600);
        uuid = QUuid();
        multicast = QString(PXMConsts::DEFAULT_MULTICAST_ADDRESS);
    }
};

#endif
