#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <mess_serv.h>
#include <unistd.h>
#include <QtCore>
#include <iostream>

#define PORT "3490"
#define BACKLOG 20
fd_set master;
fd_set read_fds;
int fdmax;
mess_serv::mess_serv()
{

}
void mess_serv::run()
{
    this->listener();
}
int mess_serv::accept_new(int socketfd, sockaddr_storage *their_addr)
{
    int result;
    socklen_t addr_size = sizeof(their_addr);
    result = (accept(socketfd, (struct sockaddr *)&their_addr, &addr_size));
    return result;
}

int mess_serv::listener()
{
    int new_fd;
    struct sockaddr_storage their_addr;
    struct addrinfo hints, *res;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    getaddrinfo(NULL, PORT, &hints, &res);

    if((socketfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) < 0)
        perror("socket error: ");
    int yes = 1;
    setsockopt(socketfd, SOL_SOCKET,SO_REUSEADDR, &yes, sizeof(int));
    if(bind(socketfd, res->ai_addr, res->ai_addrlen) < 0)
        perror("bind error: ");
    if(listen(socketfd, BACKLOG) < 0)
        perror("listen error: ");
    FD_ZERO(&master);
    FD_ZERO(&read_fds);
    FD_SET(socketfd, &master);
    int nbytes;

    fdmax = socketfd;
    while(1)
    {
        char buf[256] = {};
        read_fds = master;
        if(select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1){
            perror("select:");
        }
        for(int i = 0; i <= fdmax; i++)
        {
            if(FD_ISSET(i, &read_fds))
            {
                if(i == socketfd)
                {
                    new_fd = accept_new(socketfd, &their_addr);
                    if(new_fd == -1)
                    {
                        perror("accept:");
                    }
                    else
                    {
                        FD_SET(new_fd, &master);
                        if(new_fd > fdmax){
                            fdmax = new_fd;
                        }
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
                        close(i);
                        FD_CLR(i, &master);
                    }
                    else
                    {
                        char hostn[12] = {};
                        char mes[244] = {};
                        strncat(hostn, buf, 12);
                        int hostnLen = strlen(hostn);
                        for(int k = 0; k < hostnLen; k++)
                        {
                            if(hostn[k] == ' ')
                            {
                                hostn[k] = '\0';
                            }
                        }
                        for(int k = 12; ( ( k < 255) && (buf[k] != '\0') ); k++)
                        {
                            mes[k-12] = buf[k];
                        }
                        emit mess_rec(QString::fromUtf8(hostn, strlen(hostn)) + ": " + QString::fromUtf8(mes, strlen(mes)));
                    }
                }
            }
        }
    }
    /*while((new_fd = accept_new(socketfd, &their_addr))){
    char s[INET_ADDRSTRLEN];

    socklen_t addr_size2 = sizeof(their_addr);
    getpeername(new_fd, (struct sockaddr*)&their_addr, &addr_size2);
    struct sockaddr_in *socketfd2 = (struct sockaddr_in *)&their_addr;
    inet_ntop(AF_INET, &socketfd2->sin_addr, s, sizeof s);

    mess_recieve *p = new mess_recieve(QString::fromUtf8(s), new_fd);
    threads << p;
    socketDesc.push_back(new_fd);
    QObject::connect(p, SIGNAL (mess_rec2(const QString)), this, SLOT (mess_rec3(const QString)));
    p->start();
    }
    */
    return 0;
}
void mess_serv::mess_rec3(const QString s)
{
    qDebug() << s;
    emit mess_rec(s);
}
