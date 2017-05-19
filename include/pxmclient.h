/** @file pxmclient.h
 * @brief Public header for pxmclient.cpp
 *
 * Primary class used to send messages over the network
 */

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
    /** Sets the uuid for use in comms
     * @brief setLocalUUID
     * @param uuid QUuid to set
     */
    void setLocalUUID(QUuid uuid);
   public slots:
    /*!
     * \brief sendMsgSlot
     * Function for sending data to already connected peers.
     * \param bw pointer containing a Bevwrapper, which contains a bufferevent
     * \param msg bytes to send, does not need to be null terminated
     * \param len length of bytes to send
     * \param type type of message to send in header
     * \param theiruuid recipients uuid, only used when confirming message was
     * 			sent
     */
    void sendMsgSlot(QSharedPointer<Peers::BevWrapper> bw,
                     QByteArray msg,
                     size_t len,
                     PXMConsts::MESSAGE_TYPE type,
                     QUuid theiruuid = QUuid());
    /*!
     * \brief sendUDP
     * Sends a UDP packet to the multicast group
     * \param msg message to send, must be null-terminated
     * \param port port on the multicast to send to
     * \return 0 on success, -1 on error
     */
    int sendUDP(const char* msg, unsigned short port);
   signals:
    /*!
     * \brief resultOfTCPSend
     * This returns the result of the sendMsgSlot, with information about how
     * successful it was contained in the first integer.
     */
    void resultOfTCPSend(int, QUuid, QUuid, QString, bool, QSharedPointer<Peers::BevWrapper>);
};

#endif  // PXMCLIENT_H
