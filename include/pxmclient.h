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
    void sendMsgSlot(QSharedPointer<Peers::BevWrapper> bw,
                     QByteArray msg,
                     size_t len,
                     PXMConsts::MESSAGE_TYPE type,
                     QUuid theiruuid = QUuid());
    int sendUDP(const char* msg, unsigned short port);
   signals:
    void resultOfTCPSend(int, QUuid, QString, bool, QSharedPointer<Peers::BevWrapper>);
};

#endif  // PXMCLIENT_H
