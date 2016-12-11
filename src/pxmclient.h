#ifndef PXMCLIENT_H
#define PXMCLIENT_H

#include <QWidget>
#include <QUuid>
#include <QDebug>

#include <sys/types.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>

#include <event2/event.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>

#include "pxmdefinitions.h"
#include "uuidcompression.h"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#endif

class PXMClient : public QObject
{
    Q_OBJECT
    int recursiveSend(evutil_socket_t socketfd, void *msg, int len, int count);
    in_addr multicastAddress;
public:
    PXMClient(QObject *parent, in_addr multicast);
    ~PXMClient() {qDebug() << "Shutdown of PXMClient Successful";}
public slots:
    void sendMsg(BevWrapper *bw, const char *msg, size_t msgLen, PXMConsts::MESSAGE_TYPE type, QUuid uuidSender, QUuid uuidReceiver);
    /*!
     * \brief sendMsgSlot
     *
     * Slot for calling sendMsg.  QString to char* converstion
     * \param s socket to send on
     * \param msg message to send
     * \param type packet type to send as
     * \param uuid our UUID
     * \param theiruuid recipient UUID, only used to report results, not
     * 			included in packet
     * \see sendMsg()
     */
    void sendMsgSlot(BevWrapper *bw, QByteArray msg, PXMConsts::MESSAGE_TYPE type, QUuid uuid, QUuid theiruuid);
    /*!
     * \brief sendUDP
     *
     * Sends a udp packet to the multicast group defined by MULTICAST_ADDRESS.
     * Important:These messages are currently send twice to improves odds that
     * one of them gets their.
     * \param msg Message to send
     * \param port Port to send to in the multicast group
     */
    int sendUDP(const char* msg, unsigned short port);
    /*!
     * \brief connectToPeer
     *
     * Establish connection to peer.
     * \param socketfd Socket to connect on
     * \param socketAddr sockaddr_in structure containing port and ip to connect
     * 			to
     * \return emits resultOfConnectionAttempt() with the socket number and a
     * 			bool variable containing the result.
     * \see man connect
     */
    void connectToPeer(evutil_socket_t, sockaddr_in socketAddr, bufferevent *bev);
    void sendIpsSlot(BevWrapper *bw, char *msg, size_t len, PXMConsts::MESSAGE_TYPE type, QUuid uuid, QUuid theiruuid);
    static void connectCB(bufferevent *bev, short event, void *arg);
signals:
    void resultOfConnectionAttempt(evutil_socket_t, bool, bufferevent*);
    void resultOfTCPSend(unsigned int, QUuid, QString, bool, BevWrapper*);
};

#endif // PXMCLIENT_H
