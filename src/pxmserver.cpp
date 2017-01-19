#include <pxmserver.h>
#include <QDebug>
#include <QUuid>

#include <unistd.h>
#include <sys/types.h>
#include <stdint.h>

#include <event2/event.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/util.h>
#include <event2/thread.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <fcntl.h>
#endif

#include "pxmpeers.h"
#include "pxmconsts.h"

using namespace PXMConsts;
using namespace PXMServer;

struct event_base* ServerThread::base = nullptr;
struct ServerThreadPrivate{
    ServerThreadPrivate(ServerThread *q) : q_ptr(q) {}
    ServerThread *q_ptr;
    //Data Members
    QUuid localUUID;
    unsigned short tcpListenerPort;
    unsigned short udpListenerPort;
    in_addr multicastAddress;
    bool gotDiscover;
    //Functions
    evutil_socket_t setupUDPSocket(evutil_socket_t s_listen);
    evutil_socket_t setupTCPSocket();
    unsigned short getPortNumber(evutil_socket_t socket);
    int singleMessageIterator(bufferevent *bev, char *buf, uint16_t len,
                              QUuid quuid);
};

ServerThread::ServerThread(QObject *parent, unsigned short tcpPort,
                           unsigned short udpPort, in_addr multicast) :
    QThread(parent), d_ptr(new ServerThreadPrivate(this))
{
    //Init
    d_ptr->gotDiscover = false;
    //End of Init

    d_ptr->tcpListenerPort = tcpPort;

    if(udpPort == 0)
        d_ptr->udpListenerPort = PXMConsts::DEFAULT_UDP_PORT;
    else
        d_ptr->udpListenerPort = udpPort;

    d_ptr->multicastAddress = multicast;
    this->setObjectName("PXMServer");
}
ServerThread::~ServerThread()
{
    if(d_ptr)
    {
        delete d_ptr;
        d_ptr = 0;
    }
    if(base)
    {
        event_base_free(base);
        base = 0;
    }
    qDebug() << "Shutdown of PXMServer Successful";
}
int ServerThread::setLocalUUID(QUuid uuid)
{
    d_ptr->localUUID = uuid;
    return 0;
}
void ServerThread::accept_new(evutil_socket_t s, short, void *arg)
{
    evutil_socket_t result;

    ServerThread *realServer = static_cast<ServerThread*>(arg);

    struct sockaddr_in ss;
    socklen_t addr_size = sizeof(ss);

    result = accept(s, (struct sockaddr *)&ss, &addr_size);
    if(result < 0)
    {
        qCritical() << "accept: " << QString::fromUtf8(strerror(errno));
    }
    else
    {
        struct bufferevent *bev;
        evutil_make_socket_nonblocking(result);
        bev = bufferevent_socket_new(ServerThread::base, result, BEV_OPT_THREADSAFE);
        bufferevent_setcb(bev, ServerThread::tcpAuth, NULL, ServerThread::tcpError, realServer);
        bufferevent_setwatermark(bev, EV_READ, PACKET_HEADER_LENGTH, PACKET_HEADER_LENGTH);
        bufferevent_enable(bev, EV_READ|EV_WRITE);

        realServer->newTCPConnection(bev);
    }
}

void ServerThread::tcpAuth(struct bufferevent *bev, void *arg)
{
    ServerThread *server_ptr = static_cast<ServerThread*>(arg);
    uint16_t nboBufLen;
    uint16_t bufLen;
    evutil_socket_t socket = bufferevent_getfd(bev);

    if(evbuffer_get_length(bufferevent_get_input(bev)) == PACKET_HEADER_LENGTH)
    {
        evbuffer_copyout(bufferevent_get_input(bev), &nboBufLen, PACKET_HEADER_LENGTH);
        bufLen = ntohs(nboBufLen);
        if(bufLen == 0 || bufLen > MAX_AUTH_PACKET_LENGTH)
        {
            qWarning().noquote() << "Bad buffer length, disconnecting";
            bufferevent_disable(bev, EV_READ|EV_WRITE);
            server_ptr->peerQuit(socket, bev);
            return;
        }
        bufferevent_setwatermark(bev, EV_READ, bufLen + PACKET_HEADER_LENGTH, bufLen + PACKET_HEADER_LENGTH);
        return;
    }
    bufferevent_read(bev, &nboBufLen, PACKET_HEADER_LENGTH);

    bufLen = ntohs(nboBufLen);

    unsigned char bufUUID[UUIDCompression::PACKED_UUID_LENGTH];
    if(bufferevent_read(bev, bufUUID, UUIDCompression::PACKED_UUID_LENGTH) < UUIDCompression::PACKED_UUID_LENGTH)
    {
        qWarning() << "Bad Auth packet length, closing socket...";
        bufferevent_disable(bev, EV_READ|EV_WRITE);
        server_ptr->peerQuit(socket, bev);
        return;
    }
    QUuid quuid = UUIDCompression::unpackUUID(bufUUID);
    bufLen -= UUIDCompression::PACKED_UUID_LENGTH;
    if(quuid.isNull())
    {
        qWarning() << "Bad Auth packet UUID, closing socket...";
        bufferevent_disable(bev, EV_READ|EV_WRITE);
        server_ptr->peerQuit(socket, bev);
        return;
    }

    char buf[bufLen + 1];
    bufferevent_read(bev, buf, bufLen);
    buf[bufLen] = 0;
    qDebug().noquote() << QString::fromUtf8(&buf[sizeof(MESSAGE_TYPE)]);
    MESSAGE_TYPE *type = (MESSAGE_TYPE*)&buf[0];
    if(*type == MSG_AUTH)
    {
        //Auth packet format "Hostname:::12345:::001.001.001"
        bufLen -= sizeof(MESSAGE_TYPE);
        QStringList hpsplit = (QString::fromUtf8(&buf[sizeof(MESSAGE_TYPE)], bufLen)).split(AUTH_SEPERATOR);
        if(hpsplit.length() != 3)
        {
            qWarning() << "Bad Auth packet, closing socket...";
            bufferevent_disable(bev, EV_READ|EV_WRITE);
            server_ptr->peerQuit(socket, bev);
            return;
        }
        unsigned short port = hpsplit[1].toUShort();
        if(port == 0)
        {
            qWarning() << "Bad port in Auth packet, closing socket...";
            bufferevent_disable(bev, EV_READ|EV_WRITE);
            server_ptr->peerQuit(socket, bev);
            return;
        }
        server_ptr->authenticationReceived(hpsplit[0], port, hpsplit[2], socket, quuid, bev);
        bufferevent_setwatermark(bev, EV_READ, PACKET_HEADER_LENGTH, PACKET_HEADER_LENGTH);
        bufferevent_setcb(bev, ServerThread::tcpRead, NULL, ServerThread::tcpError, server_ptr);
    }
    else
    {
        qWarning() << "Non-Auth packet, closing socket...";
        bufferevent_disable(bev, EV_READ|EV_WRITE);
        server_ptr->peerQuit(socket, bev);
    }
}
void ServerThread::tcpRead(struct bufferevent *bev, void *arg)
{
    ServerThread *server_ptr = static_cast<ServerThread*>(arg);
    uint16_t nboBufLen;
    uint16_t bufLen;
    evbuffer *input = bufferevent_get_input(bev);
    if(evbuffer_get_length(input) == 1)
    {
        qDebug().noquote() << "Setting timeout, 1 byte recieved";
        bufferevent_set_timeouts(bev, &READ_TIMEOUT, NULL);
        return;
    }
    if(evbuffer_get_length(bufferevent_get_input(bev)) <= PACKET_HEADER_LENGTH)
    {
        qDebug().noquote() << "Recieved bufferlength value";
        evbuffer_copyout(input, &nboBufLen, PACKET_HEADER_LENGTH);
        bufLen = ntohs(nboBufLen);
        if(bufLen == 0)
        {
            qWarning().noquote() << "Bad buffer length, draining...";
            evbuffer_drain(input, UINT16_MAX);
            return;
        }
        bufferevent_setwatermark(bev, EV_READ, bufLen + PACKET_HEADER_LENGTH, bufLen + PACKET_HEADER_LENGTH);
        bufferevent_set_timeouts(bev, &READ_TIMEOUT, NULL);
        qDebug().noquote() << "Setting watermark to " + QString::number(bufLen) + " bytes";
        qDebug().noquote() << "Setting timeout to " +
                              QString::asprintf("%ld.%06ld", READ_TIMEOUT.tv_sec, READ_TIMEOUT.tv_usec) + " seconds";
        return;
    }
    qDebug() << "Full packet received";
    bufferevent_setwatermark(bev, EV_READ, PACKET_HEADER_LENGTH, PACKET_HEADER_LENGTH);

    bufferevent_read(bev, &nboBufLen, PACKET_HEADER_LENGTH);

    bufLen = ntohs(nboBufLen);
    if(bufLen <= UUIDCompression::PACKED_UUID_LENGTH)
    {
        evbuffer_drain(bufferevent_get_input(bev), UINT16_MAX);
        return;
    }

    unsigned char rawUUID[UUIDCompression::PACKED_UUID_LENGTH];
    bufferevent_read(bev, rawUUID, UUIDCompression::PACKED_UUID_LENGTH);

    QUuid uuid = UUIDCompression::unpackUUID(rawUUID);
    if(uuid.isNull())
    {
        evbuffer_drain(bufferevent_get_input(bev), UINT16_MAX);
        return;
    }
    qInfo() << "Sender Uuid for message" << uuid.toString();

    bufLen -= UUIDCompression::PACKED_UUID_LENGTH;
    char *buf = new char[bufLen + 1];
    bufferevent_read(bev, buf, bufLen);
    buf[bufLen] = 0;

    server_ptr->d_ptr->singleMessageIterator(bev, buf, bufLen, uuid);

    bufferevent_set_timeouts(bev, &READ_TIMEOUT_RESET, NULL);

    delete [] buf;
}

void ServerThread::tcpError(struct bufferevent *bev, short error, void *arg)
{
    ServerThread *server_ptr = static_cast<ServerThread*>(arg);
    evutil_socket_t i = bufferevent_getfd(bev);
    if (error & BEV_EVENT_EOF)
    {
        qDebug() << "BEV EOF";
        bufferevent_disable(bev, EV_READ|EV_WRITE);
        server_ptr->peerQuit(i, bev);
    }
    else if (error & BEV_EVENT_ERROR)
    {
        qDebug() << "BEV ERROR";
        bufferevent_disable(bev, EV_READ|EV_WRITE);
        server_ptr->peerQuit(i, bev);
    }
    else if (error & BEV_EVENT_TIMEOUT)
    {
        qDebug() << "BEV TIMEOUT";
        bufferevent_setwatermark(bev, EV_READ, PACKET_HEADER_LENGTH, PACKET_HEADER_LENGTH);
        bufferevent_set_timeouts(bev, &READ_TIMEOUT_RESET, NULL);
        bufferevent_enable(bev, EV_READ|EV_WRITE);
        evbuffer *input = bufferevent_get_input(bev);
        int len = evbuffer_get_length(input);
        qDebug() << "Length:" << len;
        if(len > 0)
        {
            qDebug() << "Draining...";
            evbuffer_drain(input, UINT16_MAX);
            len = evbuffer_get_length(input);
            qDebug() << "Length: " << len;
        }
    }
}
int ServerThreadPrivate::singleMessageIterator(bufferevent *bev, char *buf, uint16_t bufLen, QUuid quuid)
{
    using namespace PXMConsts;
    if(bufLen == 0)
    {
        qCritical() << "Blank message! -- Not Good!";
        return -1;
    }
    MESSAGE_TYPE* type = (MESSAGE_TYPE*)(&buf[0]);
    buf = &buf[sizeof(MESSAGE_TYPE)];
    bufLen -= sizeof(MESSAGE_TYPE);
    switch (*type)
    {
    case MSG_TEXT:
        qInfo().noquote() << "MSG :" << QString::fromUtf8(&buf[0], bufLen);
        emit q_ptr->messageRecieved(QString::fromUtf8(&buf[0], bufLen), quuid, bev, false);
        break;
    case MSG_SYNC:
    {
        char *ipHeapArray = new char[bufLen];
        memcpy(ipHeapArray, &buf[0], bufLen);
        QByteArray qba = QByteArray::fromRawData(&buf[0], bufLen);
        qInfo().noquote() << "SYNC received";
        emit q_ptr->syncPacketIterator(ipHeapArray, bufLen, quuid);
        break;
    }
    case MSG_SYNC_REQUEST:
        qInfo().noquote() << "SYNC_REQUEST received" << QString::fromUtf8(&buf[0], bufLen);
        emit q_ptr->sendSyncPacket(bev, quuid);
        break;
    case MSG_GLOBAL:
        qInfo().noquote() << "GLOBAL :" << QString::fromUtf8(&buf[0], bufLen);
        emit q_ptr->messageRecieved(QString::fromUtf8(&buf[0], bufLen), quuid, bev, true);
        break;
    case MSG_NAME:
        qInfo().noquote() << "NAME :" << QString::fromUtf8(&buf[0], bufLen);
        emit q_ptr->nameChange(QString::fromUtf8(&buf[0], bufLen), quuid);
        break;
    case MSG_AUTH:
        qInfo().noquote() << "AUTH packet recieved after alread authenticated, disregarding...";
        return -1;
        break;
    default:
        qInfo().noquote() << "Bad message type in the packet, discarding the rest";
        return -1;
        break;
    }
    return 0;
}
void ServerThread::udpRecieve(evutil_socket_t socketfd, short int, void *args)
{
    ServerThread *realServer = static_cast<ServerThread*>(args);
    sockaddr_in si_other;
    socklen_t si_other_len = sizeof(sockaddr);
    char buf[500] = {};

    recvfrom(socketfd, buf, sizeof(buf)-1, 0, (sockaddr *)&si_other, &si_other_len);

    if (strncmp(&buf[0], "/discover", 9) == 0)
    {
        qDebug() << "Discovery Packet:" << buf;
        if(!realServer->d_ptr->gotDiscover)
        {
            realServer->d_ptr->gotDiscover = true;
            realServer->multicastIsFunctional();
        }
        evutil_socket_t replySocket;
        if ( (replySocket = (socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))) < 0)
        {
            qCritical().noquote() << "socket: " + QString::fromUtf8(strerror(errno));
            qCritical().noquote() << "Reply to /discover packet not sent";
            return;
        }

        si_other.sin_port = htons(realServer->d_ptr->udpListenerPort);

        int len = sizeof(uint16_t) + UUIDCompression::PACKED_UUID_LENGTH + strlen("/name:");

        char name[len+1];

        strcpy(name, "/name:");

        uint16_t port = htons(realServer->d_ptr->tcpListenerPort);
        memcpy(&name[strlen("/name:")], &(port), sizeof(port));
        UUIDCompression::packUUID(&name[strlen("/name:") + sizeof(port)], realServer->d_ptr->localUUID);

        name[len+1] = 0;

        for(int k = 0; k < 2; k++)
        {
            if(sendto(replySocket, name, len, 0, (sockaddr *)&si_other, si_other_len) != len)
                qCritical().noquote() << "sendto: " + QString::fromUtf8(strerror(errno));
        }
        evutil_closesocket(replySocket);
    }
    else if ((strncmp(&buf[0], "/name:", 6)) == 0)
    {
        memcpy(&si_other.sin_port, &buf[6], sizeof(uint16_t));
        QUuid uuid = UUIDCompression::unpackUUID((unsigned char*)&buf[8]);
        qDebug() << "Name Packet:" << inet_ntoa(si_other.sin_addr) << ":" << ntohs(si_other.sin_port) << "with id:" << uuid.toString();
        realServer->attemptConnection(si_other, uuid);
    }
    else
    {
        qWarning() << "Bad udp packet!";
        return;
    }

    return;
}
unsigned short ServerThreadPrivate::getPortNumber(evutil_socket_t socket)
{
    sockaddr_in needPortNumber;
    memset(&needPortNumber, 0, sizeof(needPortNumber));
    socklen_t needPortNumberLen = sizeof(needPortNumber);
    if(getsockname(socket, (struct sockaddr*)&needPortNumber, &needPortNumberLen) != 0)
    {
        qDebug().noquote() << "getsockname: " + QString::fromUtf8(strerror(errno));
        return -1;
    }
    return ntohs(needPortNumber.sin_port);
}
evutil_socket_t ServerThreadPrivate::setupUDPSocket(evutil_socket_t s_listen)
{
    sockaddr_in si_me;
    ip_mreq multicastGroup;
    int udpSocketNumber;

    evutil_socket_t socketUDP = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    multicastGroup.imr_multiaddr = multicastAddress;
    multicastGroup.imr_interface.s_addr = INADDR_ANY;
    if(setsockopt(socketUDP, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&multicastGroup, sizeof(multicastGroup)) < 0)
    {
        qCritical().noquote() << "setsockopt: " + QString::fromUtf8(strerror(errno));
        evutil_closesocket(socketUDP);
        return -1;
    }

    memset(&si_me, 0, sizeof(si_me));
    si_me.sin_family = AF_INET;
    si_me.sin_port = htons(udpListenerPort);
    //si_me.sin_addr.s_addr = inet_addr(MULTICAST_ADDRESS);
    si_me.sin_addr.s_addr = INADDR_ANY;

    if(setsockopt(socketUDP, SOL_SOCKET, SO_REUSEADDR, "true", sizeof(int)) < 0)
    {
        qCritical().noquote() << "setsockopt: " + QString::fromUtf8(strerror(errno));
        evutil_closesocket(socketUDP);
        return -1;
    }

    evutil_make_socket_nonblocking(socketUDP);

    if(bind(socketUDP, (sockaddr *)&si_me, sizeof(sockaddr)))
    {
        qCritical().noquote() << "bind: " + QString::fromUtf8(strerror(errno));
        evutil_closesocket(socketUDP);
        return -1;
    }

    tcpListenerPort = getPortNumber(s_listen);

    udpSocketNumber = getPortNumber(socketUDP);

    emit q_ptr->setListenerPorts(tcpListenerPort, udpSocketNumber);

    qInfo().noquote() << "Port number for Multicast: " + QString::number(udpSocketNumber);

    //send our discover packet to find other computers
    emit q_ptr->sendUDP("/discover", udpSocketNumber);

    return socketUDP;
}
evutil_socket_t ServerThreadPrivate::setupTCPSocket()
{
    struct addrinfo hints, *res;
    QVector<evutil_socket_t> tcpSockets;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    char tcpPortChar[6] = {};
    sprintf(tcpPortChar, "%d", tcpListenerPort);

    if(getaddrinfo(NULL, tcpPortChar, &hints, &res) < 0)
    {
        qCritical().noquote() << "getaddrinfo: " + QString::fromUtf8(strerror(errno));
        return -1;
    }

    for(addrinfo *p = res; p; p = p->ai_next)
    {
        evutil_socket_t socketTCP;
        if((socketTCP = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) < 0)
        {
            qCritical().noquote() << "socket: " + QString::fromUtf8(strerror(errno));
        }
        if(setsockopt(socketTCP, SOL_SOCKET,SO_REUSEADDR, "true", sizeof(int)) < 0)
        {
            qCritical().noquote() << "setsockopt: " + QString::fromUtf8(strerror(errno));
        }

        evutil_make_socket_nonblocking(socketTCP);

        if(bind(socketTCP, res->ai_addr, res->ai_addrlen) < 0)
        {
            qCritical().noquote() << "bind: " + QString::fromUtf8(strerror(errno));
        }
        if(listen(socketTCP, SOMAXCONN) < 0)
        {
            qCritical().noquote() << "listen: " + QString::fromUtf8(strerror(errno));
        }
        tcpSockets.append(socketTCP);
        qInfo().noquote() << "Port number for TCP/IP Listener: " + QString::number(getPortNumber(socketTCP));
    }
    qInfo().noquote() << "Number of tcp sockets:" << tcpSockets.length();

    freeaddrinfo(res);

    return tcpSockets.first();
}
void ServerThread::stopLoopBufferevent(bufferevent* bev, void*)
{
    char readBev[6] = {};
    bufferevent_read(bev, readBev, 5);
    if(!strncmp(readBev, "/exit", 5))
        event_base_loopexit(ServerThread::base, NULL);
    else
        bufferevent_flush(bev, EV_READ, BEV_FLUSH);
}

void ServerThread::run()
{
    evutil_socket_t s_discover, s_listen;
    struct event *eventAccept, *eventDiscover;
    int failureCodes;
#ifdef _WIN32
    evthread_use_windows_threads();
#else
    evthread_use_pthreads();
#endif

    base = event_base_new();

    if(!base)
    {
        qCritical() << "FATAL:event_base_new returned NULL";
        serverSetupFailure();
        return;
    }

    qInfo().noquote() << "Using " + QString::fromUtf8(event_base_get_method(base)) + " as the libevent backend";
    emit libeventBackend(QString::fromUtf8(event_base_get_method(base)));

    //Pair for self communication
    struct bufferevent *selfCommsPair[2];
    bufferevent_pair_new(base, BEV_OPT_THREADSAFE, selfCommsPair);
    bufferevent_setcb(selfCommsPair[0], ServerThread::tcpRead, NULL, ServerThread::tcpError, this);
    bufferevent_setwatermark(selfCommsPair[0], EV_READ, 2, sizeof(uint16_t));
    bufferevent_enable(selfCommsPair[0], EV_READ);
    bufferevent_enable(selfCommsPair[1], EV_WRITE);

    emit setSelfCommsBufferevent(selfCommsPair[1]);

    //TCP STUFF
    s_listen = d_ptr->setupTCPSocket();
    if(s_listen < 0)
    {
        qCritical() << "FATAL:TCP socket setup has failed";
        serverSetupFailure();
        return;
    }

    //UDP STUFF
    s_discover = d_ptr->setupUDPSocket(s_listen);
    if(s_discover < 0)
    {
        qCritical() << "FATAL:UDP socket setup has failed";
        serverSetupFailure();
        return;
    }

    eventDiscover = event_new(base, s_discover, EV_READ|EV_PERSIST, udpRecieve, this);
    if(!eventDiscover)
    {
        qCritical() << "FATAL:event_new returned NULL";
        serverSetupFailure();
        return;
    }

    failureCodes = event_add(eventDiscover, NULL);
    if(failureCodes < 0)
    {
        qCritical() << "FATAL:event_add returned -1";
        serverSetupFailure();
        return;
    }

    eventAccept = event_new(base, s_listen, EV_READ|EV_PERSIST, accept_new, this);
    if(!eventAccept)
    {
        qCritical() << "FATAL:event_new returned NULL";
        serverSetupFailure();
        return;
    }

    failureCodes = event_add(eventAccept, NULL);
    if(failureCodes < 0)
    {
        qCritical() << "FATAL:event_add returned -1";
        serverSetupFailure();
        return;
    }

    //Pair to shutdown server
    struct bufferevent *closePair[2];
    bufferevent_pair_new(base, BEV_OPT_THREADSAFE, closePair);
    bufferevent_setcb(closePair[0], ServerThread::stopLoopBufferevent, NULL, NULL, NULL);
    bufferevent_enable(closePair[0], EV_READ);
    bufferevent_enable(closePair[1], EV_WRITE);

    emit setCloseBufferevent(closePair[1]);

    failureCodes = event_base_dispatch(ServerThread::base);
    if(failureCodes < 0)
    {
        qWarning() << "event_base_dispatch shutdown with error";
    }

    qDebug() << "Freeing events...";
    event_free(eventAccept);
    event_free(eventDiscover);

    bufferevent_free(closePair[1]);
    bufferevent_free(closePair[0]);

    bufferevent_free(selfCommsPair[0]);
    //bufferevent_free(selfCommsPair[1]);

    event_base_free(base);
    base = 0;

    qDebug() << "Events free, returning from PXMServer::run()";
}
