#ifndef PXMCLIENT_H
#define PXMCLIENT_H

#include <QObject>
#include <QUuid>
#include "pxmconsts.h"

#include <event2/util.h>

struct bufferevent;
namespace Peers
{
class BevWrapper;
}

class PXMClient : public QObject
{
    Q_OBJECT
    int recursiveSend(evutil_socket_t socketfd, void *msg, int len, int count);
    in_addr multicastAddress;
    char packedLocalUUID[16];
public:
    PXMClient(QObject *parent, in_addr multicast, QUuid localUUID);
    ~PXMClient();
    void setLocalUUID(QUuid uuid);
public slots:
    void sendMsg(QSharedPointer<Peers::BevWrapper> bw, const char *msg,
                 size_t msgLen,PXMConsts::MESSAGE_TYPE type,
                 QUuid uuidReceiver);
    void sendMsgSlot(QSharedPointer<Peers::BevWrapper> bw, QByteArray msg,
                     PXMConsts::MESSAGE_TYPE type, QUuid theiruuid = QUuid());
    int sendUDP(const char* msg, unsigned short port);
    void connectToPeer(evutil_socket_t, sockaddr_in socketAddr,
                       QSharedPointer<Peers::BevWrapper> bw);
    void sendIpsSlot(QSharedPointer<Peers::BevWrapper> bw, char *msg,
                     size_t len, PXMConsts::MESSAGE_TYPE type,
                     QUuid theiruuid = QUuid());
    static void connectCB(bufferevent *bev, short event, void *arg);
signals:
    void resultOfConnectionAttempt(evutil_socket_t, bool, bufferevent *);
    void resultOfTCPSend(unsigned int, QUuid, QString, bool,
                         QSharedPointer<Peers::BevWrapper>);
};

#endif // PXMCLIENT_H
