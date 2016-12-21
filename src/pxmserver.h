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
#include "uuidcompression.h"

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

const timeval READ_TIMEOUT = {1, 0};
const timeval READ_TIMEOUT_RESET = {3600, 0};

class PXMServer : public QThread
{
    Q_OBJECT

    QString localHostname;
    QUuid localUUID;
    unsigned short tcpListenerPort;
    unsigned short udpListenerPort;
    in_addr multicastAddress;
    bool gotDiscover;

    int singleMessageIterator(bufferevent *bev, char *buf, uint16_t len,
                              QUuid quuid);
    unsigned short getPortNumber(evutil_socket_t socket);
    evutil_socket_t setupUDPSocket(evutil_socket_t s_listen);
    evutil_socket_t setupTCPSocket();
public:
    PXMServer(QObject *parent, unsigned short tcpPort, unsigned short udpPort,
              in_addr multicast);
    PXMServer(PXMServer const&) = delete;
    PXMServer& operator=(PXMServer const&) = delete;
    PXMServer& operator=(PXMServer&&) noexcept = delete;
    PXMServer(PXMServer&&) noexcept = delete;
    void run() Q_DECL_OVERRIDE;
    int setLocalHostname(QString hostname);
    int setLocalUUID(QUuid uuid);
    ~PXMServer();
    static void tcpError(bufferevent *bev, short error, void *arg);
    static void tcpReadUUID(bufferevent *bev, void *arg);
    static void tcpRead(bufferevent *bev, void *arg);
    static void accept_new(evutil_socket_t socketfd, short, void *arg);
    static void udpRecieve(evutil_socket_t socketfd, short, void *args);
    static void stopLoopBufferevent(bufferevent *bev, void *);
    static struct event_base *base;
signals:
    void messageRecieved(QString, QUuid, bufferevent*, bool);
    void newTCPConnection(bufferevent*);
    void authenticationReceived(QString, unsigned short, evutil_socket_t,
                                QUuid, bufferevent*);
    void peerQuit(evutil_socket_t, bufferevent*);
    void attemptConnection(sockaddr_in, QUuid);
    void sendSyncPacket(bufferevent*, QUuid);
    void sendName(bufferevent*, QString, QString);
    void syncPacketIterator(char*, size_t, QUuid);
    void setPeerHostname(QString, QUuid);
    void sendUDP(const char*, unsigned short);
    void setListenerPorts(unsigned short, unsigned short);
    void libeventBackend(QString);
    void setCloseBufferevent(bufferevent*);
    void setSelfCommsBufferevent(bufferevent*);
    void multicastIsFunctional();
    void serverSetupFailure();
};

#endif // MESS_SERV_H
