#include <pxmclient.h>

PXMClient::PXMClient(QObject *parent, in_addr multicast) : QObject(parent)
{
    this->setObjectName("PXMClient");

    multicastAddress = multicast;
}
int PXMClient::sendUDP(const char* msg, unsigned short port)
{
    int len;
    struct sockaddr_in broadaddr;
    evutil_socket_t socketfd2;

    memset(&broadaddr, 0, sizeof(broadaddr));
    broadaddr.sin_family = AF_INET;
    broadaddr.sin_addr = multicastAddress;
    broadaddr.sin_port = htons(port);

    if ( (socketfd2 = (socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))) < 0)
    {
        qDebug() << "socket: " + QString::fromUtf8(strerror(errno));
        return -1;
    }

    len = strlen(msg);

    char loopback = 1;
    if(setsockopt(socketfd2, IPPROTO_IP, IP_MULTICAST_LOOP, &loopback, sizeof(loopback)) < 0)
    {
        qDebug() << "setsockopt: " + QString::fromUtf8(strerror(errno));
        evutil_closesocket(socketfd2);
        return -1;
    }

    for(int i = 0; i < 1; i++)
    {
        if(sendto(socketfd2, msg, len+1, 0, (struct sockaddr *)&broadaddr, sizeof(broadaddr)) != len+1)
        {
            qDebug() << "sendto: " + QString::fromUtf8(strerror(errno));
            evutil_closesocket(socketfd2);
            return -1;
        }
    }
    evutil_closesocket(socketfd2);
    return 0;
}
void PXMClient::connectCB(struct bufferevent *bev, short event, void *arg)
{
    PXMClient *realClient = static_cast<PXMClient*>(arg);
    if(event & BEV_EVENT_CONNECTED)
        realClient->resultOfConnectionAttempt(bufferevent_getfd(bev), true, bev);
    else
        realClient->resultOfConnectionAttempt(bufferevent_getfd(bev), false, bev);
}

void PXMClient::connectToPeer(evutil_socket_t, sockaddr_in socketAddr, bufferevent *bev)
{
    bufferevent_setcb(bev, NULL, NULL, PXMClient::connectCB, (void*)this);
    timeval timeout = {5,0};
    bufferevent_set_timeouts(bev, &timeout, &timeout);
    bufferevent_socket_connect(bev, (struct sockaddr*)&socketAddr, sizeof(socketAddr));
}

void PXMClient::sendMsg(BevWrapper *bw, const char *msg, size_t msgLen, PXMConsts::MESSAGE_TYPE type, QUuid uuidSender, QUuid uuidReceiver)
{
    int bytesSent = 0;
    int packetLen;
    uint16_t packetLenNBO;
    bool print = false;

    if(msgLen > 65400)
    {
        emit resultOfTCPSend(-1, uuidReceiver, QString("Message too Long!"), print, bw);
        return;
    }
    packetLen = UUIDCompression::PACKED_UUID_BYTE_LENGTH + sizeof(uint8_t) + msgLen;

    if(type == PXMConsts::MSG_TEXT)
        print = true;

    char full_mess[packetLen + 1];

    packetLenNBO = htons(packetLen);

    UUIDCompression::packUUID(full_mess, &uuidSender);
    memcpy(full_mess+UUIDCompression::PACKED_UUID_BYTE_LENGTH, &type, sizeof(uint8_t));
    memcpy(full_mess+UUIDCompression::PACKED_UUID_BYTE_LENGTH+sizeof(uint8_t), msg, msgLen);
    full_mess[packetLen] = 0;

    bw->lockBev();

    if((bw->getBev() == nullptr) || !(bufferevent_get_enabled(bw->getBev()) & EV_WRITE))
    {
        bw->unlockBev();
        emit resultOfTCPSend(-1, uuidReceiver, QStringLiteral("Peer is Disconnected, message not sent"), print, bw);
        return;
    }

    if(bufferevent_write(bw->getBev(), &packetLenNBO, sizeof(uint16_t)) == 0)
    {
        bytesSent = bufferevent_write(bw->getBev(), full_mess, packetLen);

        bw->unlockBev();
    }
    else
    {
        bytesSent = -1;
        msg = "Message send failure, not sent\0";
    }

    if(!uuidReceiver.isNull())
    {
        emit resultOfTCPSend(bytesSent, uuidReceiver, QString::fromUtf8(msg), print, bw);
    }

    return;
}
void PXMClient::sendMsgSlot(BevWrapper *bw, QByteArray msg, PXMConsts::MESSAGE_TYPE type, QUuid uuid, QUuid theiruuid)
{
    this->sendMsg(bw, msg.constData(), msg.length(), type, uuid, theiruuid);
}

void PXMClient::sendIpsSlot(BevWrapper *bw, char *msg, size_t len, PXMConsts::MESSAGE_TYPE type, QUuid uuid, QUuid theiruuid)
{
    this->sendMsg(bw, msg, len, type, uuid, theiruuid);
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
        qDebug() << "send on socket " + QString::number(socketfd) + ": " + QString::fromUtf8(strerror(errno));
        return -1;
    }

    if( ( status != len ) && ( count < 10 ) )
    {
        qDebug() << "We are partially sending this msg";
        int len2 = len - status;
        //uint8_t cast to avoid compiler warning, we want to advance the pointer the number of bytes in status
        msg = (void*)((char*)msg + status);
        count++;

        status2 = recursiveSend(socketfd, msg, len2, count);
        if(status2 <= 0)
            return -1;
    }
    return status + status2;
}
