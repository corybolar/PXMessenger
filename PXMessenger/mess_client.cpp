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

char *fmsg;
char fhost[13];
mess_client::mess_client()
{

}
void mess_client::setMsg(const char *msg)
{
    fmsg = new char[strlen(msg)];
    strcpy(fmsg, msg);
    return;
}
void mess_client::setHost(const char *host)
{

    for(int i = 0; i < 13; i++)
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
        return -1;
    }
    return socketfd;
}
int mess_client::send_msg(int socketfd, const char *msg, const char *ipaddr)
{
    //this->c_connect(socketfd, ipaddr);
    int len, bytes_sent;

    if( strcmp(msg, "exit") == 0 )
    {
        return -1;
    }
    len = strlen(msg) + 13;
    char full_mess[len];
    for(int i = 0; i < len; i++)
    {
        full_mess[i]=' ';
    }
    strncpy(full_mess, fhost, strlen(fhost));
    int len2 = strlen(fhost);
    for(int i = len2; i < len; i++)
    {
        full_mess[i] = msg[i-len2];
    }
    bytes_sent = this->partialSend(socketfd, full_mess, len);
    if(bytes_sent >= 0)
    {
        if(bytes_sent >= len)
        {
            return bytes_sent;
        }
        else
        {
            //ADD code to resend partial send; while loop needed
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
int mess_client::partialSend(int socketfd, const char *msg, int len)
{
    return send(socketfd, msg, len, MSG_NOSIGNAL);
}
