#include <mess_serv.h>

mess_serv::mess_serv(QWidget *parent) : QThread(parent)
{


}

/**
 * @brief				Start of thread, call the listener function which is an infinite loop
 */
void mess_serv::run()
{
    this->listener();
}

/**
 * @brief 				Accept a new TCP connection from the listener socket and let the GUI know.
 * @param s				Listener socket on from which we accept a new connection
 * @param their_addr
 * @return 				Return socket descriptor for new connection, -1 on error on linux, INVALID_SOCKET on Windows
 */
int mess_serv::accept_new(int s, sockaddr_storage *their_addr)
{
    int result;
    char ipstr[INET6_ADDRSTRLEN];
    char service[20];

    socklen_t addr_size = sizeof(sockaddr);
    result = (accept(s, (struct sockaddr *)&their_addr, &addr_size));
    //inet_ntop(AF_INET, &(((struct sockaddr_in*)&their_addr)->sin_addr), ipstr, sizeof(ipstr));
    getnameinfo(((struct sockaddr*)&their_addr), addr_size, ipstr, sizeof(ipstr), service, sizeof(service), NI_NUMERICHOST);
    emit new_client(result, QString::fromUtf8(ipstr, strlen(ipstr)));
    this->update_fds(result);
    return result;
}

/**
 * @brief 				Add an FD to the set if its not already in there and check if its the new max
 * @param s				Socket descriptor number to add to set
 */
void mess_serv::update_fds(int s)
{
    if( !( FD_ISSET(s, &master) ) )
    {
        FD_SET(s, &master);
        this->set_fdmax(s);
    }
}

/**
 * @brief 				Determine if a new socket descriptor is the highest in the set, adjust the fd_max variable accordingly.
 * @param s				Socket descriptor to potentially make the new max.  fd_max is needed for select in the listener function
 * @return 				0 if it is the new max, -1 if it is not;
 */
int mess_serv::set_fdmax(int s)
{
    if(s > fdmax)
    {
        fdmax = s;
        return 0;
    }
    return -1;
}

/**
 * @brief 				Called when the TCP listener recieves a new connection
 * @param i				Socket descriptor of new connection
 * @return 				0 on success, 1 on error
 */
int mess_serv::newConnection(int i)
{
#ifdef _WIN32
    unsigned new_fd;
#else
    int new_fd;
#endif
    struct sockaddr_storage their_addr;

    new_fd = accept_new(i, &their_addr);

#ifdef __unix__
    if(new_fd == -1)
    {
        perror("accept:");
        return 1;
    }
#endif
#ifdef _WIN32
    if(new_fd == INVALID_SOCKET)
    {
        perror("accept:");
        return 1;
    }
#endif
    else
    {
        this->update_fds(new_fd);
        std::cout << "new connection" << std::endl;
        return 0;
    }
    return 1;
}

/**
 * @brief 				Message recieved from a previously connected TCP socket
 * @param i				Socket descriptor to recieve data from
 * @return 				0 on success, 1 on error
 */
int mess_serv::tcpRecieve(int i)
{
    int nbytes;
    char buf[1050] = {};

    if((nbytes = recv(i,buf,sizeof(buf), 0)) <= 0)
    {
        //The tcp socket is telling us it has been closed
        //If nbytes == 0, normal close
        if(nbytes == 0)
        {
            std::cout << "connection closed" << std::endl;
        }
        //Error
        else
        {
            perror("recv");
        }
        //Either way, alert the main thread this socket has closed.  It will update the peer_class
        //and will make the list item italics to signal a disconnected peer
        emit peerQuit(i);
#ifdef __unix__
        close(i);
#endif
#ifdef _WIN32
        closesocket(i);
#endif
        //Remove the socket from the list to check in select
        FD_CLR(i, &master);
        return 1;
    }
    //Normal message coming here
    else
    {
        //partialMsg points to the start of the original buffer.
        //If there is more than one message on it, it will be increased to where the
        //next message begins
        char *partialMsg = buf;

        //These are variable for determining the ip address of the sender
        struct sockaddr_storage addr;
        socklen_t socklen = sizeof(addr);
        char ipstr2[INET6_ADDRSTRLEN];
        char service[20];

        //This holds the first 3 characters of partialMsg which will represent how long the recieved message should be.
        char bufLenStr[4] = {};

        //Get ip address of sender
        getpeername(i, (struct sockaddr*)&addr, &socklen);
        getnameinfo((struct sockaddr*)&addr, socklen, ipstr2, sizeof(ipstr2), service, sizeof(service), NI_NUMERICHOST);

        //We come back to here if theres more than one message in buf
moreToRead:

        //The first three characters of each message should be the length of the message.
        //We parse this to an integer so as not to confuse messages with one another
        //when more than one are recieved from the same socket at the same time
        //If we encounter a valid character after this length, we come back here
        //and iterate through the if statements again.
        bufLenStr[0] = partialMsg[0];
        bufLenStr[1] = partialMsg[1];
        bufLenStr[2] = partialMsg[2];
        bufLenStr[4] = '\0';
        //char *bufLenStrTemp = bufLenStr;
        int bufLen = atoi(bufLenStr);
        partialMsg = partialMsg+3;


        //These packets should be formatted like "/msghostname: msg\0"
        if(strncmp(partialMsg, "/msg", 4) == 0)
        {
            //The size of mes is the bufLen-4 (because we remove "/msg") +1 (because we need a null character at the end);
            char mes[bufLen-4+1] = {};
            //Copy the string in buf starting at buf[4] until we reach the length of the message
            //We dont want to keep the "/msg" substring, hence the +/- 4 here.
            //Remember, we added 3 to buf above to ignore the size of the message
            //buf is actually at buf[7] from its original location
            strncpy(mes, partialMsg+4, bufLen-4);

            //The following signal is going to the main thread and will call the slot prints(QString, QString)
            emit mess_rec(QString::fromUtf8(mes, strlen(mes)), ipstr2, false);

            //Check to see if theres anything else in the buffer and if we need to reiterate through these if statements
            if(partialMsg[bufLen] != '\0')
            {
                partialMsg += bufLen;
                goto moreToRead;
            }
            else
                return 0;
        }
        //These packets should come formatted like "/ip:hostname@192.168.1.1:hostname2@192.168.1.2\0"
        else if(strncmp(partialMsg, "/ip", 3) == 0)
        {
            qDebug() << "/ip recieved from " << QString::fromUtf8(ipstr2);
            int count = 0;

            //We need to extract each hostname and ip set out of the message and send them to the main thread
            //to check if whether we already know about them or not
            for(int k = 4; k < bufLen; k++)
            {
                //Theres probably a better way to do this
                if(partialMsg[k] == ':')
                {
                    char temp[INET_ADDRSTRLEN + 28] = {};
                    strncpy(temp, partialMsg+(k-count), count);
                    count = 0;
                    if((strlen(temp) < 2))
                        *temp = 0;
                    else
                        //The following signal is going to the main thread and will call the slot ipCheck(QString)
                        emit ipCheck(QString::fromUtf8(temp));
                }
                else
                    count++;
            }
            //Check to see if we need to reiterate because there is another message on the buffer
            if(partialMsg[bufLen] != '\0')
            {
                partialMsg += bufLen;
                goto moreToRead;
            }
            else
                return 0;


        }
        //This packet is asking us to communicate our list of peers with the sender, leads to us sending an /ip packet
        //These packets should come formatted like "/request\0"
        else if(!(strncmp(partialMsg, "/request", 8)))
        {
            qDebug() << "/request recieved from " << QString::fromUtf8(ipstr2) << "\nsending ips";
            //The following signal is going to the m_client object and thread and will call the slot sendIps(int)
            //The int here is the socketdescriptor we want to send our ip set too.
            emit sendIps(i);
            //Check to see if we need to reiterate because there is another message on the buffer
            if(partialMsg[8] != '\0')
            {
                partialMsg += 8;
                goto moreToRead;
            }
            else
                return 0;
        }
        //These packets are messages sent to the global chat room
        //These packets should come formatted like "/globalhostname: msg\0"
        else if(!(strncmp(partialMsg, "/global", 7)))
        {
            //bufLen-6 instead of 7 because we need a trailing NULL character for QString conversion
            char emitStr[bufLen-6] = {};
            strncpy(emitStr, (partialMsg+7), bufLen-7);
            emit mess_rec(QString::fromUtf8(emitStr), ipstr2, true);
            //Check to see if we need to reiterate because there is another message on the buffer
            if(partialMsg[bufLen] != '\0')
            {
                partialMsg += bufLen;
                goto moreToRead;
            }
            else
                return 0;
        }
        //This packet is an updated hostname for the computer that sent it
        //These packets should come formatted like "/hostnameHostname1\0"
        else if(!(strncmp(partialMsg, "/hostname", 9)))
        {
            qDebug() << "/hostname recieved" << QString::fromUtf8(ipstr2);
            //bufLen-8 instead of 9 because we need a trailing NULL character for QString conversion
            char emitStr[bufLen-8] = {};
            strncpy(emitStr, (partialMsg+9), bufLen-9);
            qDebug() << "hello";
            emit setPeerHostname(QString::fromUtf8(emitStr), QString::fromUtf8(ipstr2));
            if(partialMsg[bufLen] != '\0')
            {
                partialMsg += bufLen;
                goto moreToRead;
            }
            else
                return 0;
        }
        //This packet is asking us to communicate an updated hostname to the sender
        //These packets should come formatted like "/namerequest\0"
        else if(!(strncmp(partialMsg, "/namerequest", 12)))
        {
            qDebug() << "/namerequest recieved from " << QString::fromUtf8(ipstr2) << "\nsending hostname";
            emit sendName(i);
            if(partialMsg[bufLen] != '\0')
            {
                partialMsg += bufLen;
                goto moreToRead;
            }
            else
                return 0;
        }
    }
    return 1;
}

/**
 * @brief 				UDP packet recieved. Remember, these are connectionless
 * @param i				Socket descriptor to recieve UDP packet from
 * @return 				0 on success, 1 on error
 */
int mess_serv::udpRecieve(int i)
{
    QString hname;
    QString ipaddr;
    socklen_t si_other_len;
    sockaddr_in si_other;
    char service_disc[20];
    int status_disc;
    char buf[100] = {};
    char ipstr[INET6_ADDRSTRLEN];

    si_other_len = sizeof(sockaddr);
    recvfrom(i, buf, sizeof(buf)-1, 0, (sockaddr *)&si_other, &si_other_len);
    getnameinfo(((struct sockaddr*)&si_other), si_other_len, ipstr, sizeof(ipstr), service_disc, sizeof(service_disc), NI_NUMERICHOST);
    //std::cout << "upd message: " << buf << std::endl << "from ip: " << ipstr << std::endl;
    if (strncmp(buf, "/discover", 9) == 0)
    {
        struct sockaddr_in addr;
        int socket1;
        if ( (socket1 = (socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))) < 0)
            perror("socket:");
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = si_other.sin_addr.s_addr;
        addr.sin_port = htons(PORT_DISCOVER);
        char name[28] = {};
        char fname[34] = {};
        gethostname(name, sizeof name);
        strcpy(fname, "/name:\0");
        strcat(fname, name);
        int len = strlen(fname);

        for(int k = 0; k < 1; k++)
        {
            sendto(socket1, fname, len+1, 0, (struct sockaddr *)&addr, sizeof(addr));
        }
        //emit potentialReconnect(QString::fromUtf8(ipstr));
        char tname[strlen(buf)-8] = {};
        unsigned bufLen = strlen(buf);
        for(unsigned i = 9; i < (bufLen+1);i++)
        {
            tname[i-9] = buf[i];
        }
        //emit mess_peers(QString::fromUtf8(tname), QString::fromUtf8(ipstr));
#ifdef _WIN32
        closesocket(socket1);
#else
        close(socket1);
#endif

    }
    //This will get sent from anyone recieving a /discover packet
    //when this is recieved it add the sender to the list of peers and connects to him
    else if ((status_disc = strncmp(buf, "/name:", 6)) == 0)
    {
        char name[28];
        strcpy(name, &buf[6]);

        hname = QString::fromUtf8(name);
        ipaddr = QString::fromUtf8(ipstr);

        emit mess_peers(hname, ipaddr);
    }
    else
    {
        return 1;
    }

    return 0;
}

/**
 * @brief 				Main listener called from the run function.  Infinite while loop in here that is interuppted by
 *						the GUI thread upon shutdown.  Two listeners, one TCP/IP and one UDP, are created here and checked
 *						for new connections.  All new connections that come in here are listened to after they have had their
 *						descriptors added to the master FD set.
 * @return				Should never return, -1 means something terrible has happened
 */
int mess_serv::listener()
{
    //Potential rewrite to change getaddrinfo to a manual setup of the socket structures.
    //Changing to manual setup may improve load times on windows systems.  Locks us into
    //ipv4 however.
    struct addrinfo hints, *res;
    int s_discover, s_listen;
    sockaddr_in si_me;

    //TCP STUFF
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    getaddrinfo(NULL, PORT, &hints, &res);

    if((s_listen = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) < 0)
        perror("socket error: ");
    setsockopt(s_listen, SOL_SOCKET,SO_REUSEADDR, "true", sizeof(int));
    if(bind(s_listen, res->ai_addr, res->ai_addrlen) < 0)
        perror("bind error: ");
    if(listen(s_listen, BACKLOG) < 0)
        perror("listen error: ");
    //END OF TCP STUFF

    //UDP STUFF
    s_discover = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    setsockopt(s_discover, SOL_SOCKET, SO_BROADCAST, "true", sizeof (int));
    memset(&si_me, 0, sizeof(si_me));
    si_me.sin_family = AF_INET;
    si_me.sin_port = htons(PORT_DISCOVER);
    si_me.sin_addr.s_addr = INADDR_ANY;
    bind(s_discover, (sockaddr *)&si_me, sizeof(sockaddr));
    //END OF UDP STUFF

    freeaddrinfo(res);
    FD_ZERO(&master);
    FD_ZERO(&read_fds);
    //FD_ZERO(&write_fds);
    FD_SET(s_listen, &master);
    FD_SET(s_discover, &master);
    this->set_fdmax(s_listen);
    this->set_fdmax(s_discover);

    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 250000;

    //END of setup for sockets, being infinite while loop to listen.
    //Select is used with a time limit to enable the main thread to close
    //this thread with a signal.
    while( !this->isInterruptionRequested() )
    {
        read_fds = master;
        int status = 0;
        //This while loop will interrupt on signal from the main thread, or having a socket
        //that has data waiting to be read from.  Select returns -1 on error, 0 on no data
        //waiting on any socket.
        while( ( (status  ==  0) | (status == -1) ) && ( !this->isInterruptionRequested() ) )
        {
            if(status == -1){
                perror("select:");
            }
            read_fds = master;
            write_fds = master;
            status = select(fdmax+1, &read_fds, NULL, NULL, &tv);
            tv.tv_sec = 0;
            tv.tv_usec = 250000; //quarter of a second, select modifies this value so it must be reset before every loop
        }
        for(int i = 0; i <= fdmax; i++)
        {
            if(FD_ISSET(i, &read_fds))
            {
                //New tcp connection signaled from s_listen
                if(i == s_listen)
                {
                    this->newConnection(i);
                }
                //UDP packet sent to the udp socket
                else if(i == s_discover)
                {
                    this->udpRecieve(i);
                }
                //TCP packet sent to CONNECTED socket
                else
                {
                    this->tcpRecieve(i);
                }
            }
        }
    }
    return -1;
}
