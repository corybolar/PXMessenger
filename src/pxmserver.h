#ifndef PXMSERVER_H
#define PXMSERVER_H

#include <QThread>
#include <QUuid>

#include <sys/time.h>

#include <event2/util.h>

struct bufferevent;
struct event_base;

namespace PXMServer{
const timeval READ_TIMEOUT = {1, 0};
const timeval READ_TIMEOUT_RESET = {3600, 0};
const uint8_t PACKET_HEADER_LENGTH = 2;
class ServerThread : public QThread
{
    Q_OBJECT

    struct servImpl;
    servImpl *pImpl;

    int singleMessageIterator(bufferevent *bev, char *buf, uint16_t len,
                              QUuid quuid);
    unsigned short getPortNumber(evutil_socket_t socket);
    evutil_socket_t setupUDPSocket(evutil_socket_t s_listen);
    evutil_socket_t setupTCPSocket();
public:
    ServerThread(QObject *parent, unsigned short tcpPort,
                 unsigned short udpPort, in_addr multicast);
    ServerThread(ServerThread const&) = delete;
    ServerThread& operator=(ServerThread const&) = delete;
    ServerThread& operator=(ServerThread&&) noexcept = delete;
    ServerThread(ServerThread&&) noexcept = delete;
    void run() Q_DECL_OVERRIDE;
    int setLocalHostname(QString hostname);
    int setLocalUUID(QUuid uuid);
    ~ServerThread();
    static void tcpError(bufferevent *bev, short error, void *arg);
    static void tcpAuth(bufferevent *bev, void *arg);
    static void tcpRead(bufferevent *bev, void *arg);
    static void accept_new(evutil_socket_t socketfd, short, void *arg);
    static void udpRecieve(evutil_socket_t socketfd, short, void *args);
    static void stopLoopBufferevent(bufferevent *bev, void *);
    static struct event_base *base;
signals:
    void messageRecieved(QString, QUuid, bufferevent*, bool);
    void newTCPConnection(bufferevent*);
    void authenticationReceived(QString, unsigned short, QString,
                                evutil_socket_t, QUuid, bufferevent*);
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
    void nameChange(QString, QUuid);
};

}

#endif // MESS_SERV_H
