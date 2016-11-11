#ifndef PXMSERVER_H
#define PXMSERVER_H

#include <QThread>
#include <QWidget>
#include <QUuid>
#include <QDebug>

#include <ctime>
#include <sys/types.h>
#include <sys/time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>

#include <event2/event.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/util.h>
#include <event2/thread.h>

#include "pxmdefinitions.h"

#ifdef __unix__
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <ctime>
#include <fcntl.h>
#endif

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

class PXMServer : public QThread
{
    Q_OBJECT

public:
    PXMServer(QWidget *parent, unsigned short tcpPort, unsigned short udpPort);
    int listener();
    void run();
    void setLocalHostname(QString hostname);
    void setLocalUUID(QString uuid);
    ~PXMServer();
    struct event_base *base;
    static void tcpError(bufferevent *buf, short error, void *arg);
    static void tcpRead(bufferevent *bev, void *arg);
public slots:
private:
    static void udpRecieve(evutil_socket_t socketfd, short event, void *args);
    static void accept_new(evutil_socket_t socketfd, short event, void *arg);
    QString localHostname;
    QString localUUID;
    unsigned short tcpListenerPort;
    unsigned short udpListenerPort;

    int singleMessageIterator(evutil_socket_t i, char *buf, uint32_t len, bufferevent *bev);
    int getPortNumber(evutil_socket_t socket);
    evutil_socket_t setupUDPSocket(evutil_socket_t s_listen);
    evutil_socket_t setupTCPSocket();
    int setSocketToNonBlocking(evutil_socket_t socket);
signals:
    void messageRecieved(const QString, QUuid, bool);
    void newConnectionRecieved(evutil_socket_t);
    void recievedUUIDForConnection(QString, QString, evutil_socket_t, QUuid, void*);
    void peerQuit(evutil_socket_t);
    void updNameRecieved(QString hname, QString ipaddr, QString uuid);
    void potentialReconnect(QString);
    void exitRecieved(QString);
    void sendIps(evutil_socket_t);
    void sendName(evutil_socket_t, QString, QString);
    void hostnameCheck(QString, QUuid);
    void setPeerHostname(QString, QUuid);
    void sendMsg(evutil_socket_t, QString, QString, QUuid, QString);
    void sendUdp(QString, unsigned short);
    void setListenerPort(unsigned short);
};

#endif // MESS_SERV_H
