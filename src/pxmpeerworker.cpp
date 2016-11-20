#include <pxmpeerworker.h>

PXMPeerWorker::PXMPeerWorker(QObject *parent, QString hostname, QString uuid, PXMServer *server) : QObject(parent)
{
    localHostname = hostname;
    localUUID = uuid;
    realServer = server;
    syncer = new PXMSync(this);
    QObject::connect(syncer, &PXMSync::requestIps, this, &PXMPeerWorker::requestIps);
    QObject::connect(syncer, SIGNAL(finished()), this, SLOT(doneSync()));
    srand (time(NULL));
    syncTimer = new QTimer(this);
    syncTimer->setInterval((rand() % 300000) + 900000);
    QObject::connect(syncTimer, SIGNAL(timeout()), this, SLOT(beginSync()));
    syncTimer->start();
}
PXMPeerWorker::~PXMPeerWorker()
{
    syncTimer->stop();
    if(syncer->isRunning())
    {
        syncer->requestInterruption();
        syncer->quit();
        syncer->wait(5000);
    }
    syncer->deleteLater();
}

void PXMPeerWorker::setLocalHostName(QString name)
{
    localHostname = name;
}
void PXMPeerWorker::setListenerPort(unsigned short port)
{
    ourListenerPort = QString::number(port);
}
void PXMPeerWorker::doneSync()
{
    areWeSyncing = false;
    qDebug() << "Finished Syncing peers";
}

void PXMPeerWorker::beginSync()
{
    if(syncer->isRunning())
        return;
    syncer->setHash(peerDetailsHash);
    qDebug() << "Beginning Sync of connected peers";
    areWeSyncing = true;
    syncer->start();
}
void PXMPeerWorker::hostnameCheck(char *ipHeapArray, size_t len, QUuid senderUuid)
{
    if(senderUuid != waitingOnIpsFrom)
    {
        return;
    }
    qDebug() << "Recieved connection list from" << peerDetailsHash[senderUuid].socketDescriptor;

    size_t index = 0;
    while(index < len)
    {
        sockaddr_in addr;
        QUuid uuid;
        addr.sin_family = AF_INET;
        memcpy(&(addr.sin_addr.s_addr), ipHeapArray+index, sizeof(in_addr_t));
        index += sizeof(in_addr_t);
        memcpy(&(addr.sin_port), ipHeapArray+index, sizeof(in_port_t));
        index += sizeof(in_port_t);
        memcpy(&(uuid.data1), ipHeapArray+index, sizeof(uint32_t));
        index += sizeof(uint32_t);
        uuid.data1 = ntohl(uuid.data1);
        memcpy(&(uuid.data2), ipHeapArray+index, sizeof(uint16_t));
        index += sizeof(uint16_t);
        uuid.data2 = ntohs(uuid.data2);
        memcpy(&(uuid.data3), ipHeapArray+index, sizeof(uint16_t));
        index += sizeof(uint16_t);
        uuid.data3 = ntohs(uuid.data3);
        memcpy(&(uuid.data4), ipHeapArray+index, 8);
        index += 8;
        qDebug() << inet_ntoa(addr.sin_addr);
        qDebug() << uuid;
        attemptConnection(addr, uuid);
    }

    waitingOnIpsFrom = QUuid();
    if(syncer->isRunning())
        syncer->moveOnPlease = true;

    delete [] ipHeapArray;
}
void PXMPeerWorker::newTcpConnection(evutil_socket_t s)
{
    this->sendIdentityMsg(s);
}
void PXMPeerWorker::sendIdentityMsg(evutil_socket_t s)
{
    emit sendMsg(s, localHostname % QStringLiteral(":") % ourListenerPort, QStringLiteral("/uuid"), localUUID, "");
}
void PXMPeerWorker::requestIps(evutil_socket_t s, QUuid uuid)
{
    waitingOnIpsFrom = uuid;
    qDebug() << "Requesting ips from" << s;
    emit sendMsg(s, "", QStringLiteral("/request"), localUUID, "");
}
void PXMPeerWorker::attemptConnection(sockaddr_in addr, QUuid uuid)
{
    if(peerDetailsHash.value(uuid).isConnected || uuid.isNull())
    {
        return;
    }
    evutil_socket_t s = socket(AF_INET, SOCK_STREAM, 0);

    peerDetailsHash[uuid].identifier = uuid;
    peerDetailsHash[uuid].socketDescriptor = s;
    peerDetailsHash[uuid].isConnected = true;
    peerDetailsHash[uuid].ipAddressRaw = addr;

    emit connectToPeer(s, addr);
}
void PXMPeerWorker::setPeerHostname(QString hname, QUuid uuid)
{
    if(peerDetailsHash.contains(uuid))
    {
        peerDetailsHash[uuid].hostname = hname;
        emit updateListWidget(uuid);
    }
    return;
}
void PXMPeerWorker::peerQuit(evutil_socket_t s)
{
    for(auto &itr : peerDetailsHash)
    {
        if(itr.socketDescriptor == s)
        {
            qDebug() << itr.hostname << QStringLiteral("has disconnected, attempting reconnect");
            peerDetailsHash[itr.identifier].isConnected = false;
            peerDetailsHash[itr.identifier].isAuthenticated = false;
            peerDetailsHash[itr.identifier].socketDescriptor = -1;
            peerDetailsHash[itr.identifier].bev = NULL;
            emit setItalicsOnItem(itr.identifier, 1);
            //this->attemptConnection(itr.ipAddressRaw, itr.identifier.toString());
            return;
        }
    }
}
void PXMPeerWorker::sendIps(evutil_socket_t i)
{
    qDebug() << "Sending ips to" << i;
    //QString msgRaw;
    char *msgRaw = new char[peerDetailsHash.size() * (sizeof(in_addr_t) + sizeof(in_port_t) + 38)];
    size_t index = 0;
    for(auto & itr : peerDetailsHash)
    {
        if(itr.isConnected)
        {
            memcpy(msgRaw + index, &(itr.ipAddressRaw.sin_addr.s_addr),sizeof(in_addr_t));
            index += sizeof(in_addr_t);
            memcpy(msgRaw + index, &(itr.ipAddressRaw.sin_port), sizeof(in_port_t));
            index += sizeof(in_port_t);
            uint32_t uuidSectionL = htonl((uint32_t)(itr.identifier.data1));
            memcpy(msgRaw + index, &(uuidSectionL), sizeof(uint32_t));
            index += sizeof(uint32_t);
            uint16_t uuidSectionS = htons((uint16_t)(itr.identifier.data2));
            memcpy(msgRaw + index, &(uuidSectionS), sizeof(uint16_t));
            index += sizeof(uint16_t);
            uuidSectionS = htons((uint16_t)(itr.identifier.data3));
            memcpy(msgRaw + index, &(uuidSectionS), sizeof(uint16_t));
            index += sizeof(uint16_t);
            unsigned char *uuidSectionC = itr.identifier.data4;
            memcpy(msgRaw + index, uuidSectionC, 8);
            index += 8;
        }
    }
    emit sendMsgIps(i, msgRaw, index, "/ip", localUUID, "");
}
void PXMPeerWorker::resultOfConnectionAttempt(evutil_socket_t socket, bool result)
{
    QUuid uuid;
    for(auto &itr : peerDetailsHash)
    {
        if(itr.socketDescriptor == socket)
        {
            uuid = itr.identifier;
        }
    }
    if(uuid.isNull())
        return;
    if(!result)
    {
        struct bufferevent *bev;
        evutil_make_socket_nonblocking(socket);
        bev = bufferevent_socket_new(realServer->base, socket, BEV_OPT_CLOSE_ON_FREE);
        bufferevent_setcb(bev, realServer->tcpReadUUID, NULL, realServer->tcpError, (void*)realServer);
        bufferevent_setwatermark(bev, EV_READ, sizeof(uint16_t), sizeof(uint16_t));
        bufferevent_enable(bev, EV_READ);
        if(peerDetailsHash[uuid].bev != nullptr)
            bufferevent_free(peerDetailsHash[uuid].bev);
        peerDetailsHash[uuid].bev = bev;

        emit sendIdentityMsg(socket);
    }
    else
    {
        peerDetailsHash[uuid].isConnected = false;
    }
}
void PXMPeerWorker::resultOfTCPSend(int levelOfSuccess, QString uuidString, QString msg, bool print)
{
    QUuid uuid = uuidString;
    if(print)
    {
        if(levelOfSuccess < 0)
        {
            msg = QStringLiteral("Message was not sent successfully, Broken Pipe.  Peer likely disconnected");
            peerQuit(peerDetailsHash.value(uuid).socketDescriptor);
        }
        else if(levelOfSuccess > 0)
        {
            msg.append(QStringLiteral("\nThe previous message was only paritally sent.  This was very bad\nContact the administrator of this program immediately\nNumber of bytes sent: ") % QString::number(levelOfSuccess));
            qDebug() << "Partial Send";
        }
        emit printToTextBrowser(msg, uuid, 0);

        return;
    }
    if(levelOfSuccess<0)
        peerQuit(peerDetailsHash.value(uuid).socketDescriptor);
}
/**
 * @brief				This is the function called when mess_discover recieves a udp packet starting with "/name:"
 * 						here we check to see if the ip of the selected host is already in the peers array and if not
 * 						add it.  peers is then sorted and displayed to the gui.  QStrings are used for ease of
 * 						passing through the QT signal and slots mechanism.
 * @param hname			Hostname of peer to compare to existing hostnames
 * @param ipaddr		IP address of peer to compare to existing IP addresses
 */
void PXMPeerWorker::authenticationRecieved(QString hname, QString port, evutil_socket_t s, QUuid uuid, void *bevptr)
{
    if(uuid.isNull())
    {
        evutil_closesocket(s);
        return;
    }
    else if(peerDetailsHash.contains(uuid) && peerDetailsHash.value(uuid).isAuthenticated)
    {
        return;
    }
    bufferevent *bev = (bufferevent*)bevptr;
    struct sockaddr_in addr;
    socklen_t socklen = sizeof(addr);
    memset(&addr, 0, socklen);

    getpeername(s, (struct sockaddr*)&addr, &socklen);
    addr.sin_port = htons(port.toUInt());

    qDebug() << "hostname:" << hname << "on port" << port << "authenticated!";

    peerDetailsHash[uuid].socketDescriptor = s;
    peerDetailsHash[uuid].isConnected = true;
    peerDetailsHash[uuid].isAuthenticated = true;
    peerDetailsHash[uuid].identifier = uuid;
    peerDetailsHash[uuid].hostname = hname;
    peerDetailsHash[uuid].bev = bev;
    peerDetailsHash[uuid].ipAddressRaw = addr;

    emit updateListWidget(uuid);
    emit requestIps(s, uuid);
    emit setItalicsOnItem(uuid, 0);
}
