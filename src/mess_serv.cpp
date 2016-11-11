#include <mess_serv.h>

MessengerServer::MessengerServer(QWidget *parent, unsigned short tcpPort, unsigned short udpPort) : QThread(parent)
{
    tcpListenerPort = tcpPort;
    udpListenerPort = udpPort;
}
MessengerServer::~MessengerServer()
{
}

void MessengerServer::setLocalHostname(QString hostname)
{
    localHostname = hostname;
}
void MessengerServer::setLocalUUID(QString uuid)
{
    localUUID = uuid;
}
/**
 * @brief				Start of thread, call the listener function which is an infinite loop
 */
void MessengerServer::run()
{
    this->listener();
}
/**
 * @brief 				Accept a new TCP connection from the listener socket and let the GUI know.
 * @param s				Listener socket on from which we accept a new connection
 * @param their_addr
 * @return 				Return socket descriptor for new connection, -1 on error on linux, INVALID_SOCKET on Windows
 */
void MessengerServer::accept_new(evutil_socket_t s, short event, void *arg)
{
    evutil_socket_t result;

    MessengerServer *realServer = static_cast<MessengerServer*>(arg);

    struct event_base *base = realServer->base;
    struct sockaddr_storage ss;
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
        bufferevent_setcb(bev, tcpRead, NULL, tcpError, (void*)realServer);
        bufferevent_setwatermark(bev, EV_READ, 0, TCP_BUFFER_LENGTH);
        bufferevent_enable(bev, EV_READ);
        realServer->newConnectionRecieved(result, bev);
    }
    //emit newConnectionRecieved(result);
}
void MessengerServer::tcpRead(struct bufferevent *bev, void *arg)
{
    MessengerServer *realServer = static_cast<MessengerServer*>(arg);
    evutil_socket_t i = bufferevent_getfd(bev);
    while(evbuffer_get_length(bufferevent_get_input(bev)) > 0)
    {
        char len[4] = {};
        bufferevent_read(bev, &len, 4);

        ev_uint32_t bufLen = atoi(len);

        char *buf = new char[bufLen];
        bufferevent_read(bev, buf, bufLen);
        realServer->singleMessageIterator(i, buf, bufLen, bev);
        delete [] buf;
    }
}

void MessengerServer::tcpError(struct bufferevent *buf, short error, void *arg)
{
    MessengerServer *realServer = static_cast<MessengerServer*>(arg);
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
int MessengerServer::singleMessageIterator(evutil_socket_t i, char *buf, ev_uint32_t bufLen, bufferevent *bev)
{
    //partialMsg points to the start of the original buffer.
    //If there is more than one message on it, it will be increased to where the
    //next message begins
    char *partialMsg = buf;

    //This holds the first 3 characters of partialMsg which will represent how long the recieved message should be.
    //char bufLenStr[5] = {};

    //The first three characters of each message should be the length of the message.
    //We parse this to an integer so as not to confuse messages with one another
    //when more than one are recieved from the same socket at the same time
    //If we encounter a valid character after this length, we come back here
    //and iterate through the if statements again.
    //strncpy(bufLenStr, partialMsg, 4);

    //int bufLen = atoi(bufLenStr);
    if(bufLen <= 38)
        return 1;
    bufLen -= 38;
    //partialMsg = partialMsg+4;
    //skip over the UUID, not checking this yet, initial stages
    //FIX THIS AFTER MORE FUNCTIONALITY
    QUuid quuid = QString::fromUtf8(partialMsg, 38);
    if(quuid.isNull())
        return -1;
    partialMsg += 38;


    //These packets should be formatted like "/msghostname: msg\0"
    if( !( strncmp(partialMsg, "/msg", 4) ) )
    {
        //Copy the string in buf starting at buf[4] until we reach the length of the message
        //We dont want to keep the "/msg" substring, hence the +/- 4 here.
        //Remember, we added 3 to buf above to ignore the size of the message
        //buf is actually at buf[7] from its original location

        //The following signal is going to the main thread and will call the slot prints(QString, QString)
        emit messageRecieved(QString::fromUtf8(partialMsg+4, bufLen-4), quuid, false);
    }
    //These packets should come formatted like "/ip:hostname@192.168.1.1:hostname2@192.168.1.2\0"
    else if( !( strncmp(partialMsg, "/ip", 3) ) )
    {
        qDebug() << "/ip recieved from " << QString::number(i);
        int count = 0;

        //We need to extract each hostname and ip set out of the message and send them to the main thread
        //to check if whether we already know about them or not
        for(unsigned int k = 4; k < bufLen; k++)
        {
            //Theres probably a better way to do this
            if(partialMsg[k] == '[' || partialMsg[k] == ']')
            {
                char temp[INET_ADDRSTRLEN + 5 + 38] = {};
                strncpy(temp, partialMsg+(k-count), count);
                count = 0;
                if((strlen(temp) < 2))
                    *temp = 0;
                else
                    //The following signal is going to the main thread and will call the slot ipCheck(QString)
                    emit hostnameCheck(QString::fromUtf8(temp), quuid);
            }
            else
                count++;
        }
    }
    else if(!(strncmp(partialMsg, "/uuid", 5)))
    {
        qDebug() << "uuid recieved: " + quuid.toString();
        QString hostandport = QString::fromUtf8(partialMsg+5, bufLen-5);
        QStringList hpsplit = hostandport.split(":");
        emit recievedUUIDForConnection(hpsplit[0], hpsplit[1], i, quuid, bev);
    }
    //This packet is asking us to communicate our list of peers with the sender, leads to us sending an /ip packet
    //These packets should come formatted like "/request\0"
    else if(!(strncmp(partialMsg, "/request", 8)))
    {
        qDebug() << "/request recieved from " << QString::number(i) << "\nsending ips";
        //The following signal is going to the m_client object and thread and will call the slot sendIps(int)
        //The int here is the socketdescriptor we want to send our ip set too.
        emit sendIps(i);
    }
    //These packets are messages sent to the global chat room
    //These packets should come formatted like "/globalhostname: msg\0"
    else if(!(strncmp(partialMsg, "/global", 7)))
    {
        //bufLen-6 instead of 7 because we need a trailing NULL character for QString conversion
        char emitStr[bufLen-6] = {};
        strncpy(emitStr, (partialMsg+7), bufLen-7);
        emit messageRecieved(QString::fromUtf8(emitStr), quuid, true);
    }
    //This packet is an updated hostname for the computer that sent it
    //These packets should come formatted like "/hostnameHostname1\0"
    else if(!(strncmp(partialMsg, "/hostname", 9)))
    {
        qDebug() << "/hostname recieved" << QString::number(i);
        //bufLen-8 instead of 9 because we need a trailing NULL character for QString conversion
        char emitStr[bufLen-8] = {};
        strncpy(emitStr, (partialMsg+9), bufLen-9);
        emit setPeerHostname(QString::fromUtf8(emitStr), quuid);
    }
    //This packet is asking us to communicate an updated hostname to the sender
    //These packets should come formatted like "/namerequest\0"
    else if(!(strncmp(partialMsg, "/namerequest", 12)))
    {
        qDebug() << "/namerequest recieved from " << QString::number(i) << "\nsending hostname";
        emit sendName(i, quuid.toString(), localUUID);
    }
    else
    {
        qDebug() << "Bad message type in the packet, discarding the rest";
        return -1;
    }

    //Check if theres more in the buffer
    //if(partialMsg[bufLen] != '\0')
    //{
    //partialMsg += bufLen;
    //this->singleMessageIterator(i, partialMsg, bev);
    //}
    return 0;
}
/**
 * @brief 				UDP packet recieved. Remember, these are connectionless
 * @param i				Socket descriptor to recieve UDP packet from
 * @return 				0 on success, 1 on error
 */
void MessengerServer::udpRecieve(evutil_socket_t socketfd, short int event, void *args)
{
    MessengerServer *realServer = (MessengerServer*)args;
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
        QString fullidStr = QString::fromUtf8(&buf[6]);
        QString portNumber = fullidStr.left(fullidStr.indexOf("{"));
        QString uuid = fullidStr.right(fullidStr.length() - fullidStr.indexOf("{"));
        QString ipAddress = QString::fromUtf8(inet_ntoa(si_other.sin_addr));

        realServer->updNameRecieved(portNumber, ipAddress, uuid);
    }
    else
    {
        return;
    }

    return;
}
int MessengerServer::getPortNumber(evutil_socket_t socket)
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
evutil_socket_t MessengerServer::setupUDPSocket(evutil_socket_t s_listen)
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
    emit sendUdp("/discover:" + QString::number(tcpListenerPort), udpSocketNumber);

    return socketUDP;
}
evutil_socket_t MessengerServer::setupTCPSocket()
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
int MessengerServer::setSocketToNonBlocking(evutil_socket_t socket)
{
#ifdef _WIN32
    unsigned long ul = 1;
    ioctlsocket(socket, FIONBIO, &ul);
    return 0;
#else
    int flags;
    flags = fcntl(socket, F_GETFL);
    if(flags < 0)
        return flags;
    flags |= O_NONBLOCK;
    if(fcntl(socket, F_SETFL, flags) < 0)
        return -1;
    return 0;
#endif
}
/**
 * @brief 				Main listener called from the run function.  Infinite while loop in here that is interuppted by
 *						the GUI thread upon shutdown.  Two listeners, one TCP/IP and one UDP, are created here and checked
 *						for new connections.  All new connections that come in here are listened to after they have had their
 *						descriptors added to the master FD set.
 * @return				Should never return, -1 means something terrible has happened
 */
int MessengerServer::listener()
{
    //Potential rewrite to change getaddrinfo to a manual setup of the socket structures.
    //Changing to manual setup may improve load times on windows systems.  Locks us into
    //ipv4 however.
    evutil_socket_t s_discover, s_listen;
    struct event *eventAccept;
    struct event *eventDiscover;


    base = event_base_new();


    qDebug() << "Using" << QString::fromUtf8(event_base_get_method(base)) << "as the libevent backend";
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
