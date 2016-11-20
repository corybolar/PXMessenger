#include <pxmclient.h>

PXMClient::PXMClient()
{
}
void PXMClient::sendUDP(const char* msg, unsigned short port)
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

    unsigned char loopback = 1;
    setsockopt(socketfd2, IPPROTO_IP, IP_MULTICAST_LOOP, &loopback, sizeof(loopback));

    for(int i = 0; i < 1; i++)
    {
        sendto(socketfd2, msg, len+1, 0, (struct sockaddr *)&broadaddr, sizeof(broadaddr));
    }
    evutil_closesocket(socketfd2);
}
/**
 * @brief 			This function connects a socket to a specific ip address.
 * @param socketfd	socket to connect on
 * @param ipaddr	ip address to connect socket to
 * @return			-1 on failure to connect, socket descriptor on success
 */
int PXMClient::connectToPeer(evutil_socket_t socketfd, sockaddr_in socketAddr)
{
    int status;
    if( (status = ::connect(socketfd, (struct sockaddr*)&socketAddr, sizeof(socketAddr))) < 0 )
    {
        qDebug() << strerror(errno);
        emit resultOfConnectionAttempt(socketfd, status);
        evutil_closesocket(socketfd);
        return 1;
    }
    emit resultOfConnectionAttempt(socketfd, status);
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
void PXMClient::sendMsg(evutil_socket_t socketfd, const char *msg, size_t msgLen, const char *type, QUuid uuid, const char *theiruuid)
{
    int bytesSent = 0;
    uint16_t packetLen;
    uint16_t packetLenNBO;
    bool print = false;

    //Combine strings into final message (host): (msg)\0

    if(msgLen > 65400)
    {
        emit resultOfTCPSend(-1, QString::fromLatin1(theiruuid), QString::fromLatin1(msg), print);
    }
    packetLen = PACKED_UUID_BYTE_LENGTH + strlen(type) + msgLen;

    if(!strcmp(type, "/msg") )
    {
        print = true;
    }

    char full_mess[packetLen + 1] = {};

    packetLenNBO = htons(packetLen);

    //strcat(full_mess, uuid);
    packUUID(full_mess, uuid ,0);
    memcpy(full_mess+PACKED_UUID_BYTE_LENGTH, type, strlen(type));
    memcpy(full_mess+PACKED_UUID_BYTE_LENGTH+strlen(type), msg, msgLen);

    bytesSent = this->recursiveSend(socketfd, &packetLenNBO, sizeof(uint16_t), 0);

    if(bytesSent != sizeof(uint16_t))
    {
        emit resultOfTCPSend(-1, QString::fromLatin1(theiruuid), QString::fromLatin1(msg), print);
        return;
    }

    bytesSent = this->recursiveSend(socketfd, full_mess, packetLen, 0);

    if(bytesSent >= static_cast<int>(packetLen))
    {
        bytesSent = 0;
    }
    emit resultOfTCPSend(bytesSent, QString::fromLatin1(theiruuid), QString::fromLatin1(msg), print);
    return;
}
void PXMClient::sendMsgSlot(evutil_socket_t s, QString msg, QString type, QUuid uuid, QString theiruuid)
{
    this->sendMsg(s, msg.toLocal8Bit().constData(), strlen(msg.toLocal8Bit().constData()), type.toLocal8Bit().constData(), uuid, theiruuid.toLocal8Bit().constData());
}
void PXMClient::packUUID(char *dest, QUuid uuid, size_t index)
{
    uint32_t uuidSectionL = htonl((uint32_t)(uuid.data1));
    memcpy(dest + index, &(uuidSectionL), sizeof(uint32_t));
    index += sizeof(uint32_t);
    uint16_t uuidSectionS = htons((uint16_t)(uuid.data2));
    memcpy(dest + index, &(uuidSectionS), sizeof(uint16_t));
    index += sizeof(uint16_t);
    uuidSectionS = htons((uint16_t)(uuid.data3));
    memcpy(dest + index, &(uuidSectionS), sizeof(uint16_t));
    index += sizeof(uint16_t);
    memcpy(dest + index, &(uuid.data4), 8);
    return;
}

void PXMClient::sendIpsSlot(evutil_socket_t s, const char *msg, size_t len, QString type, QUuid uuid, QString theiruuid)
{
    this->sendMsg(s, msg, len, type.toLocal8Bit().constData(), uuid, theiruuid.toLocal8Bit().constData());
    delete [] msg;
}

/**
 * @brief 			Recursively sends all data in case the kernel fails to do so in one pass
 * @param socketfd	Socket to send to
 * @param msg		final formatted message to send
 * @param len		length of message to send
 * @param count		Only attempt to resend 5 times so as not to hang the program if something goes wrong
 * @return 			-1 on error, total bytes sent otherwise
 */
int PXMClient::recursiveSend(evutil_socket_t socketfd, void *msg, int len, int count)
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
        qDebug() << "We are partially sending this msg";
        int len2 = len - status;
        //uint8_t cast to avoid compiler warning, we want to advance the pointer the number of bytes in status
        msg = (uint8_t*)msg + status;
        count++;

        status2 = recursiveSend(socketfd, msg, len2, count);
        if(status2 <= 0)
            return -1;
    }
    return status + status2;
}
