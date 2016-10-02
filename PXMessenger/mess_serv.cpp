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
#include <mess_recieve.h>

#define PORT "3490"
#define BACKLOG 20
mess_serv::mess_serv()
{

}
void mess_serv::run()
{
    this->listener();
}
int mess_serv::closeThreads()
{
    while(threads.length() > 0)
    {
        close(socketDesc[threads.length()-1]);
        threads[threads.length()-1]->quit();
        threads.removeLast();
    }
    close(socketfd);
    return 0;
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
    while((new_fd = accept_new(socketfd, &their_addr))){
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
    return 0;
}
void mess_serv::mess_rec3(const QString s)
{
    qDebug() << s;
    emit mess_rec(s);
}
