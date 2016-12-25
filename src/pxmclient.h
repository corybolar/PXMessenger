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
    char packedLocalUUID[16];
public:
    PXMClient(QObject *parent, in_addr multicast, QUuid localUUID);
    ~PXMClient() {qDebug() << "Shutdown of PXMClient Successful";}
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
