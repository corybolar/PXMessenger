#include <pxmclient.h>

#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>

#include <event2/bufferevent.h>
#include <event2/event.h>

#include <QDebug>

#include "pxmpeers.h"
#include "netcompression.h"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#endif

static_assert(sizeof(uint8_t) == 1, "uint8_t not defined as 1 byte");
static_assert(sizeof(uint16_t) == 2, "uint16_t not defined as 2 bytes");
static_assert(sizeof(uint32_t) == 4, "uint32_t not defined as 4 bytes");

struct PXMClientPrivate {
    in_addr multicastAddress;
    unsigned char packedLocalUUID[NetCompression::PACKED_UUID_LENGTH];
};
PXMClient::PXMClient(QObject* parent, in_addr multicast, QUuid localUUID) : QObject(parent), d_ptr(new PXMClientPrivate)
{
    this->setObjectName("PXMClient");

    d_ptr->multicastAddress = multicast;

    setLocalUUID(localUUID);
}

PXMClient::~PXMClient()
{
    qDebug() << "Shutdown of PXMClient Successful";
}

void PXMClient::setLocalUUID(QUuid uuid)
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
    return -1;
}

void PXMClient::sendMsg(QSharedPointer<Peers::BevWrapper> bw,
                        const char* msg,
                        size_t msgLen,
                        PXMConsts::MESSAGE_TYPE type,
                        QUuid uuidReceiver)
{
    int bytesSent = -1;
    size_t packetLen;
    uint16_t packetLenNBO;
    bool print = false;

    if (msgLen > 65400) {
        emit resultOfTCPSend(-1, uuidReceiver, QString("Message too Long!"), print, bw);
        return;
    }
    packetLen = NetCompression::PACKED_UUID_LENGTH + sizeof(PXMConsts::MESSAGE_TYPE) + msgLen;

    if (type == PXMConsts::MSG_TEXT)
        print = true;

    QScopedArrayPointer<char> full_mess(new char[packetLen + 1]);
    // char full_mess[packetLen + 1];

    packetLenNBO     = htons(static_cast<uint16_t>(packetLen));
    uint32_t typeNBO = htonl(type);

    memcpy(&full_mess[0], d_ptr->packedLocalUUID, NetCompression::PACKED_UUID_LENGTH);
    memcpy(&full_mess[NetCompression::PACKED_UUID_LENGTH], &typeNBO, sizeof(PXMConsts::MESSAGE_TYPE));
    memcpy(&full_mess[NetCompression::PACKED_UUID_LENGTH + sizeof(PXMConsts::MESSAGE_TYPE)], msg, msgLen);
    full_mess[packetLen] = 0;

    bw->lockBev();

    if ((bw->getBev() == nullptr) || !(bufferevent_get_enabled(bw->getBev()) & EV_WRITE)) {
        msg = "Peer is Disconnected, message not sent";
    } else {
        if (bufferevent_write(bw->getBev(), &packetLenNBO, sizeof(uint16_t)) == 0) {
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
        emit resultOfTCPSend(bytesSent, uuidReceiver, QString::fromUtf8(msg), print, bw);
    }

    return;
}
void PXMClient::sendMsgSlot(QSharedPointer<Peers::BevWrapper> bw,
                            QByteArray msg,
                            PXMConsts::MESSAGE_TYPE type,
                            QUuid theiruuid)
{
    this->sendMsg(bw, msg.constData(), msg.length(), type, theiruuid);
}

void PXMClient::sendIpsSlot(QSharedPointer<Peers::BevWrapper> bw,
                            unsigned char* msg,
                            size_t len,
                            PXMConsts::MESSAGE_TYPE type,
                            QUuid theiruuid)
{
    this->sendMsg(bw, (char*)msg, len, type, theiruuid);
    delete[] msg;
}
