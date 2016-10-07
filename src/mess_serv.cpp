#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <mess_serv.h>
#include <unistd.h>
#include <QtCore>
#include <iostream>

#ifdef __unix__
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/select.h>
#endif

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

#define PORT "13653"
#define BACKLOG 20
mess_serv::mess_serv()
{

}
void mess_serv::run()
{
    this->listener();
}
void mess_serv::new_fds(int s)
{
    FD_SET(s, &master);
    return;
}

int mess_serv::accept_new(int socketfd, sockaddr_storage *their_addr)
{
    int result;
    char ipstr[INET6_ADDRSTRLEN];
    char service[20];

    socklen_t addr_size = sizeof(sockaddr);
    result = (accept(socketfd, (struct sockaddr *)&their_addr, &addr_size));
    //inet_ntop(AF_INET, &(((struct sockaddr_in*)&their_addr)->sin_addr), ipstr, sizeof(ipstr));
    getnameinfo(((struct sockaddr*)&their_addr), addr_size, ipstr, sizeof(ipstr), service, sizeof(service), NI_NUMERICHOST);
    emit new_client(result, QString::fromUtf8(ipstr, strlen(ipstr)));
    return result;
}

void mess_serv::update_fds(int s)
{
    if( !( FD_ISSET(s, &master) ) )
    {
        FD_SET(s, &master);
        this->set_fdmax(s);
    }
}

int mess_serv::set_fdmax(int m)
{
    if(m > fdmax)
    {
        fdmax = m;
        return 0;
    }
    return -1;
}

int mess_serv::listener()
{
#ifdef _WIN32
    unsigned new_fd;
#else
	int new_fd;
#endif
    struct sockaddr_storage their_addr;
    struct addrinfo hints, *res;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    getaddrinfo(NULL, PORT, &hints, &res);

    if((socketfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) < 0)
        perror("socket error: ");
    setsockopt(socketfd, SOL_SOCKET,SO_REUSEADDR, "true", sizeof(int));
    if(bind(socketfd, res->ai_addr, res->ai_addrlen) < 0)
        perror("bind error: ");
    if(listen(socketfd, BACKLOG) < 0)
        perror("listen error: ");

    freeaddrinfo(res);
    FD_ZERO(&master);
    FD_ZERO(&read_fds);
    FD_ZERO(&write_fds);
    FD_SET(socketfd, &master);

    int nbytes;
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 250000;

    fdmax = socketfd;
    while(1)
    {
        char buf[256] = {};
        read_fds = master;
        int status = 0;
        while( ( (status  ==  0) | (status == -1) ) )
        {
            if(status == -1){
                perror("select:");
            }
            read_fds = master;
            write_fds = master;
            status = select(fdmax+1, &read_fds, NULL, NULL, &tv);
            tv.tv_sec = 0;
            tv.tv_usec = 250000;
        }
        for(int i = 0; i <= fdmax; i++)
        {
            if(FD_ISSET(i, &read_fds))
            {
                if(i == socketfd)
                {
                    new_fd = accept_new(socketfd, &their_addr);
#ifdef __unix__
                    if(new_fd == -1)
                    {
                        perror("accept:");
                    }
#endif
#ifdef _WIN32
                    if(new_fd == INVALID_SOCKET)
                    {
                        perror("accept:");
                    }
#endif
                    else
                    {
                        this->update_fds(new_fd);
                        std::cout << "new connection" << std::endl;
                    }
                }
                else
                {
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
                    }
                    else
                    {
                        char mes[nbytes] = {};
                        struct sockaddr_storage addr;
                        socklen_t socklen = sizeof(addr);
                        char ipstr2[INET6_ADDRSTRLEN];
                        char service[20];

                        strcpy(mes, buf);

                        getpeername(i, (struct sockaddr*)&addr, &socklen);
                        //struct sockaddr_in *temp = (struct sockaddr_in *)&addr;
                        //inet_ntop(AF_INET, &temp->sin_addr, ipstr2, sizeof ipstr2);
                        getnameinfo((struct sockaddr*)&addr, socklen, ipstr2, sizeof(ipstr2), service, sizeof(service), NI_NUMERICHOST);

                        emit mess_rec(QString::fromUtf8(mes, strlen(mes)), ipstr2);
                    }
                }
            }
        }
    }
    return 0;
}
