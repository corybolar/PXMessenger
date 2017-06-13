#include <pxmclient.h>

#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#elif __unix__
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#else
#error "include headers for socket implementation"
#endif

#include <QDebug>

#include <event2/bufferevent.h>
#include <event2/event.h>

#include "pxmpeers.h"
#include "netcompression.h"

static_assert(sizeof(uint8_t) == 1, "uint8_t not defined as 1 byte");
static_assert(sizeof(uint16_t) == 2, "uint16_t not defined as 2 bytes");
static_assert(sizeof(uint32_t) == 4, "uint32_t not defined as 4 bytes");

struct PXMClientPrivate {
    PXMClientPrivate(PXMClient* q) : q_ptr(q) {}
    PXMClient* q_ptr;
    in_addr multicastAddress;
    unsigned char packedLocalUUID[NetCompression::PACKED_UUID_LENGTH];
    size_t localUUIDLen;

    // Functions
    void sendMsg(const QSharedPointer<Peers::BevWrapper> bw,
                 const char* msg,
                 const size_t msgLen,
                 const PXMConsts::MESSAGE_TYPE type,
                 const QUuid uuidReceiver);
};
PXMClient::PXMClient(QObject* parent, in_addr multicast, QUuid localUUID)
    : QObject(parent), d_ptr(new PXMClientPrivate(this))
{
    this->setObjectName("PXMClient");

    d_ptr->multicastAddress = multicast;

    setLocalUUID(localUUID);
    d_ptr->localUUIDLen = sizeof(d_ptr->packedLocalUUID) / sizeof(d_ptr->packedLocalUUID[0]);
}

PXMClient::~PXMClient()
{
    qDebug() << "Shutdown of PXMClient Successful";
}

void PXMClient::setLocalUUID(const QUuid uuid)
{
    NetCompression::packUUID(&d_ptr->packedLocalUUID[0], uuid);
}
int PXMClient::sendUDP(const char* msg, unsigned short port)
{
    size_t len;
    struct sockaddr_in broadaddr;
    evutil_socket_t socketfd2;

    memset(&broadaddr, 0, sizeof(broadaddr));
    broadaddr.sin_family = AF_INET;
    broadaddr.sin_addr   = d_ptr->multicastAddress;
    broadaddr.sin_port   = htons(port);

    if ((socketfd2 = (socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))) < 0) {
        qCritical() << "socket: " + QString::fromUtf8(strerror(errno));
        return -1;
    }

    len = strlen(msg);

    char loopback = 1;
    if (setsockopt(socketfd2, IPPROTO_IP, IP_MULTICAST_LOOP, &loopback, sizeof(loopback)) < 0) {
        qCritical() << "setsockopt: " + QString::fromUtf8(strerror(errno));
        evutil_closesocket(socketfd2);
        return -1;
    }

    for (int i = 0; i < 1; i++) {
        ssize_t bytesSent =
            sendto(socketfd2, msg, len + 1, 0, reinterpret_cast<struct sockaddr*>(&broadaddr), sizeof(broadaddr));
        if (bytesSent < 0) {
            qCritical() << "sendto: " + QString::fromUtf8(strerror(errno));
            evutil_closesocket(socketfd2);
            return -1;
        } else if (static_cast<size_t>(bytesSent) != len + 1) {
            qWarning().noquote() << "Partial UDP send on port" << port;
            return -1;
        }
    }
    evutil_closesocket(socketfd2);
    return 0;
}

void PXMClient::sendSingleType(QSharedPointer<Peers::BevWrapper> bw, PXMConsts::MESSAGE_TYPE type)
{
    const unsigned int packetLen = d_ptr->localUUIDLen + sizeof(type);
    unsigned char packet[packetLen];

    memcpy(&packet[0], d_ptr->packedLocalUUID, d_ptr->localUUIDLen);
    uint32_t typeNBO = htonl(type);
    memcpy(&packet[d_ptr->localUUIDLen], &typeNBO, sizeof(typeNBO));

    uint16_t packetLenNBO = htons(static_cast<uint16_t>(packetLen));
    bw->lockBev();

    if ((bw->getBev() != nullptr) && (bufferevent_get_enabled(bw->getBev()) & EV_WRITE)) {
        if (bufferevent_write(bw->getBev(), &packetLenNBO, sizeof(packetLenNBO)) == 0) {
            if (bufferevent_write(bw->getBev(), packet, packetLen) == 0) {
                qDebug() << "Successful Type Send";
            }
        }
    }

    bw->unlockBev();
}

void PXMClientPrivate::sendMsg(const QSharedPointer<Peers::BevWrapper> bw,
                               const char* msg,
                               const size_t msgLen,
                               const PXMConsts::MESSAGE_TYPE type,
                               const QUuid uuidReceiver)
{
    int bytesSent = -1;
    size_t packetLen;
    uint16_t packetLenNBO;
    bool print = false;

    QUuid uuidLocal;
    NetCompression::unpackUUID(packedLocalUUID, uuidLocal);
    if (msgLen > 65400) {
        emit q_ptr->resultOfTCPSend(-1, uuidReceiver, uuidLocal, QString("Message too Long!"), print, bw);
        return;
    }
    packetLen = localUUIDLen + sizeof(type) + msgLen;

    if (type == PXMConsts::MSG_TEXT)
        print = true;

    QScopedArrayPointer<char> full_mess(new char[packetLen + 1]);

    packetLenNBO = htons(static_cast<uint16_t>(packetLen));
    qDebug() << ntohs(packetLenNBO);
    uint32_t typeNBO = htonl(type);

    memcpy(&full_mess[0], packedLocalUUID, localUUIDLen);
    memcpy(&full_mess[localUUIDLen], &typeNBO, sizeof(typeNBO));
    memcpy(&full_mess[localUUIDLen + sizeof(typeNBO)], msg, msgLen);
    full_mess[packetLen] = 0;

    bw->lockBev();

    if ((bw->getBev() == nullptr) || !(bufferevent_get_enabled(bw->getBev()) & EV_WRITE)) {
        msg = "Peer is Disconnected, message not sent";
    } else {
        if (bufferevent_write(bw->getBev(), &packetLenNBO, sizeof(packetLenNBO)) == 0) {
            if (bufferevent_write(bw->getBev(), full_mess.data(), packetLen) == 0) {
                qDebug() << "Successful Send";
                bytesSent = 0;
            } else {
                msg = "Message send failure, not sent";
            }
        } else {
            msg = "Message send failure, not sent";
        }
    }

    bw->unlockBev();

    if (!uuidReceiver.isNull()) {
        emit q_ptr->resultOfTCPSend(bytesSent, uuidReceiver, uuidLocal, QString::fromUtf8(msg), print, bw);
    }

    return;
}
void PXMClient::sendMsgSlot(QSharedPointer<Peers::BevWrapper> bw,
                            QByteArray msg,
                            size_t len,
                            PXMConsts::MESSAGE_TYPE type,
                            QUuid theiruuid)
{
    QByteArray test = QByteArray::fromRawData(msg.data(), len);
    d_ptr->sendMsg(bw, test.constData(), len, type, theiruuid);
}
