#ifndef PXMSERVER_H
#define PXMSERVER_H

#include <QThread>
#include <QWidget>
#include <QUuid>
#include <QDebug>

//TEMP
#include <QMessageBox>

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

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <ctime>
#include <fcntl.h>
#endif

const timeval readTimeout {1, 0};

class PXMServer : public QThread
{
    Q_OBJECT

public:
    PXMServer(QWidget *parent, unsigned short tcpPort, unsigned short udpPort);
    void run() Q_DECL_OVERRIDE;
    void setLocalHostname(QString hostname);
    void setLocalUUID(QString uuid);
    ~PXMServer();
    static void tcpError(bufferevent *bev, short error, void *arg);
    static void tcpReadUUID(bufferevent *bev, void *arg);
    static QUuid unpackUUID(unsigned char *src);
    static void tcpRead(bufferevent *bev, void *arg);
    static void accept_new(evutil_socket_t socketfd, short, void *arg);
    static void udpRecieve(evutil_socket_t socketfd, short, void *args);
    static void stopLoopBufferevent(bufferevent *bev, void *);
    static struct event_base *base;
private:
    QString localHostname;
    QString localUUID;
    unsigned short tcpListenerPort;
    unsigned short udpListenerPort;
    bool gotDiscover;

    int singleMessageIterator(bufferevent *bev, char *buf, uint16_t len, QUuid quuid);
    int getPortNumber(evutil_socket_t socket);
    evutil_socket_t setupUDPSocket(evutil_socket_t s_listen);
    evutil_socket_t setupTCPSocket();
signals:
    void messageRecieved(QString, QUuid, bufferevent*, bool);
    void newTCPConnection(bufferevent*);
    void authenticationReceived(QString, unsigned short, evutil_socket_t, QUuid, bufferevent*);
    void peerQuit(evutil_socket_t, bufferevent*);
    void attemptConnection(sockaddr_in, QUuid);
    void sendIps(bufferevent*, QUuid);
    void sendName(bufferevent*, QString, QString);
    void hostnameCheck(char*, size_t, QUuid);
    void setPeerHostname(QString, QUuid);
    void sendUDP(const char*, unsigned short);
    void setListenerPort(unsigned short);
    void libeventBackend(QString);
    void setCloseBufferevent(bufferevent*);
    void setSelfCommsBufferevent(bufferevent*);
    void multicastIsFunctional();
};

#endif // MESS_SERV_H
