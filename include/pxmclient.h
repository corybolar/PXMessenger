#ifndef PXMCLIENT_H
#define PXMCLIENT_H

#include "pxmconsts.h"
#include <QObject>
#include <QUuid>

#include <event2/util.h>

struct bufferevent;
namespace Peers
{
class BevWrapper;
}

struct PXMClientPrivate;
class PXMClient : public QObject
{
    Q_OBJECT
    QScopedPointer<PXMClientPrivate> d_ptr;

   public:
    PXMClient(QObject* parent, in_addr multicast, QUuid localUUID);
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
    void sendMsg(QSharedPointer<Peers::BevWrapper> bw,
                 const char* msg,
                 size_t msgLen,
                 PXMConsts::MESSAGE_TYPE type,
                 QUuid uuidReceiver);
    void sendMsgSlot(QSharedPointer<Peers::BevWrapper> bw,
                     QByteArray msg,
                     PXMConsts::MESSAGE_TYPE type,
                     QUuid theiruuid = QUuid());
    int sendUDP(const char* msg, unsigned short port);
    //void connectToPeer(evutil_socket_t, struct sockaddr_in socketAddr,
    //                   QSharedPointer<Peers::BevWrapper> bw);
    void sendIpsSlot(QSharedPointer<Peers::BevWrapper> bw,
                     QSharedPointer<unsigned char> msg,
                     size_t len,
                     PXMConsts::MESSAGE_TYPE type,
                     QUuid theiruuid = QUuid());
    //static void connectCB(bufferevent* bev, short event, void* arg);
   signals:
    void resultOfTCPSend(int, QUuid, QString, bool,
                         QSharedPointer<Peers::BevWrapper>);
};

#endif  // PXMCLIENT_H
