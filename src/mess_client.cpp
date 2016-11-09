#include <mess_client.h>

MessengerClient::MessengerClient()
{

}
void MessengerClient::setlocalUUID(QString uuid)
{
    localUUID = uuid;
}
void MessengerClient::udpSendSlot(QString msg, unsigned short port)
{
    this->udpSend(msg.toStdString().c_str(), port);
}
void MessengerClient::udpSend(const char* msg, unsigned short port)
{
    int len;
    struct sockaddr_in broadaddr;
    evutil_socket_t socketfd2;

    memset(&broadaddr, 0, sizeof(broadaddr));
    broadaddr.sin_family = AF_INET;
    broadaddr.sin_addr.s_addr = inet_addr(MULTICAST_ADDRESS);
    broadaddr.sin_port = htons(port);

    if ( (socketfd2 = (socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))) < 0)
        perror("socket:");

    len = strlen(msg);

    for(int i = 0; i < 1; i++)
    {
        sendto(socketfd2, msg, len+1, 0, (struct sockaddr *)&broadaddr, sizeof(broadaddr));
    }
    evutil_closesocket(socketfd2);
}
void MessengerClient::connectToPeerSlot(evutil_socket_t s, QString ipaddr, QString service)
{
    this->c_connect(s, ipaddr.toStdString().c_str(), service.toStdString().c_str());
}
/**
 * @brief 			This function connects a socket to a specific ip address.
 * @param socketfd	socket to connect on
 * @param ipaddr	ip address to connect socket to
 * @return			-1 on failure to connect, socket descriptor on success
 */
int MessengerClient::c_connect(evutil_socket_t socketfd, const char *ipaddr, const char *service)
{
    int status;
    struct addrinfo hints, *res;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if ((status = getaddrinfo(ipaddr, service, &hints, &res)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", (char*)gai_strerror(status));
        return 2;
    }

    if( (status = ::connect(socketfd, res->ai_addr, res->ai_addrlen)) < 0 )
    {
        std::cout << strerror(errno) << std::endl;
        freeaddrinfo(res);
        emit resultOfConnectionAttempt(socketfd, status);

        evutil_closesocket(socketfd);

        return 1;
    }
    qDebug() << "Successfully connected to" << ipaddr << "on port" << service << "on socket" << socketfd;
    emit resultOfConnectionAttempt(socketfd, status);
    freeaddrinfo(res);
    return 0;
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
void MessengerClient::send_msg(evutil_socket_t socketfd, const char *msg, const char *type, const char *uuid, const char *theiruuid)
{
    int packetLen, bytes_sent, sendcount = 0;
    bool print = false;

    //Combine strings into final message (host): (msg)\0

    packetLen = strlen(uuid) + strlen(type) + strlen(msg);
    if(!strcmp(type, "/msg") )
    {
        print = true;
    }

    int packetLenLength = 4;
    //account for the numbers we just added to the front of the final message

    char full_mess[packetLen + packetLenLength + 1] = {};
    sprintf(full_mess, "%04d", packetLen);

    packetLen += packetLenLength;

    strcat(full_mess, uuid);
    strcat(full_mess, type);
    strcat(full_mess, msg);

    bytes_sent = this->partialSend(socketfd, full_mess, packetLen, sendcount);

    if(bytes_sent > 0)
    {
        if(bytes_sent >= packetLen)
        {
            emit resultOfTCPSend(0, QString::fromUtf8(theiruuid), QString::fromUtf8(msg), print);
        }
        else
        {
            std::cout << "Partial Send has failed not all bytes sent" << std::endl;
            emit resultOfTCPSend(bytes_sent, QString::fromUtf8(theiruuid), QString::fromUtf8(msg), print);
        }
    }
    else
    {
        emit resultOfTCPSend(-1, QString::fromUtf8(theiruuid), QString::fromUtf8(msg), print);
    }
    return;
}
void MessengerClient::sendMsgSlot(evutil_socket_t s, QString msg, QString type, QUuid uuid, QString theiruuid)
{
    this->send_msg(s, msg.toStdString().c_str(), type.toStdString().c_str(), uuid.toString().toStdString().c_str(), theiruuid.toStdString().c_str());
}
/**
 * @brief 			Recursively sends all data in case the kernel fails to do so in one pass
 * @param socketfd	Socket to send to
 * @param msg		final formatted message to send
 * @param len		length of message to send
 * @param count		Only attempt to resend 5 times so as not to hang the program if something goes wrong
 * @return 			-1 on error, total bytes sent otherwise
 */
int MessengerClient::partialSend(evutil_socket_t socketfd, const char *msg, int len, int count)
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
        qDebug() << "send error was on socket " << socketfd;
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
