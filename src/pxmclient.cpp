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
        xdebug("socket: " + QString::fromUtf8(strerror(errno)));

    len = strlen(msg);

    char loopback = 1;
    if(setsockopt(socketfd2, IPPROTO_IP, IP_MULTICAST_LOOP, &loopback, sizeof(loopback)) < 0)
        xdebug("setsockopt: " + QString::fromUtf8(strerror(errno)));

    for(int i = 0; i < 1; i++)
    {
        if(sendto(socketfd2, msg, len+1, 0, (struct sockaddr *)&broadaddr, sizeof(broadaddr)) != len+1)
            xdebug("sendto: " + QString::fromUtf8(strerror(errno)));
    }
    evutil_closesocket(socketfd2);
}

int PXMClient::connectToPeer(evutil_socket_t socketfd, sockaddr_in socketAddr)
{
    int status;
    if( (status = ::connect(socketfd, (struct sockaddr*)&socketAddr, sizeof(socketAddr))) < 0 )
    {
        xdebug("connect:  " + QString::fromUtf8(strerror(errno)));
        emit resultOfConnectionAttempt(socketfd, status);
        evutil_closesocket(socketfd);
        return 1;
    }
    emit resultOfConnectionAttempt(socketfd, status);
    return 0;
}

void PXMClient::sendMsg(evutil_socket_t socketfd, const char *msg, size_t msgLen, const char *type, QUuid uuid, const char *theiruuid)
{
    int bytesSent = 0;
    uint16_t packetLen;
    uint16_t packetLenNBO;
    bool print = false;

    //Combine strings into final message (host): (msg)\0

    if(msgLen > 65400)
    {
        emit resultOfTCPSend(-1, QString::fromUtf8(theiruuid), QString::fromUtf8(msg), print);
    }
    packetLen = PACKED_UUID_BYTE_LENGTH + strlen(type) + msgLen;

    if(!strcmp(type, "/msg") )
    {
        print = true;
    }

    char full_mess[packetLen + 1];

    packetLenNBO = htons(packetLen);
    //packetLenNBO = htons(packetLen + 100);

    packUUID(full_mess, uuid ,0);
    memcpy(full_mess+PACKED_UUID_BYTE_LENGTH, type, strlen(type));
    memcpy(full_mess+PACKED_UUID_BYTE_LENGTH+strlen(type), msg, msgLen);
    full_mess[packetLen] = 0;

    // if(!strcmp(type, "/msg"))
    //    packetLenNBO = htons(1500);

    bytesSent = this->recursiveSend(socketfd, &packetLenNBO, sizeof(uint16_t), 0);

    if(bytesSent != sizeof(uint16_t))
    {
        emit resultOfTCPSend(-1, QString::fromLatin1(theiruuid), QString::fromUtf8(msg), print);
        return;
    }

    bytesSent = this->recursiveSend(socketfd, full_mess, packetLen, 0);

    if(bytesSent >= packetLen)
    {
        bytesSent = 0;
    }
    if(*theiruuid != '\0')
        emit resultOfTCPSend(bytesSent, QString::fromLatin1(theiruuid), QString::fromUtf8(msg), print);
    return;
}
void PXMClient::sendMsgSlot(evutil_socket_t s, QString msg, QString type, QUuid uuid, QString theiruuid)
{
    QByteArray msgB = msg.toUtf8();
    this->sendMsg(s, msgB, msgB.length(), type.toLocal8Bit().constData(), uuid, theiruuid.toLocal8Bit().constData());
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

int PXMClient::recursiveSend(evutil_socket_t socketfd, void *msg, int len, int count)
{
    int status2 = 0;
#ifdef _WIN32
    int status = send(socketfd, (char*)msg, len, 0);
#else
    int status = send(socketfd, msg, len, MSG_NOSIGNAL);
#endif

    if( (status <= 0) )
    {
        xdebug("send on socket " + QString::number(socketfd) + ": " + QString::fromUtf8(strerror(errno)));
        return -1;
    }

    if( ( status != len ) && ( count < 10 ) )
    {
        xdebug("We are partially sending this msg");
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
