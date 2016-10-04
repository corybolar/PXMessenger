#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include "mess_client.h"
#define PORT "3490"

//char *fmsg;
char fhost[12];
mess_client::mess_client()
{

}
void mess_client::setHost(const char *host)
{

    for(int i = 0; i < 12; i++)
    {
        fhost[i] = ' ';
    }
    for(int i = 0; i < 12; i++)
    {
        if(host[i] != '\0')
            fhost[i] = host[i];
        else
            break;
    }
    return;
}

int mess_client::c_connect(int socketfd, const char *ipaddr)
{
    int status;
    struct addrinfo hints, *res;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((status = getaddrinfo(ipaddr, PORT, &hints, &res)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        return 2;
    }
    //socketfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

    struct sockaddr_in temp;
    inet_pton(AF_INET, ipaddr, &(temp.sin_addr));
    if( (status = connect(socketfd, res->ai_addr, res->ai_addrlen)) < 0 )
    {
        std::cout << strerror(errno) << std::endl;
        freeaddrinfo(res);
        return -1;
    }
    freeaddrinfo(res);
    return socketfd;
}
int mess_client::send_msg(int socketfd, const char *msg, const char *host)
{
    //this->c_connect(socketfd, ipaddr);
    int len, bytes_sent, sendcount = 0;

    if( ( strcmp(msg, "exit") == 0 ) && ( strlen(msg) == 4 ) )
    {
        return -1;
    }
    len = strlen(msg) + strlen(host) + 3;
    char full_mess[len] = {};
    strncpy(full_mess, host, strlen(host));
    full_mess[strlen(host)] = ':';
    full_mess[strlen(host)+1] = ' ';
    strcat(full_mess, msg);
    bytes_sent = this->partialSend(socketfd, full_mess, len, sendcount);
    if(bytes_sent >= 0)
    {
        if(bytes_sent >= len)
        {
            return bytes_sent;
        }
        else
        {
            std::cout << "Partial Send has failed not all bytes sent" << std::endl;
        }
    }
    else
    {
        switch (errno)
        {
        case EPIPE:
            return -5;
        }
    }
    return bytes_sent;
}
/*This function recursively sends all data in case the kernel fails to do so in one pass*/
/*Recieving this has not yet been implemented, sending a constant buffer size could help fix this in the future.*/
int mess_client::partialSend(int socketfd, const char *msg, int len, int count)
{
    int status2 = 0;
    int status = send(socketfd, msg, len, MSG_NOSIGNAL);
    if( ( status != len ) && ( count < 10 ) )
    {
        int len2 = len - status;
        char msg2[len2];
        strncpy(msg2, &msg[status], len2);
        count++;

        status2 = partialSend(socketfd, msg2, len2, count);
    }
    if(status < 1)
        perror("send:");
    return status + status2;
}
