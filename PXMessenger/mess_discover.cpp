#include "mess_discover.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <QtCore>
#include <iostream>
#include <unistd.h>

#define PORT 3491
//char nlist[256][28];

mess_discover::mess_discover()
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

    std::cout << "hello from d3" << std::endl;
    sockaddr_in si_me, si_other;
    int s;
    s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    int broadcast = 1;
    setsockopt(s, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof (broadcast));

    memset(&si_me, 0, sizeof(si_me));
    si_me.sin_family = AF_INET;
    si_me.sin_port = htons(PORT);
    si_me.sin_addr.s_addr = INADDR_ANY;
    bind(s, (sockaddr *)&si_me, sizeof(sockaddr));
    char ipstr[INET6_ADDRSTRLEN];
    while(1)
    {
        char buf[100];
        unsigned slen = sizeof(sockaddr);
        recvfrom(s, buf, sizeof(buf)-1, 0, (sockaddr *)&si_other, &slen);
        inet_ntop(si_other.sin_family, (struct sockaddr_in *)&si_other.sin_addr, ipstr, sizeof(ipstr));
        std::cout << "upd message: " << buf << std::endl << "from ip: " << ipstr << std::endl;
        if (strcmp(buf, "/discover") == 0)
        {
            struct sockaddr_in addr;
            int socket1;
            if ( (socket1 = (socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))) < 0)
                perror("socket:");
            memset(&addr, 0, sizeof(addr));
            addr.sin_family = AF_INET;
            addr.sin_addr.s_addr = si_other.sin_addr.s_addr;
            addr.sin_port = htons(PORT);
            char name[28];
            gethostname(name, sizeof name);
            char fname[34];
            strcpy(fname, "/name:");
            strcat(fname, name);
            int len = strlen(fname);

            sendto(socket1, fname, len+1, 0, (struct sockaddr *)&addr, sizeof(addr));
            close(socket1);
        }
        int status;
        if ((status = strncmp(buf, "/name:", 6)) == 0)
        {
            char name[28];
            strcpy(name, &buf[6]);

            hname = QString::fromUtf8(name);
            ipaddr = QString::fromUtf8(ipstr);

            emit mess_peers(hname, ipaddr);
        }
    }
}
