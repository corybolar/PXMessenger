#include <mess_client.h>

mess_client::mess_client()
{

}


/**
 * @brief 			This function connects a socket to a specific ip address.
 * @param socketfd	socket to connect on
 * @param ipaddr	ip address to connect socket to
 * @return			-1 on failure to connect, socket descriptor on success
 */
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

    if( (status = ::connect(socketfd, res->ai_addr, res->ai_addrlen)) < 0 )
    {
        std::cout << strerror(errno) << std::endl;
        freeaddrinfo(res);
        return -1;
    }
    freeaddrinfo(res);
    return socketfd;
}

/**
 *	@brief			This will send a message to the socket, should check beforehand to make sure its connected.
 * 					The size of the final message is sent in the first three characters
 *  @param socketfd the socket descriptor to send to, does not check if is connected or not
 *  @param msg 		message to send
 *  @param host 	hostname of current computer to display before msg
 *  @param type		type of message to be sending.  Valid types are /msg /hostname /request
 *   				/namerequest /ip
 *  @return 		number of bytes that were sent, should be equal to strlen(full_mess).
 *   				-5 if socket is not connected
 */
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

    sprintf(msgLen, "%03d", len);
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

/**
 * @brief 			Recursively sends all data in case the kernel fails to do so in one pass
 * @param socketfd	Socket to send to
 * @param msg		final formatted message to send
 * @param len		length of message to send
 * @param count		Only attempt to resend 5 times so as not to hang the program if something goes wrong
 * @return 			-1 on error, total bytes sent otherwise
 */
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

/**
 * @brief 			Slot function for signal called from mess_serv class.  Sends hostname to socket
 * @param s			Socket to send our hostname to
 */
void mess_client::sendNameSlot(int s)
{
    char name[128] = {};

    gethostname(name, sizeof name);
    this->send_msg(s,name, "", "/hostname");
    return;
}
