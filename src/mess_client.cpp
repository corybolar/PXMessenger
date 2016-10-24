#include <mess_client.h>

mess_client::mess_client()
{

}

int mess_client::c_connect(int socketfd, const char *ipaddr)
{
    int status;
    struct addrinfo hints, *res;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if ((status = getaddrinfo(ipaddr, PORT, &hints, &res)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        return 2;
    }
    //socketfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

    //struct sockaddr_in temp;
    //inet_pton(AF_INET, ipaddr, &(temp.sin_addr));
    //temp.sin_family = AF_INET;
    //temp.sin_addr.s_addr = inet_addr(ipaddr);

    if( (status = ::connect(socketfd, res->ai_addr, res->ai_addrlen)) < 0 )
    {
        std::cout << strerror(errno) << std::endl;
        freeaddrinfo(res);
        return -1;
    }
    freeaddrinfo(res);
    return socketfd;
}
int mess_client::send_msg(int socketfd, const char *msg, const char *host, const char *type)
{
    int len, bytes_sent, sendcount = 0;
    char msgLen[3];

    //Combine strings into final message (host): (msg)\0

    if(!strcmp(type, "/msg") || !strcmp(type,"/global"))
    {
        len = strlen(type) + strlen(host) + strlen(msg) + 2;
    }
    else
    {
        len = strlen(type) + strlen(msg);
    }

    sprintf(msgLen, "%3d", len);
    //account for the numbers we just added to the front of the final message
    len += 3;

    char full_mess[len] = {};
    strncpy(full_mess, msgLen, 3);

    strcat(full_mess, type);
    if(!strcmp(type, "/msg") || !strcmp(type,"/global"))
    {
       strcat(full_mess, host);
       full_mess[strlen(host) + strlen(type) + 3] = ':';
       full_mess[strlen(host) + strlen(type) + 3 + 1] = ' ';
    }
    strcat(full_mess, msg);

    bytes_sent = this->partialSend(socketfd, full_mess, len, sendcount);

    if(bytes_sent > 0)
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
#ifdef _WIN32
        return -5;
#else
        switch (errno)
        {
        case EPIPE:
            return -5;
        }
#endif

    }
    return bytes_sent;
}
/*This function recursively sends all data in case the kernel fails to do so in one pass*/
/*Recieving this has not yet been implemented, sending a constant buffer size could help fix this in the future.*/
int mess_client::partialSend(int socketfd, const char *msg, int len, int count)
{
    int status2 = 0;
#ifdef _WIN32
    int status = send(socketfd, msg, len, 0);
#else
    int status = send(socketfd, msg, len, MSG_NOSIGNAL);
#endif

    if( (status <= 0) )
    {
        perror("send:");
        return -1;
    }

    if( ( status != len ) && ( count < 10 ) )
    {
        int len2 = len - status;
        char msg2[len2];
        strncpy(msg2, &msg[status], len2);
        count++;

        status2 = partialSend(socketfd, msg2, len2, count);
        if(status2 <= 0)
            return -1;
    }
    return status + status2;
}
void mess_client::sendNameSlot(int s)
{
    char name[128] = {};

    gethostname(name, sizeof name);
    this->send_msg(s,name, "", "/hostname");
    return;
}
/*
int mess_client::sendName(int s)
{
    int bytes_sent, len, sendcount = 0;
    char name[128] = {};
    char msg[138] = {};


    gethostname(name, sizeof name);

    strcpy(msg, "/hostname");
    strcat(msg, name);
    len = strlen(msg);

    bytes_sent = this->partialSend(s, msg, len, sendcount);

    if(bytes_sent > 0)
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
#ifdef _WIN32
        return -5;
#else
        switch (errno)
        {
        case EPIPE:
            return -5;
        }
#endif

    }
    return bytes_sent;
}
*/
