#include <pxmserver.h>

PXMServer::PXMServer(QWidget *parent, unsigned short tcpPort, unsigned short udpPort) : QThread(parent)
{
    tcpListenerPort = tcpPort;
    udpListenerPort = udpPort;
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
/**
 * @brief				Start of thread, call the listener function which is an infinite loop
 */
void PXMServer::run()
{
    this->listener();
}
/**
 * @brief 				Accept a new TCP connection from the listener socket and let the GUI know.
 * @param s				Listener socket on from which we accept a new connection
 * @param their_addr
 * @return 				Return socket descriptor for new connection, -1 on error on linux, INVALID_SOCKET on Windows
 */
void PXMServer::accept_new(evutil_socket_t s, short event, void *arg)
{
    evutil_socket_t result;

    PXMServer *realServer = static_cast<PXMServer*>(arg);

    struct event_base *base = realServer->base;
    struct sockaddr_in ss;
    socklen_t addr_size = sizeof(ss);

    result = (accept(s, (struct sockaddr *)&ss, &addr_size));
    if(result < 0)
    {
        perror("accept");
    }
    else
    {
        struct bufferevent *bev;
        evutil_make_socket_nonblocking(result);
        bev = bufferevent_socket_new(base, result, BEV_OPT_CLOSE_ON_FREE );
        bufferevent_setcb(bev, tcpReadUUID, NULL, tcpError, (void*)realServer);
        bufferevent_setwatermark(bev, EV_READ, 55, TCP_BUFFER_WATERMARK);
        bufferevent_enable(bev, EV_READ);
        realServer->newConnectionRecieved(result);
    }
}
void PXMServer::tcpReadUUID(struct bufferevent *bev, void *arg)
{
    PXMServer *realServer = static_cast<PXMServer*>(arg);

    char len[6] = {};
    bufferevent_read(bev, &len, 5);
    ev_uint32_t bufLen = atoi(len);
    evutil_socket_t socket = bufferevent_getfd(bev);

    char bufUUID[38];
    if(bufferevent_read(bev, bufUUID, 38) < 38)
    {
        evutil_closesocket(socket);
        return;
    }
    QUuid quuid = QString::fromLatin1(bufUUID, 38);
    if(quuid.isNull())
    {
        evutil_closesocket(socket);
        return;
    }

    char buf[bufLen-38];
    bufLen = bufferevent_read(bev, buf, bufLen-38);
    if(!strncmp(buf, "/uuid", 5))
    {
        bufferevent_setcb(bev, realServer->tcpRead, NULL, realServer->tcpError, (void*)realServer);
        bufferevent_setwatermark(bev, EV_READ, 5, 5);
        qDebug() << "uuid recieved: " + quuid.toString();
        QStringList hpsplit = (QString::fromLatin1(buf+5, bufLen-5)).split(":");
        realServer->recievedUUIDForConnection(hpsplit[0], hpsplit[1], socket, quuid, bev);
    }
    else
    {
        evutil_closesocket(socket);
    }
}
//void PXMServer::tcpReadComplete(evutil_socket_t socket, )

void PXMServer::tcpRead(struct bufferevent *bev, void *arg)
{
    PXMServer *realServer = static_cast<PXMServer*>(arg);
    char *end;
    char len[6] = {};
    if(evbuffer_get_length(bufferevent_get_input(bev)) <= 5)
    {
        evbuffer_copyout(bufferevent_get_input(bev), len, 5);
        long int watermark = strtol(len, &end, 10);
        if(watermark == 0L)
        {
            qDebug() << "Bad buffer length, draining...";
            evbuffer_drain(bufferevent_get_input(bev), TCP_BUFFER_WATERMARK);
            return;
        }
        bufferevent_setwatermark(bev, EV_READ, watermark + 5, watermark + 5);
        return;
    }
    evutil_socket_t i = bufferevent_getfd(bev);
    bufferevent_read(bev, &len, 5);

    long int bufLen = strtol(len, &end, 10);
    int evbufLen = evbuffer_get_length(bufferevent_get_input(bev));
    if(evbufLen != bufLen)
    {
        qDebug() << "Bad buffer length, draining...";
        evbuffer_drain(bufferevent_get_input(bev), TCP_BUFFER_WATERMARK);
        bufferevent_setwatermark(bev, EV_READ, 5, 5);
        return;
    }

    int bytesRead;
    char *buf = new char[bufLen];
    bytesRead = bufferevent_read(bev, buf, bufLen);
    qDebug() << bytesRead << "/" << bufLen;
    /*
    int count = 0;
    int newBytesRead = 0;
    //This works but CANNOT be good for the event loop
    while((bytesRead < bufLen) && count < 10)
    {
        QMessageBox msgBox;
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setText("MESSAGE ERROR.");
        msgBox.exec();
        qDebug() << "Message length warning " << bytesRead << "/" << bufLen;
        count++;
        newBytesRead = recv(i, buf+bytesRead, bufLen - bytesRead, 0);
        qDebug() << "More here:" << QString::fromUtf8(buf+bytesRead, newBytesRead) << newBytesRead;
        bytesRead += newBytesRead;
    }
    */
    realServer->singleMessageIterator(i, buf, bufLen, bev);
    delete [] buf;
    bufferevent_setwatermark(bev, EV_READ, 5, 5);
}

void PXMServer::tcpError(struct bufferevent *buf, short error, void *arg)
{
    PXMServer *realServer = static_cast<PXMServer*>(arg);
    evutil_socket_t i = bufferevent_getfd(buf);
    if (error & BEV_EVENT_EOF)
    {
        realServer->peerQuit(i);
    }
    else if (error & BEV_EVENT_ERROR)
    {
        realServer->peerQuit(i);
    }
    else if (error & BEV_EVENT_TIMEOUT)
    {

    }
    bufferevent_free(buf);
}
int PXMServer::singleMessageIterator(evutil_socket_t socket, char *buf, long int bufLen, bufferevent *bev)
{
    //partialMsg points to the start of the original buffer.
    //If there is more than one message on it, it will be increased to where the
    //next message begins

    //The first four characters of each message should be the length of the message.
    //We parse this to an integer so as not to confuse messages with one another
    //when more than one are recieved from the same socket at the same time
    //If we encounter a valid character after this length, we come back here
    //and iterate through the if statements again.

    if(bufLen <= 38)
        return 1;
    bufLen -= 38;
    QUuid quuid = QString::fromLatin1(buf, 38);
    if(quuid.isNull())
        return -1;
    buf += 38;

    //These packets should be formatted like "/msghostname: msg\0"
    if( !( strncmp(buf, "/msg", 4) ) )
    {
        //Copy the string in buf starting at buf[4] until we reach the length of the message
        //We dont want to keep the "/msg" substring, hence the +/- 4 here.
        //Remember, we added 3 to buf above to ignore the size of the message
        //buf is actually at buf[7] from its original location

        //The following signal is going to the main thread and will call the slot prints(QString, QString)
        emit messageRecieved(QString::fromLatin1(buf+4, bufLen-4), quuid, socket, false);
    }
    //These packets should come formatted like "/ip:hostname@192.168.1.1:hostname2@192.168.1.2\0"
    else if( !( strncmp(buf, "/ip", 3) ) )
    {
        //qDebug() << "/ip recieved from " << QString::number(i);
        //int count = 0;
        emit hostnameCheck(QString::fromLatin1(buf+3, bufLen-3), quuid);
    }
    //This packet is asking us to communicate our list of peers with the sender, leads to us sending an /ip packet
    //These packets should come formatted like "/request\0"
    else if(!(strncmp(buf, "/request", 8)))
    {
        //The int here is the socketdescriptor we want to send our ip set too.
        emit sendIps(socket);
    }
    //These packets are messages sent to the global chat room
    //These packets should come formatted like "/globalhostname: msg\0"
    else if(!(strncmp(buf, "/global", 7)))
    {
        //bufLen-6 instead of 7 because we need a trailing NULL character for QString conversion
        emit messageRecieved(QString::fromLatin1(buf+7, bufLen -7), quuid, socket, true);
    }
    //This packet is an updated hostname for the computer that sent it
    //These packets should come formatted like "/hostnameHostname1\0"
    else if(!(strncmp(buf, "/hostname", 9)))
    {
        qDebug() << "/hostname recieved" << QString::number(socket);
        //bufLen-8 instead of 9 because we need a trailing NULL character for QString conversion
        emit setPeerHostname(QString::fromLatin1(buf+9, bufLen-9), quuid);
    }
    //This packet is asking us to communicate an updated hostname to the sender
    //These packets should come formatted like "/namerequest\0"
    else if(!(strncmp(buf, "/namerequest", 12)))
    {
        qDebug() << "/namerequest recieved from " << QString::number(socket) << "\nsending hostname";
        emit sendName(socket, quuid.toString(), localUUID);
    }
    else
    {
        qDebug() << "Bad message type in the packet, discarding the rest";
        return -1;
    }

    return 0;
}
/**
 * @brief 				UDP packet recieved. Remember, these are connectionless
 * @param i				Socket descriptor to recieve UDP packet from
 * @return 				0 on success, 1 on error
 */
void PXMServer::udpRecieve(evutil_socket_t socketfd, short int event, void *args)
{
    PXMServer *realServer = (PXMServer*)args;
    sockaddr_in si_other;
    socklen_t si_other_len = sizeof(sockaddr);
    char buf[100] = {};

    recvfrom(socketfd, buf, sizeof(buf)-1, 0, (sockaddr *)&si_other, &si_other_len);

    if (strncmp(buf, "/discover", 9) == 0)
    {
        evutil_socket_t replySocket;
        if ( (replySocket = (socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))) < 0)
            perror("socket:");
        si_other.sin_port = htons(realServer->udpListenerPort);

        char name[QString::number(realServer->tcpListenerPort).length() + realServer->localUUID.length() + 7] = {};

        strcpy(name, "/name:");
        snprintf(&name[6], sizeof(name), "%d", realServer->tcpListenerPort);
        strcat(name, realServer->localUUID.toStdString().c_str());

        int len = strlen(name);

        for(int k = 0; k < 2; k++)
        {
            sendto(replySocket, name, len+1, 0, (struct sockaddr *)&si_other, si_other_len);
        }
        evutil_closesocket(replySocket);
    }
    //This will get sent from anyone recieving a /discover packet
    //when this is recieved it add the sender to the list of peers and connects to him
    else if ((strncmp(buf, "/name:", 6)) == 0)
    {
        QString fullidStr = QString::fromLatin1(&buf[6]);
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
        printf("getsockname: %s\n", strerror(errno));
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

    setsockopt(socketUDP, SOL_SOCKET, SO_REUSEADDR, "true", sizeof(int));
    evutil_make_socket_nonblocking(socketUDP);

    if(bind(socketUDP, (sockaddr *)&si_me, sizeof(sockaddr)))
    {
        perror("Binding datagram socket error");
        close(socketUDP);
        exit(1);
    }

    tcpListenerPort = getPortNumber(s_listen);

    emit setListenerPort(tcpListenerPort);

    udpSocketNumber = getPortNumber(socketUDP);

    qDebug() << "Port number for Multicast: " << udpSocketNumber;

    multicastGroup.imr_multiaddr.s_addr = inet_addr(MULTICAST_ADDRESS);
    multicastGroup.imr_interface.s_addr = htonl(INADDR_ANY);
    setsockopt(socketUDP, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&multicastGroup, sizeof(multicastGroup));

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

    //getaddrinfo(NULL, ourListenerPort.toStdString().c_str(), &hints, &res);
    getaddrinfo(NULL, QString::number(tcpListenerPort).toStdString().c_str(), &hints, &res);

    if((socketTCP = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) < 0)
    {
        perror("socket error: ");
    }
    if(setsockopt(socketTCP, SOL_SOCKET,SO_REUSEADDR, "true", sizeof(int)) < 0)
    {
        perror("setsockopt error: ");
    }

    evutil_make_socket_nonblocking(socketTCP);

    if(bind(socketTCP, res->ai_addr, res->ai_addrlen) < 0)
    {
        perror("bind error: ");
    }
    if(listen(socketTCP, BACKLOG) < 0)
    {
        perror("listen error: ");
    }
    qDebug() << "Port number for TCP/IP Listner" << getPortNumber(socketTCP);

    freeaddrinfo(res);

    return socketTCP;
}

/**
 * @brief 				Main listener called from the run function.  Infinite while loop in here that is interuppted by
 *						the GUI thread upon shutdown.  Two listeners, one TCP/IP and one UDP, are created here and checked
 *						for new connections.  All new connections that come in here are listened to after they have had their
 *						descriptors added to the master FD set.
 * @return				Should never return, -1 means something terrible has happened
 */
int PXMServer::listener()
{
    //Potential rewrite to change getaddrinfo to a manual setup of the socket structures.
    //Changing to manual setup may improve load times on windows systems.  Locks us into
    //ipv4 however.
    evutil_socket_t s_discover, s_listen;
    struct event *eventAccept;
    struct event *eventDiscover;


    base = event_base_new();


    qDebug() << "Using" << QString::fromLatin1(event_base_get_method(base)) << "as the libevent backend";
    if(!base)
        return -1;

    //TCP STUFF
    s_listen = setupTCPSocket();

    //UDP STUFF
    s_discover = setupUDPSocket(s_listen);

    eventDiscover = event_new(base, s_discover, EV_READ|EV_PERSIST, udpRecieve, (void*)this);

    event_add(eventDiscover, NULL);

    eventAccept = event_new(base, s_listen, EV_READ|EV_PERSIST, accept_new, (void*)this);

    event_add(eventAccept, NULL);

    struct timeval halfSec;
    halfSec.tv_sec = 0;
    halfSec.tv_usec = 500000;

    while(!this->isInterruptionRequested())
    {
        event_base_loopexit(base, &halfSec);

        event_base_dispatch(base);
    }

    event_free(eventAccept);
    event_free(eventDiscover);

    event_base_free(base);


    return 0;
}
