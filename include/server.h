/*! @file pxmserver.h
 * @brief public header for for pxmserver.cpp
 *
 * Manages the listener socket and all subsequent connections made to this
 * device.
 */

#ifndef PXMSERVER_H
#define PXMSERVER_H

#include <sys/time.h>

#include <QThread>
#include <QUuid>
#include <QScopedPointer>
#include <QSharedPointer>

#include <event2/util.h>
#include <consts.h>

struct bufferevent;
struct event_base;
class ServerThreadPrivate;

namespace PXMServer
{
const timeval READ_TIMEOUT         = {1, 0}; /*!< timeout if we receive partial
                                               packets */
const timeval READ_TIMEOUT_RESET   = {3600, 0}; /*!< workaround for libevent
                                                  bug, fixed in 2.1 */
const uint8_t PACKET_HEADER_LEN = 2;
const size_t INTERNAL_MSG_LENGTH 	  = 200;
enum INTERNAL_MSG : uint16_t {
    ADD_DEFAULT_BEV = 0x1111,
    EXIT            = 0x2222,
    CONNECT_TO_ADDR = 0x3333,
    SSL_CONNECT_TO_ADDR = 0x4444,
    TCP_PORT_CHANGE = 0x8888,  // Under Construction
    UDP_PORT_CHANGE = 0x9999   // Under Construction
};
class ServerThread : public QThread
{
    Q_OBJECT
    QScopedPointer<ServerThreadPrivate> d_ptr;

   public:
    // Default
    ServerThread(QObject* parent,
                 QUuid uuid,
                 in_addr multicast,
                 unsigned short tcpPort = 0,
                 unsigned short udpPort = 0);

    // Copy
    ServerThread(ServerThread const&) = delete;
    // Copy assignment
    ServerThread& operator=(ServerThread const&) = delete;
    // Move
    ServerThread(ServerThread&& st) noexcept = delete;
    // Move assignment
    ServerThread& operator=(ServerThread&& st) noexcept = delete;
    // Destructor
    ~ServerThread();

    void run() Q_DECL_OVERRIDE;
   signals:
    /*!
     * \brief packetHandler
     * Send a received packet to a handler to be dealt with
     */
    void packetHandler(const QSharedPointer<unsigned char>, const size_t, const PXMConsts::MESSAGE_TYPE, const QUuid, const bufferevent*);
    /*!
     * \brief newTCPConnection
     * New connection has been received via accept().  No action has been taken
     * with it yet.
     */
    void newTCPConnection(bufferevent*, bool);
    /*!
     * \brief authHandler
     * Authentication packet has been received and needs to be dealt with.
     */
    void authHandler(QStringList, evutil_socket_t, QUuid, bufferevent*);
    /*!
     * \brief peerQuit
     * A connection has been terminated
     */
    void peerQuit(evutil_socket_t, bufferevent*);
    /*!
     * \brief attemptConnection
     * Recieved a udp "name" packet, we probably want to connect to the address
     * that came with it.  Pass this info along to see if we are already 
     * connected to it.
     */
    void attemptConnection(struct sockaddr_in, QUuid, bool);
    void sendName(bufferevent*, QString, QString);
    void setPeerHostname(QString, QUuid);
    void sendUDP(const char*, unsigned short);
    void setListenerPorts(unsigned short, unsigned short, unsigned short);
    void libeventBackend(QString, QString);
    void setInternalBufferevent(bufferevent*);
    void setSelfCommsBufferevent(bufferevent*);
    void multicastIsFunctional();
    void serverSetupFailure(QString);
    void resultOfConnectionAttempt(evutil_socket_t, bool, bufferevent*, QUuid, bool);
};
}

#endif  // MESS_SERV_H
