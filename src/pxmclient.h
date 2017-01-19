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
    in_addr multicastAddress;
    char packedLocalUUID[16];
public:
    PXMClient(QObject *parent, in_addr multicast, QUuid localUUID);
    ~PXMClient();
    /** Sets the uuid for this program for use in comms
     * @brief setLocalUUID
     * @param uuid QUuid to set
     */
    void setLocalUUID(QUuid uuid);
public slots:
    /** Send a byte array to another computer
     *
     *
     * @brief sendMsg
     * @param bw The BevWrapper to use for sending /see Peers::BevWrapper
     * @param msg The data to send
     * @param msgLen Number of bytes to send
     * @param type Type of message to append to front of message
     * @param uuidReceiver Specifies a uuid for returning a result
     */
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
