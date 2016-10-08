#include <mess_discover.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <iostream>
#include <unistd.h>
#include <QWidget>

#ifdef __unix__
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#endif

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

#define PORT 13654

mess_discover::mess_discover(QWidget *parent) : QThread(parent)
{

}
void mess_discover::run()
{
    this->d_listener();
}
void mess_discover::d_listener()
{
    QString hname;
    QString ipaddr;

    sockaddr_in si_me, si_other;
    socklen_t si_other_len;
    int s;
    char service[20];
    struct timeval tv;
    fd_set fds, tempfds;

    tv.tv_sec = 0;
    tv.tv_usec = 500000;

    s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    setsockopt(s, SOL_SOCKET, SO_BROADCAST, "true", sizeof (int));
    //setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    FD_ZERO(&fds);
    FD_ZERO(&tempfds);
    FD_SET(s, &fds);
    memset(&si_me, 0, sizeof(si_me));
    si_me.sin_family = AF_INET;
    si_me.sin_port = htons(PORT);
    si_me.sin_addr.s_addr = INADDR_ANY;
    bind(s, (sockaddr *)&si_me, sizeof(sockaddr));
    char ipstr[INET6_ADDRSTRLEN];
    while( !this->isInterruptionRequested() )
    {
        char buf[100] = {};
        int n = 0;
        si_other_len = sizeof(sockaddr);
        //while(bytes_rec <= 0 && !this->isInterruptionRequested())
        //{
        //    bytes_rec = recvfrom(s, buf, sizeof(buf)-1, 0, (sockaddr *)&si_other, &si_other_len);
        //}

        while( ( ( n == 0 ) | ( n == -1 ) ) && ! ( this->isInterruptionRequested() ) )
        {
            tempfds = fds;
            n = select( s+1, &tempfds, NULL, NULL, &tv);
            tv.tv_usec = 500000;
            std::cout << "loop" << std::endl;
        }
        if( !this->isInterruptionRequested() )
        {
            recvfrom(s, buf, sizeof(buf)-1, 0, (sockaddr *)&si_other, &si_other_len);
        }

        //inet_ntop(si_other.sin_family, (struct sockaddr_in *)&si_other.sin_addr, ipstr, sizeof(ipstr));
        getnameinfo(((struct sockaddr*)&si_other), si_other_len, ipstr, sizeof(ipstr), service, sizeof(service), NI_NUMERICHOST);
        std::cout << "upd message: " << buf << std::endl << "from ip: " << ipstr << std::endl;
        int status;
        if (strncmp(buf, "/discover", 9) == 0)
        {
            struct sockaddr_in addr;
            int socket1;
            if ( (socket1 = (socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))) < 0)
                perror("socket:");
            memset(&addr, 0, sizeof(addr));
            addr.sin_family = AF_INET;
            addr.sin_addr.s_addr = si_other.sin_addr.s_addr;
            addr.sin_port = htons(PORT);
            char name[28] = {};
            char fname[34] = {};
            gethostname(name, sizeof name);
            strcpy(fname, "/name:\0");
            strcat(fname, name);
            int len = strlen(fname);

            sendto(socket1, fname, len+1, 0, (struct sockaddr *)&addr, sizeof(addr));
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
        else if ((status = strncmp(buf, "/name:", 6)) == 0)
        {
            char name[28];
            strcpy(name, &buf[6]);

            hname = QString::fromUtf8(name);
            ipaddr = QString::fromUtf8(ipstr);

            emit mess_peers(hname, ipaddr);
        }
        else if (( status = strncmp(buf, "/exit", 5)) == 0)
        {
            emit exitRecieved(QString::fromUtf8(ipstr));
        }
    }
}
