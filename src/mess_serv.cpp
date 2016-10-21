#include <mess_serv.h>

mess_serv::mess_serv(QWidget *parent) : QThread(parent)
{


}

//Start of thread
void mess_serv::run()
{
    QObject::connect(this->parent(), SIGNAL (sendPeerListSignal(peerClass*)), this, SLOT(recievePeerList(peerClass*)));
    this->listener();
}

void mess_serv::recievePeerList(peerClass *peers)
{
    std::cout << "Hey look, PEERS!" << std::endl;
    std::cout << peers->peers[0].hostname.toStdString() << " from mess_serv" << std::endl;
    return;
}

//Currently obsolete
void mess_serv::new_fds(int s)
{
    FD_SET(s, &master);
    return;
}

//Accept a new TCP connection from the listener socket and let the GUI know.
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
    return result;
}

//Add an FD to the set if its not already in there and check if its the new max
void mess_serv::update_fds(int s)
{
    if( !( FD_ISSET(s, &master) ) )
    {
        FD_SET(s, &master);
        this->set_fdmax(s);
    }
}

//Determine if a new socket descriptor is the highest in the set, adjust the fd_max variable accordingly.
int mess_serv::set_fdmax(int m)
{
    if(m > fdmax)
    {
        fdmax = m;
        return 0;
    }
    return -1;
}

//Called when the TCP listener recieves a new connection
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

//Message recieved from a previously connected TCP socket
int mess_serv::tcpRecieve(int i)
{
    int nbytes;
    char buf[1050] = {};

    if((nbytes = recv(i,buf,sizeof(buf), 0)) <= 0)
    {
        if(nbytes == 0)
        {
            std::cout << "connection closed" << std::endl;
        }

        else
        {
            perror("recv");
        }
        emit peerQuit(i);
#ifdef __unix__
        close(i);
#endif
#ifdef _WIN32
        closesocket(i);
#endif
        FD_CLR(i, &master);
        return 0;
    }
    //Normal message coming here
    else
    {
        char mes[nbytes] = {};
        struct sockaddr_storage addr;
        socklen_t socklen = sizeof(addr);
        char ipstr2[INET6_ADDRSTRLEN];
        char service[20];

        if(strncmp(buf, "/msg", 4) == 0)
        {


            strncpy(mes, buf + 4, strlen(buf) - 4);
            getpeername(i, (struct sockaddr*)&addr, &socklen);
            //struct sockaddr_in *temp = (struct sockaddr_in *)&addr;
            //inet_ntop(AF_INET, &temp->sin_addr, ipstr2, sizeof ipstr2);
            getnameinfo((struct sockaddr*)&addr, socklen, ipstr2, sizeof(ipstr2), service, sizeof(service), NI_NUMERICHOST);

            emit mess_rec(QString::fromUtf8(mes, strlen(mes)), ipstr2);
            return 0;
        }
        else if(strncmp(buf, "/ip", 3) == 0)
        {
            std::cout << "hello" << std::endl;
            int count = 0;

            for(unsigned k = 3; k < strlen(buf); k++)
            {
                if(buf[k] == ':')
                {
                   char temp[INET6_ADDRSTRLEN] = {};
                   strncpy(temp, buf+(k-count+1), count-1);
                   count = 0;
                   if((strlen(temp) < 2))
                       *temp = 0;
                   else
                       emit ipCheck(QString::fromUtf8(temp));
                }
                count++;
            }


        }
        else if(strncmp(buf, "/request", 10))
        {
            emit sendIps(i);
        }
    }
    return 1;
}

//UDP packet recieved. Remember, these are connectionless
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
    std::cout << "upd message: " << buf << std::endl << "from ip: " << ipstr << std::endl;
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

        for(int k = 0; k < 2; k++)
        {
            sendto(socket1, fname, len+1, 0, (struct sockaddr *)&addr, sizeof(addr));
        }
        emit potentialReconnect(QString::fromUtf8(ipstr));
        char tname[strlen(buf)-8] = {};
        unsigned bufLen = strlen(buf);
        for(unsigned i = 9; i < (bufLen+1);i++)
        {
            tname[i-9] = buf[i];
        }
        emit mess_peers(QString::fromUtf8(tname), QString::fromUtf8(ipstr));
#ifdef _WIN32
        closesocket(socket1);
#else
        close(socket1);
#endif

    }
    else if ((status_disc = strncmp(buf, "/name:", 6)) == 0)
    {
        char name[28];
        strcpy(name, &buf[6]);

        hname = QString::fromUtf8(name);
        ipaddr = QString::fromUtf8(ipstr);

        emit mess_peers(hname, ipaddr);
    }
    else if (( status_disc = strncmp(buf, "/exit", 5)) == 0)
    {
        emit exitRecieved(QString::fromUtf8(ipstr));
    }
    else
    {
        return 1;
    }

    return 0;
}

//Main listener called from the run function.  Infinite while loop in here that is interuppted by
//the GUI thread upon shutdown.  Two listeners, one TCP/IP and one UDP, are created here and checked
//for new connections.  All new connections that come in here are listened to after they have had their
//descriptors added to the master FD set.
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
