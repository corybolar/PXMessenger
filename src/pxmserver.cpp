#include <pxmserver.h>

struct event_base* PXMServer::base;
PXMServer::PXMServer(QWidget *parent, unsigned short tcpPort, unsigned short udpPort) : QThread(parent)
{
    tcpListenerPort = tcpPort;
    udpListenerPort = udpPort;
    this->setObjectName("PXMServer");
}
PXMServer::~PXMServer()
{
}

void PXMServer::setLocalHostname(QString hostname)
{
    localHostname = hostname;
}
void PXMServer::setLocalUUID(QString uuid)
{
    localUUID = uuid;
}
void PXMServer::accept_new(evutil_socket_t s, short, void *arg)
{
    evutil_socket_t result;

    PXMServer *realServer = static_cast<PXMServer*>(arg);

    struct sockaddr_in ss;
    socklen_t addr_size = sizeof(ss);

    result = (accept(s, (struct sockaddr *)&ss, &addr_size));
    //realServer->xdebug(QString::number(FD_SETSIZE));
    if(result < 0)
    {
        realServer->xdebug("accept: " + QString::fromUtf8(strerror(errno)));
    }
    else
    {
        struct bufferevent *bev;
        evutil_make_socket_nonblocking(result);
        bev = bufferevent_socket_new(PXMServer::base, result, 0);
        bufferevent_setcb(bev, PXMServer::tcpReadUUID, NULL, PXMServer::tcpError, (void*)realServer);
        bufferevent_setwatermark(bev, EV_READ, 2, sizeof(uint16_t));
        bufferevent_enable(bev, EV_READ);
        realServer->newTCPConnection(result);
    }
}
void PXMServer::tcpReadUUID(struct bufferevent *bev, void *arg)
{
    PXMServer *realServer = static_cast<PXMServer*>(arg);
    uint16_t nboBufLen;
    uint16_t bufLen;

    if(evbuffer_get_length(bufferevent_get_input(bev)) == sizeof(uint16_t))
    {
        evbuffer_copyout(bufferevent_get_input(bev), &nboBufLen, sizeof(uint16_t));
        bufLen = ntohs(nboBufLen);
        if(bufLen == 0)
        {
            realServer->xdebug("Bad buffer length, draining...");
            evbuffer_drain(bufferevent_get_input(bev), UINT16_MAX);
            return;
        }
        bufferevent_setwatermark(bev, EV_READ, bufLen + sizeof(uint16_t), bufLen + sizeof(uint16_t));
        return;
    }
    bufferevent_read(bev, &nboBufLen, sizeof(uint16_t));

    bufLen = ntohs(nboBufLen);
    evutil_socket_t socket = bufferevent_getfd(bev);

    unsigned char bufUUID[PACKED_UUID_BYTE_LENGTH];
    if(bufferevent_read(bev, bufUUID, PACKED_UUID_BYTE_LENGTH) < PACKED_UUID_BYTE_LENGTH)
    {
        bufferevent_disable(bev, EV_READ|EV_WRITE);
        realServer->peerQuit(socket, (void*)bev);
        return;
    }
    QUuid quuid = PXMServer::unpackUUID(bufUUID);
    bufLen -= PACKED_UUID_BYTE_LENGTH;
    if(quuid.isNull())
    {
        bufferevent_disable(bev, EV_READ|EV_WRITE);
        realServer->peerQuit(socket, (void*)bev);
        return;
    }

    char buf[bufLen + 1];
    bufferevent_read(bev, buf, bufLen);
    buf[bufLen] = 0;
    realServer->xdebug(QString::fromUtf8(buf));
    if(!strncmp(buf, "/uuid", 5))
    {
        QStringList hpsplit = (QString::fromUtf8(buf+5, bufLen-5)).split("::::");
        unsigned short port = hpsplit[1].toUShort();
        if(port == 0)
        {
            bufferevent_disable(bev, EV_READ|EV_WRITE);
            realServer->peerQuit(socket, (void*)bev);
            return;
        }
        realServer->authenticationReceived(hpsplit[0], port, socket, quuid, bev);
        bufferevent_setwatermark(bev, EV_READ, sizeof(uint16_t), sizeof(uint16_t));
        bufferevent_setcb(bev, PXMServer::tcpRead, NULL, PXMServer::tcpError, (void*)realServer);
    }
    else
    {
        evutil_closesocket(socket);
    }
}
void PXMServer::tcpRead(struct bufferevent *bev, void *arg)
{
    PXMServer *realServer = static_cast<PXMServer*>(arg);
    uint16_t nboBufLen;
    uint16_t bufLen;
    evbuffer *input = bufferevent_get_input(bev);
    if(evbuffer_get_length(input) == 1)
    {
        realServer->xdebug("Setting timeout, 1 byte recieved");
        bufferevent_set_timeouts(bev, &readTimeout, NULL);
        return;
    }
    if(evbuffer_get_length(bufferevent_get_input(bev)) <= sizeof(uint16_t))
    {
        realServer->xdebug("Recieved bufferlength value");
        evbuffer_copyout(input, &nboBufLen, sizeof(uint16_t));
        bufLen = ntohs(nboBufLen);
        if(bufLen == 0)
        {
            realServer->xdebug("Bad buffer length, draining...");
            evbuffer_drain(input, UINT16_MAX);
            return;
        }
        realServer->xdebug("Setting watermark to " + QString::number(bufLen));
        bufferevent_setwatermark(bev, EV_READ, bufLen + sizeof(uint16_t), bufLen + sizeof(uint16_t));
        //timeval readTimeout = {1,0};
        realServer->xdebug("Setting timeout after length");
        bufferevent_set_timeouts(bev, &readTimeout, NULL);
        return;
    }
    realServer->xdebug("Full packet received");
    bufferevent_setwatermark(bev, EV_READ, 1, sizeof(uint16_t));

    evutil_socket_t i = bufferevent_getfd(bev);
    bufferevent_read(bev, &nboBufLen, 2);

    bufLen = ntohs(nboBufLen);
    if(bufLen <= PACKED_UUID_BYTE_LENGTH)
    {
        evbuffer_drain(bufferevent_get_input(bev), UINT16_MAX);
        return;
    }

    unsigned char rawUUID[PACKED_UUID_BYTE_LENGTH];
    bufferevent_read(bev, rawUUID, PACKED_UUID_BYTE_LENGTH);

    QUuid uuid = PXMServer::unpackUUID(rawUUID);
    if(uuid.isNull())
    {
        evbuffer_drain(bufferevent_get_input(bev), UINT16_MAX);
        return;
    }
    realServer->xdebug("Sender Uuid for message " + uuid.toString());

    bufLen -= PACKED_UUID_BYTE_LENGTH;
    char *buf = new char[bufLen + 1];
    bufferevent_read(bev, buf, bufLen);
    buf[bufLen] = 0;

    realServer->singleMessageIterator(i, buf, bufLen, uuid);

    timeval readTimeoutReset = {3600, 0};
    bufferevent_set_timeouts(bev, &readTimeoutReset, NULL);
    delete [] buf;
}

void PXMServer::tcpError(struct bufferevent *bev, short error, void *arg)
{
    PXMServer *realServer = static_cast<PXMServer*>(arg);
    evutil_socket_t i = bufferevent_getfd(bev);
    if (error & BEV_EVENT_EOF)
    {
        bufferevent_disable(bev, EV_READ|EV_WRITE);
        realServer->peerQuit(i, (void*)bev);
    }
    else if (error & BEV_EVENT_ERROR)
    {
        bufferevent_disable(bev, EV_READ|EV_WRITE);
        realServer->peerQuit(i, (void*)bev);
    }
    else if (error & BEV_EVENT_TIMEOUT)
    {
        realServer->xdebug("Timeout triggered");
        bufferevent_setwatermark(bev, EV_READ, 1, sizeof(uint16_t));
        timeval readTimeoutReset = {3600, 0};
        bufferevent_set_timeouts(bev, &readTimeoutReset, NULL);
        bufferevent_enable(bev, EV_READ);
        evbuffer *input = bufferevent_get_input(bev);
        int len = evbuffer_get_length(input);
        realServer->xdebug("Length: " + QString::number(len));
        if(len > 0)
        {
            realServer->xdebug("Draining...");
            evbuffer_drain(input, UINT16_MAX);
            len = evbuffer_get_length(input);
            realServer->xdebug("Length: " + QString::number(len));
        }
    }
}
int PXMServer::singleMessageIterator(evutil_socket_t socket, char *buf, uint16_t bufLen, QUuid quuid)
{
    //These packets should be formatted like "/msghostname: msg\0"
    if( !( strncmp(buf, "/msg", 4) ) )
    {
        xdebug("/msg : " + QString::fromUtf8(&buf[4], bufLen-4));
        emit messageRecieved(QString::fromUtf8(&buf[4], bufLen-4), quuid, socket, false);
    }
    //These packets should come formatted like "/ip:hostname@192.168.1.1:hostname2@192.168.1.2\0"
    else if( !( strncmp(buf, "/ip", 3) ) )
    {
        char *ipHeapArray = new char[bufLen-3];
        memcpy(ipHeapArray, &buf[3], bufLen-3);
        xdebug("/ip received");
        emit hostnameCheck(ipHeapArray, bufLen-3, quuid);
    }
    //This packet is asking us to communicate our list of peers with the sender, leads to us sending an /ip packet
    //These packets should come formatted like "/request\0"
    else if(!(strncmp(buf, "/request", 8)))
    {
        xdebug("/request received");
        emit sendIps(socket);
    }
    //These packets are messages sent to the global chat room
    //These packets should come formatted like "/globalhostname: msg\0"
    else if(!(strncmp(buf, "/global", 7)))
    {
        xdebug("/global : " + QString::fromUtf8(&buf[7], bufLen-7));
        emit messageRecieved(QString::fromUtf8(&buf[7], bufLen-7), quuid, socket, true);
    }
    //This packet is an updated hostname for the computer that sent it
    //These packets should come formatted like "/hostnameHostname1\0"
    else if(!(strncmp(buf, "/hostname", 9)))
    {
        xdebug("/hostname received " + QString::number(socket));
        emit setPeerHostname(QString::fromUtf8(&buf[9], bufLen-9), quuid);
    }
    //This packet is asking us to communicate an updated hostname to the sender
    //These packets should come formatted like "/namerequest\0"
    else if(!(strncmp(buf, "/namerequest", 12)))
    {
        xdebug("/namerequest received from " + QString::number(socket) + " sending hostname");
        emit sendName(socket, quuid.toString(), localUUID);
    }
    else
    {
        xdebug("Bad message type in the packet, discarding the rest");
        return -1;
    }

    return 0;
}
QUuid PXMServer::unpackUUID(unsigned char *src)
{
    QUuid uuid;
    size_t index = 0;
    uuid.data1 = ntohl(*((uint32_t*)(src)) );
    index += sizeof(uint32_t);
    uuid.data2 = ntohs(*((uint16_t*)(src+index)) );
    index += sizeof(uint16_t);
    uuid.data3 = ntohs(*((uint16_t*)(src+index)) );
    index += sizeof(uint16_t);
    memcpy(&(uuid.data4), src + index, 8);
    //index += 8;

    return uuid;
}

void PXMServer::udpRecieve(evutil_socket_t socketfd, short int, void *args)
{
    PXMServer *realServer = static_cast<PXMServer*>(args);
    sockaddr_in si_other;
    socklen_t si_other_len = sizeof(sockaddr);
    char buf[100] = {};

    recvfrom(socketfd, buf, sizeof(buf)-1, 0, (sockaddr *)&si_other, &si_other_len);

    if (strncmp(buf, "/discover", 9) == 0)
    {
        evutil_socket_t replySocket;
        if ( (replySocket = (socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))) < 0)
            realServer->xdebug("socket: " + QString::fromUtf8(strerror(errno)));
        si_other.sin_port = htons(realServer->udpListenerPort);

        char name[QString::number(realServer->tcpListenerPort).length() + realServer->localUUID.length() + 7];

        strcpy(name, "/name:");
        snprintf(&name[6], sizeof(name), "%d", realServer->tcpListenerPort);
        strcat(name, realServer->localUUID.toStdString().c_str());

        int len = strlen(name);

        for(int k = 0; k < 2; k++)
        {
            if(sendto(replySocket, name, len+1, 0, (struct sockaddr *)&si_other, si_other_len) != len+1)
                realServer->xdebug("sendto: " + QString::fromUtf8(strerror(errno)));
        }
        evutil_closesocket(replySocket);
    }
    //This will get sent from anyone recieving a /discover packet
    //when this is recieved it add the sender to the list of peers and connects to him
    else if ((strncmp(buf, "/name:", 6)) == 0)
    {
        QString fullidStr = QString::fromUtf8(&buf[6]);
        QString portNumber = fullidStr.left(fullidStr.indexOf("{"));
        QUuid uuid = fullidStr.right(fullidStr.length() - fullidStr.indexOf("{"));

        si_other.sin_port = htons(portNumber.toInt());
        realServer->attemptConnection(si_other, uuid);
    }
    else
    {
        return;
    }

    return;
}
int PXMServer::getPortNumber(evutil_socket_t socket)
{
    sockaddr_in needPortNumber;
    memset(&needPortNumber, 0, sizeof(needPortNumber));
    socklen_t needPortNumberLen = sizeof(needPortNumber);
    if(getsockname(socket, (struct sockaddr*)&needPortNumber, &needPortNumberLen) != 0)
    {
        xdebug("getsockname: " + QString::fromUtf8(strerror(errno)));
        return -1;
    }
    return ntohs(needPortNumber.sin_port);
}
evutil_socket_t PXMServer::setupUDPSocket(evutil_socket_t s_listen)
{
    sockaddr_in si_me;
    ip_mreq multicastGroup;
    int udpSocketNumber;

    evutil_socket_t socketUDP = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    memset(&si_me, 0, sizeof(si_me));
    si_me.sin_family = AF_INET;
    si_me.sin_port = htons(udpListenerPort);
    si_me.sin_addr.s_addr = INADDR_ANY;

    if(setsockopt(socketUDP, SOL_SOCKET, SO_REUSEADDR, "true", sizeof(int)) < 0)
        xdebug("setsockopt: " + QString::fromUtf8(strerror(errno)));

    evutil_make_socket_nonblocking(socketUDP);

    if(bind(socketUDP, (sockaddr *)&si_me, sizeof(sockaddr)))
    {
        xdebug("bind: " + QString::fromUtf8(strerror(errno)));
        close(socketUDP);
        exit(1);
    }

    tcpListenerPort = getPortNumber(s_listen);

    emit setListenerPort(tcpListenerPort);

    udpSocketNumber = getPortNumber(socketUDP);

    xdebug("Port number for Multicast: " + QString::number(udpSocketNumber));

    multicastGroup.imr_multiaddr.s_addr = inet_addr(MULTICAST_ADDRESS);
    multicastGroup.imr_interface.s_addr = htonl(INADDR_ANY);
    if(setsockopt(socketUDP, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&multicastGroup, sizeof(multicastGroup)) < 0)
        xdebug("setsockopt: " + QString::fromUtf8(strerror(errno)));

    //send our discover packet to find other computers
    emit sendUDP("/discover:", udpSocketNumber);

    return socketUDP;
}
evutil_socket_t PXMServer::setupTCPSocket()
{
    struct addrinfo hints, *res;
    evutil_socket_t socketTCP;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    char tcpPortChar[6];
    memset(tcpPortChar, 0, 6);
    sprintf(tcpPortChar, "%d", tcpListenerPort);

    if(getaddrinfo(NULL, tcpPortChar, &hints, &res) < 0)
        xdebug("getaddrinfo: " + QString::fromUtf8(strerror(errno)));
    if((socketTCP = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) < 0)
        xdebug("socket: " + QString::fromUtf8(strerror(errno)));
    if(setsockopt(socketTCP, SOL_SOCKET,SO_REUSEADDR, "true", sizeof(int)) < 0)
        xdebug("setsockopt: " + QString::fromUtf8(strerror(errno)));

    evutil_make_socket_nonblocking(socketTCP);

    if(bind(socketTCP, res->ai_addr, res->ai_addrlen) < 0)
        xdebug("bind: " + QString::fromUtf8(strerror(errno)));
    if(listen(socketTCP, BACKLOG) < 0)
        xdebug("listen: " + QString::fromUtf8(strerror(errno)));

    xdebug("Port number for TCP/IP Listner " + QString::number(getPortNumber(socketTCP)));

    freeaddrinfo(res);

    return socketTCP;
}
void PXMServer::stopLoop(evutil_socket_t, short, void *)
{
    if(QThread::currentThread()->isInterruptionRequested())
    {
        //event_base_loopexit(PXMServer::base, NULL);
    }
}
void PXMServer::stopLoopBufferevent(bufferevent* bev, void*)
{
    char readBev[4] = {};
    bufferevent_read(bev, readBev, 4);
    if(!strncmp(readBev, "exit", 4))
        event_base_loopexit(PXMServer::base, NULL);
    else
        bufferevent_flush(bev, EV_READ, BEV_FLUSH);
}

void PXMServer::run()
{
    evutil_socket_t s_discover, s_listen;
    struct event *eventAccept, *eventDiscover;
#ifdef _WIN32
    evthread_use_windows_threads();
#else
    evthread_use_pthreads();
#endif

    base = event_base_new();

    if(!base)
        return;

    xdebug("Using " + QString::fromUtf8(event_base_get_method(base)) + " as the libevent backend");
    emit libeventBackend(QString::fromUtf8(event_base_get_method(base)));

    //TCP STUFF
    s_listen = setupTCPSocket();

    //UDP STUFF
    s_discover = setupUDPSocket(s_listen);

    eventDiscover = event_new(base, s_discover, EV_READ|EV_PERSIST, udpRecieve, (void*)this);

    event_add(eventDiscover, NULL);

    eventAccept = event_new(base, s_listen, EV_READ|EV_PERSIST, accept_new, (void*)this);

    event_add(eventAccept, NULL);

    //struct timeval halfSecond = {0,500000};

    //eventStopLoop = event_new(base, 0, EV_PERSIST, stopLoop, NULL);
    //event_add(eventStopLoop, &halfSecond);

    struct bufferevent *closePair[2];
    bufferevent_pair_new(base, BEV_OPT_THREADSAFE, closePair);
    //bufferevent_pair_new(base, 0, closePair);
    bufferevent_setcb(closePair[0], PXMServer::stopLoopBufferevent, NULL, NULL, NULL);
    bufferevent_enable(closePair[0], EV_READ);
    bufferevent_enable(closePair[1], EV_WRITE);

    emit setCloseBufferevent((void*)closePair[1]);

    event_base_dispatch(PXMServer::base);

    xdebug("Freeing events...");
    event_free(eventAccept);
    event_free(eventDiscover);
    //event_free(eventStopLoop);

    bufferevent_free(closePair[1]);
    bufferevent_free(closePair[0]);

    event_base_free(base);

    xdebug("Events free");

    return;
}
