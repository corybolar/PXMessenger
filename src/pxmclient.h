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

#include "pxmdefinitions.h"

#ifdef __unix__
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#endif

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

class PXMClient : public QObject
{
    Q_OBJECT
public:
    PXMClient();
    void setlocalUUID(QString uuid);
public slots:
    void connectToPeerSlot(evutil_socket_t s, QString ipaddr, QString service);
    void sendMsgSlot(evutil_socket_t s, QString msg, QString type, QUuid uuid, QString theiruuid);
    void udpSendSlot(QString msg, unsigned short port);
private:
    int connectToPeer(evutil_socket_t socketfd, const char *ipaddr, const char *service);
    void sendMsg(evutil_socket_t socketfd, const char *msg, const char *type, const char *uuid, const char *theiruuid);
    void udpSend(const char *msg, unsigned short port);
    int recursiveSend(evutil_socket_t socketfd, const char *msg, int len, int count);
signals:
    void resultOfConnectionAttempt(evutil_socket_t, bool);
    void resultOfTCPSend(evutil_socket_t, QString, QString, bool);
};

#endif // PXMCLIENT_H
