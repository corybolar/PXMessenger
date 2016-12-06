#include <pxmpeerworker.h>

PXMPeerWorker::PXMPeerWorker(QObject *parent, QString hostname, QUuid uuid, PXMServer *server) : QObject(parent)
{
    //Init
    areWeSyncing = false;
    waitingOnSyncFrom = QUuid();
    //End of Init

    localHostname = hostname;
    localUUID = uuid;

    //Prevent race condition when starting threads, a bufferevent
    //for this (us) is coming soon.
    peerDetailsHash[localUUID].identifier = localUUID;
    peerDetailsHash[localUUID].hostname = localHostname;
    peerDetailsHash[localUUID].socketDescriptor = -1;
    peerDetailsHash[localUUID].isConnected = true;
    peerDetailsHash[localUUID].isAuthenticated = false;

    realServer = server;
}

PXMPeerWorker::~PXMPeerWorker()
{
    syncTimer->stop();
    nextSyncTimer->stop();
    for(auto &itr : peerDetailsHash)
    {
        qDeleteAll(itr.messages);
    }

    int count = 0;
    for(auto &itr : peerDetailsHash)
    {
        if(itr.bw->getBev() != nullptr)
        {
            bufferevent_free(itr.bw->getBev());
            count++;
        }
        delete itr.bw;
        evutil_closesocket(itr.socketDescriptor);
    }
    qDebug() << "freed" << count << "bufferevents";

    count = 0;
    for(auto &itr : extraBufferevents)
    {
        bufferevent_free(itr);
        count++;
    }
    qDeleteAll(bwShortLife);
    qDebug() << "freed" << count << "extra bufferevents";
    qDebug() << "Shutdown of PXMPeerWorker Successful";
}
void PXMPeerWorker::currentThreadInit()
{
    syncer = new PXMSync(this);
    QObject::connect(syncer, &PXMSync::requestIps, this, &PXMPeerWorker::requestIps);
    QObject::connect(syncer, &PXMSync::syncComplete, this, &PXMPeerWorker::doneSync);

    srand (time(NULL));

    syncTimer = new QTimer(this);
    syncTimer->setInterval((rand() % 300000) + 900000);
    QObject::connect(syncTimer, &QTimer::timeout, this, &PXMPeerWorker::beginSync);
    syncTimer->start();

    nextSyncTimer = new QTimer(this);
    nextSyncTimer->setInterval(2000);
    QObject::connect(nextSyncTimer, &QTimer::timeout, syncer, &PXMSync::syncNext);
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
    nextSyncTimer->stop();
    qDebug() << "Finished Syncing peers";
}

void PXMPeerWorker::beginSync()
{
    if(areWeSyncing)
        return;

    qDebug() << "Beginning Sync of connected peers";
    areWeSyncing = true;
    syncer->setsyncHash(&peerDetailsHash);
    syncer->setIteratorToStart();
    syncer->syncNext();
    nextSyncTimer->start();
}
void PXMPeerWorker::syncPacketIterator(char *ipHeapArray, size_t len, QUuid senderUuid)
{
    if(senderUuid != waitingOnSyncFrom)
    {
        delete [] ipHeapArray;
        return;
    }
    qDebug() << "Recieved connection list from" << peerDetailsHash[senderUuid].socketDescriptor;

    size_t index = 0;
    while(index + PACKED_UUID_BYTE_LENGTH + 6 <= len)
    {
        sockaddr_in addr;
        addr.sin_family = AF_INET;
        memcpy(&(addr.sin_addr.s_addr), ipHeapArray+index, sizeof(uint32_t));
        index += sizeof(uint32_t);
        memcpy(&(addr.sin_port), ipHeapArray+index, sizeof(uint16_t));
        index += sizeof(uint16_t);
        QUuid uuid = PXMServer::unpackUUID((unsigned char*)ipHeapArray+index);
        index += PACKED_UUID_BYTE_LENGTH;

        qDebug() << inet_ntoa(addr.sin_addr) << ":" << ntohs(addr.sin_port) << ":" << uuid.toString();
        attemptConnection(addr, uuid);
    }

    waitingOnSyncFrom = QUuid();
    if(areWeSyncing)
    {
        nextSyncTimer->start(2000);
        syncer->syncNext();
    }

    delete [] ipHeapArray;
}
void PXMPeerWorker::newTcpConnection(bufferevent *bev)
{
    //QSharedPointer<BevWrapper> bws = QSharedPointer<BevWrapper>(new BevWrapper(bev));
    BevWrapper *bw = new BevWrapper(bev);
    bwShortLife.append(bw);
    this->sendIdentityMsg(bw);
}
void PXMPeerWorker::sendIdentityMsg(BevWrapper *bw)
{
    emit sendMsg(bw, (localHostname % QStringLiteral("::::") % ourListenerPort).toUtf8(), "/uuid", localUUID, "");
}
void PXMPeerWorker::requestIps(BevWrapper *bw, QUuid uuid)
{
    waitingOnSyncFrom = uuid;
    qDebug() << "Requesting ips from" << peerDetailsHash.value(uuid).hostname;
    emit sendMsg(bw, "", "/request", localUUID, "");
}
void PXMPeerWorker::attemptConnection(sockaddr_in addr, QUuid uuid)
{
    if(peerDetailsHash.value(uuid).isConnected || uuid.isNull())
    {
        return;
    }

    evutil_socket_t s = socket(AF_INET, SOCK_STREAM, 0);
    evutil_make_socket_nonblocking(s);
    struct bufferevent *bev = bufferevent_socket_new(PXMServer::base, s, BEV_OPT_THREADSAFE);

    peerDetailsHash[uuid].identifier = uuid;
    peerDetailsHash[uuid].socketDescriptor = s;
    peerDetailsHash[uuid].isConnected = true;
    peerDetailsHash[uuid].ipAddressRaw = addr;

    emit connectToPeer(s, addr, bev);
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
void PXMPeerWorker::peerQuit(evutil_socket_t s, bufferevent *bev)
{
    for(auto &itr : peerDetailsHash)
    {
        if(itr.socketDescriptor == s)
        {
            peerDetailsHash[itr.identifier].isConnected = false;
            peerDetailsHash[itr.identifier].isAuthenticated = false;
            peerDetailsHash[itr.identifier].bw->lockBev();
            peerDetailsHash[itr.identifier].bw->setBev(nullptr);
            bufferevent_free(bev);
            peerDetailsHash[itr.identifier].bw->unlockBev();
            evutil_closesocket(itr.socketDescriptor);
            peerDetailsHash[itr.identifier].socketDescriptor = -1;
            emit setItalicsOnItem(itr.identifier, 1);
            return;
        }
    }
    for(auto &itr : extraBufferevents)
    {
        if(itr == bev)
        {
            extraBufferevents.removeAll(itr);
            bufferevent_free(bev);
            evutil_closesocket(s);
            return;
        }
    }
    bufferevent_free(bev);
    evutil_closesocket(s);
}
void PXMPeerWorker::sendIpsBev(bufferevent *bev, QUuid uuid)
{
    if(peerDetailsHash.contains(uuid) && peerDetailsHash.value(uuid).bw->getBev() == bev)
    {
        this->sendIps(peerDetailsHash.value(uuid).bw, uuid);
    }
    else
    {
        qDebug() << "sendIpsBev:error";
    }
}

void PXMPeerWorker::sendIps(BevWrapper *bw, QUuid uuid)
{
    qDebug() << "Sending ips to" << peerDetailsHash.value(uuid).hostname;
    char *msgRaw = new char[peerDetailsHash.size() * (sizeof(uint32_t) + sizeof(uint16_t) + PACKED_UUID_BYTE_LENGTH) + 1];
    //size_t index = 1;
    size_t index = 0;
    //msgRaw[0] = 0;
    for(auto & itr : peerDetailsHash)
    {
        if(itr.isAuthenticated)
        {
            memcpy(msgRaw + index, &(itr.ipAddressRaw.sin_addr.s_addr),sizeof(uint32_t));
            index += sizeof(uint32_t);
            memcpy(msgRaw + index, &(itr.ipAddressRaw.sin_port), sizeof(uint16_t));
            index += sizeof(uint16_t);
            index += PXMClient::packUuid(msgRaw + index, &(itr.identifier));
        }
    }
    //emit sendMsg(i, QByteArray::fromRawData(msgRaw, index), "/ip", localUUID, "");
    msgRaw[index+1] = 0;

    emit sendIpsPacket(bw, msgRaw, index, "/ip", localUUID, "");
}

void PXMPeerWorker::resultOfConnectionAttempt(evutil_socket_t socket, bool result, bufferevent *bev)
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

    if(result)
    {
        bufferevent_setcb(bev, PXMServer::tcpReadUUID, NULL, PXMServer::tcpError, (void*)realServer);
        bufferevent_setwatermark(bev, EV_READ, sizeof(uint16_t), sizeof(uint16_t));
        bufferevent_enable(bev, EV_READ|EV_WRITE);
        peerDetailsHash[uuid].bw->lockBev();
        if(peerDetailsHash[uuid].bw->getBev() != nullptr)
            bufferevent_free(peerDetailsHash[uuid].bw->getBev());
        peerDetailsHash[uuid].bw->setBev(bev);
        peerDetailsHash[uuid].bw->unlockBev();

        emit sendIdentityMsg(peerDetailsHash.value(uuid).bw);
    }
    else
    {
        peerDetailsHash[uuid].bw->lockBev();
        peerDetailsHash[uuid].bw->setBev(nullptr);
        bufferevent_free(bev);
        evutil_closesocket(socket);
        peerDetailsHash[uuid].bw->unlockBev();
        peerDetailsHash[uuid].isConnected = false;
        peerDetailsHash[uuid].isAuthenticated = false;
        peerDetailsHash[uuid].socketDescriptor = -1;
    }
}
void PXMPeerWorker::resultOfTCPSend(int levelOfSuccess, QUuid uuid, QString msg, bool print, BevWrapper *bw)
{
    if(print)
    {
        if(levelOfSuccess < 0)
        {
            msg = QStringLiteral("Message was not sent successfully, Broken Pipe.  Peer is disconnected");
        }
        else if(levelOfSuccess > 0)
        {
            msg.prepend(localHostname % ": ");
            msg.append(QStringLiteral("\nThe previous message was only paritally sent.  This was very bad\nContact the administrator of this program immediately\nNumber of bytes sent: ") % QString::number(levelOfSuccess));
            qDebug() << "Partial Send";
        }
        else
            msg.prepend(localHostname % ": ");
        emit printToTextBrowser(msg, uuid, 0, true);
    }
    if(bwShortLife.contains(bw))
    {
        delete bw;
    }
}
void PXMPeerWorker::authenticationReceived(QString hname, unsigned short port, evutil_socket_t s, QUuid uuid, bufferevent *bev)
{
    if(uuid.isNull())
    {
        evutil_closesocket(s);
        bufferevent_free(bev);
        return;
    }
    struct sockaddr_in addr;
    socklen_t socklen = sizeof(addr);
    memset(&addr, 0, socklen);

    getpeername(s, (struct sockaddr*)&addr, &socklen);
    addr.sin_port = htons(port);

    qDebug() << hname << "on port" << QString::number(port) << "authenticated!";

    if(peerDetailsHash[uuid].bw->getBev() != nullptr && peerDetailsHash[uuid].bw->getBev() != bev)
    {
        extraBufferevents.append(peerDetailsHash[uuid].bw->getBev());
    }

    peerDetailsHash[uuid].bw->lockBev();
    peerDetailsHash[uuid].bw->setBev(bev);
    peerDetailsHash[uuid].bw->unlockBev();
    peerDetailsHash[uuid].socketDescriptor = s;
    peerDetailsHash[uuid].isConnected = true;
    peerDetailsHash[uuid].isAuthenticated = true;
    peerDetailsHash[uuid].identifier = uuid;
    peerDetailsHash[uuid].hostname = hname;
    peerDetailsHash[uuid].ipAddressRaw = addr;

    emit updateListWidget(uuid);
    emit requestIps(peerDetailsHash.value(uuid).bw, uuid);
}
