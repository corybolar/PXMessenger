#include <pxmpeerworker.h>
using namespace PXMConsts;

PXMPeerWorker::PXMPeerWorker(QObject *parent, initialSettings presets, QUuid globaluuid) : QObject(parent)
{
    //Init
    areWeSyncing = false;
    //waitingOnSyncFrom = QUuid();
    //End of Init

    localHostname = presets.username;
    localUUID = presets.uuid;
    globalUUID = globaluuid;
    multicastAddress = presets.multicast;

    //Prevent race condition when starting threads, a bufferevent
    //for this (us) is coming soon.
    peerDetails selfComms;
    selfComms.identifier=localUUID;
    selfComms.hostname = localHostname;
    selfComms.connectTo = true;
    selfComms.isAuthenticated = false;
    selfComms.bw = new BevWrapper();
    peerDetailsHash.insert(localUUID, selfComms);

    peerDetails globalPeer;
    globalPeer.identifier = globalUUID;
    globalPeer.hostname = "Global Chat";
    globalPeer.connectTo = true;
    globalPeer.isAuthenticated = false;
    globalPeer.bw = new BevWrapper();
    peerDetailsHash.insert(globalUUID, globalPeer);

    in_addr multicast_in_addr;
    multicast_in_addr.s_addr = inet_addr(multicastAddress.toLatin1().constData());
    messServer = new PXMServer(this, presets.tcpPort, presets.udpPort, multicast_in_addr);
}
PXMPeerWorker::~PXMPeerWorker()
{
    qDebug() << "PXMPeerWorker destructor";
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
        evutil_closesocket(itr.socket);
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

    if(messServer != 0 && messServer->isRunning())
    {
        bufferevent_write(closeBev, "/exit", 5);
        messServer->wait(5000);
    }

    qDebug() << "Shutdown of PXMPeerWorker Successful";
}
void PXMPeerWorker::setCloseBufferevent(bufferevent *bev)
{
    closeBev = bev;
}
void PXMPeerWorker::currentThreadInit()
{
    syncer = new PXMSync(this);
    QObject::connect(syncer, &PXMSync::requestIps, this, &PXMPeerWorker::requestSyncPacket);
    QObject::connect(syncer, &PXMSync::syncComplete, this, &PXMPeerWorker::doneSync);

    srand (time(NULL));

    syncTimer = new QTimer(this);
    syncTimer->setInterval((rand() % 300000) + SYNC_TIMER);
    QObject::connect(syncTimer, &QTimer::timeout, this, &PXMPeerWorker::beginSync);
    syncTimer->start();

    nextSyncTimer = new QTimer(this);
    nextSyncTimer->setInterval(2000);
    QObject::connect(nextSyncTimer, &QTimer::timeout, syncer, &PXMSync::syncNext);

    QTimer::singleShot(10000, this, SLOT(discoveryTimerSingleShot()));

    midnightTimer = new QTimer(this);
    midnightTimer->setInterval(MIDNIGHT_TIMER_INTERVAL_MINUTES * 60000);
    QObject::connect(midnightTimer, &QTimer::timeout, this, &PXMPeerWorker::midnightTimerPersistent);
    midnightTimer->start();

    discoveryTimer= new QTimer(this);

    startClient();
    startServerThread();
}
void PXMPeerWorker::startServerThread()
{
    messServer->setLocalHostname(localHostname);
    messServer->setLocalUUID(localUUID);
    QObject::connect(messServer, &PXMServer::authenticationReceived, this, &PXMPeerWorker::authenticationReceived, Qt::QueuedConnection);
    QObject::connect(messServer, &PXMServer::messageRecieved, this, &PXMPeerWorker::recieveServerMessage, Qt::QueuedConnection);
    QObject::connect(messServer, &QThread::finished, messServer, &QObject::deleteLater);
    QObject::connect(messServer, &PXMServer::newTCPConnection, this, &PXMPeerWorker::newTcpConnection, Qt::QueuedConnection);
    QObject::connect(messServer, &PXMServer::peerQuit, this, &PXMPeerWorker::peerQuit, Qt::QueuedConnection);
    QObject::connect(messServer, &PXMServer::attemptConnection, this, &PXMPeerWorker::attemptConnection, Qt::QueuedConnection);
    QObject::connect(messServer, &PXMServer::sendSyncPacket, this, &PXMPeerWorker::sendSyncPacketBev, Qt::QueuedConnection);
    QObject::connect(messServer, &PXMServer::syncPacketIterator, this, &PXMPeerWorker::syncPacketIterator, Qt::QueuedConnection);
    QObject::connect(messServer, &PXMServer::setPeerHostname, this, &PXMPeerWorker::setPeerHostname, Qt::QueuedConnection);
    QObject::connect(messServer, &PXMServer::setListenerPorts, this, &PXMPeerWorker::setListenerPorts, Qt::QueuedConnection);
    QObject::connect(messServer, &PXMServer::libeventBackend, this, &PXMPeerWorker::setlibeventBackend, Qt::QueuedConnection);
    QObject::connect(messServer, &PXMServer::setCloseBufferevent, this, &PXMPeerWorker::setCloseBufferevent, Qt::QueuedConnection);
    QObject::connect(messServer, &PXMServer::setSelfCommsBufferevent, this, &PXMPeerWorker::setSelfCommsBufferevent, Qt::QueuedConnection);
    QObject::connect(messServer, &PXMServer::multicastIsFunctional, this, &PXMPeerWorker::multicastIsFunctional, Qt::QueuedConnection);
    QObject::connect(messServer, &PXMServer::serverSetupFailure, this, &PXMPeerWorker::serverSetupFailure, Qt::QueuedConnection);
    QObject::connect(messServer, &PXMServer::sendUDP, messClient, &PXMClient::sendUDP, Qt::QueuedConnection);
    messServer->start();
}
void PXMPeerWorker::startClient()
{
    in_addr multicast_in_addr;
    multicast_in_addr.s_addr = inet_addr(multicastAddress.toLatin1().constData());
    messClient = new PXMClient(this, multicast_in_addr);

    QObject::connect(messClient, &PXMClient::resultOfConnectionAttempt, this, &PXMPeerWorker::resultOfConnectionAttempt, Qt::AutoConnection);
    QObject::connect(messClient, &PXMClient::resultOfTCPSend, this, &PXMPeerWorker::resultOfTCPSend, Qt::AutoConnection);
    QObject::connect(this, &PXMPeerWorker::sendMsg, messClient, &PXMClient::sendMsgSlot, Qt::AutoConnection);
    QObject::connect(this, &PXMPeerWorker::connectToPeer, messClient, &PXMClient::connectToPeer, Qt::AutoConnection);
    QObject::connect(this, &PXMPeerWorker::sendIpsPacket, messClient, &PXMClient::sendIpsSlot, Qt::AutoConnection);
}

void PXMPeerWorker::setListenerPorts(unsigned short tcpport, unsigned short udpport)
{
    ourListenerPort = QString::number(tcpport);
    ourUDPListenerPort = QString::number(udpport);
}
void PXMPeerWorker::doneSync()
{
    areWeSyncing = false;
    nextSyncTimer->stop();
    qDebug() << "Finished Syncing peers";
}
void PXMPeerWorker::sendUDPAccessor(const char *msg, unsigned short port)
{
    emit sendUDP(msg, port);
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
    if(!syncablePeers.contains(senderUuid))
    {
        delete [] ipHeapArray;
        return;
    }
    qDebug() << "Recieved connection list from" << peerDetailsHash[senderUuid].socket;

    size_t index = 0;
    while(index + UUIDCompression::PACKED_UUID_BYTE_LENGTH + 6 <= len)
    {
        sockaddr_in addr;
        addr.sin_family = AF_INET;
        memcpy(&(addr.sin_addr.s_addr), &ipHeapArray[index], sizeof(uint32_t));
        index += sizeof(uint32_t);
        memcpy(&(addr.sin_port), &ipHeapArray[index], sizeof(uint16_t));
        index += sizeof(uint16_t);
        QUuid uuid = UUIDCompression::unpackUUID((unsigned char*)&ipHeapArray[index]);
        index += UUIDCompression::PACKED_UUID_BYTE_LENGTH;

        qDebug() << inet_ntoa(addr.sin_addr) << ":" << ntohs(addr.sin_port) << ":" << uuid.toString();
        attemptConnection(addr, uuid);
    }
    qDebug() << "End of sync packet";

    syncablePeers.remove(senderUuid);

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
    this->sendAuthPacket(bw);
}
void PXMPeerWorker::sendAuthPacket(BevWrapper *bw)
{
    emit sendMsg(bw, (localHostname % QStringLiteral("::::") % ourListenerPort).toUtf8(), MSG_UUID, localUUID, "");
}
void PXMPeerWorker::requestSyncPacket(BevWrapper *bw, QUuid uuid)
{
    syncablePeers.append(uuid);
    qDebug() << "Requesting ips from" << peerDetailsHash.value(uuid).hostname;
    emit sendMsg(bw, "", MSG_SYNC_REQUEST, localUUID, "");
}
void PXMPeerWorker::attemptConnection(sockaddr_in addr, QUuid uuid)
{
    if(peerDetailsHash.value(uuid).connectTo || uuid.isNull())
    {
        return;
    }

    evutil_socket_t s = socket(AF_INET, SOCK_STREAM, 0);
    evutil_make_socket_nonblocking(s);
    struct bufferevent *bev = bufferevent_socket_new(PXMServer::base, s, BEV_OPT_THREADSAFE);

    peerDetailsHash[uuid].identifier = uuid;
    peerDetailsHash[uuid].socket = s;
    peerDetailsHash[uuid].connectTo = true;
    peerDetailsHash[uuid].ipAddressRaw = addr;

    emit connectToPeer(s, addr, bev);
}
void PXMPeerWorker::setPeerHostname(QString hname, QUuid uuid)
{
    if(peerDetailsHash.contains(uuid))
    {
        peerDetailsHash[uuid].hostname = hname;
        emit updateListWidget(uuid, peerDetailsHash.value(uuid).hostname);
    }
    return;
}
void PXMPeerWorker::peerQuit(evutil_socket_t s, bufferevent *bev)
{
    for(auto &itr : peerDetailsHash)
    {
        if(itr.socket == s)
        {
            peerDetailsHash[itr.identifier].connectTo = false;
            peerDetailsHash[itr.identifier].isAuthenticated = false;
            peerDetailsHash[itr.identifier].bw->lockBev();
            qDebug().noquote() << "Peer:" << itr.identifier.toString() << "has disconnected";
            peerDetailsHash[itr.identifier].bw->setBev(nullptr);
            bufferevent_free(bev);
            peerDetailsHash[itr.identifier].bw->unlockBev();
            evutil_closesocket(itr.socket);
            peerDetailsHash[itr.identifier].socket = -1;
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
    qDebug().noquote() << "Non-Authed Peer has quit";
    bufferevent_free(bev);
    evutil_closesocket(s);
}
void PXMPeerWorker::sendSyncPacketBev(bufferevent *bev, QUuid uuid)
{
    if(peerDetailsHash.contains(uuid) && peerDetailsHash.value(uuid).bw->getBev() == bev)
    {
        this->sendSyncPacket(peerDetailsHash.value(uuid).bw, uuid);
    }
    else
    {
        qDebug() << "sendIpsBev:error";
    }
}
void PXMPeerWorker::sendSyncPacket(BevWrapper *bw, QUuid uuid)
{
    qDebug() << "Sending ips to" << peerDetailsHash.value(uuid).hostname;
    char *msgRaw = new char[peerDetailsHash.size() * (sizeof(uint32_t) + sizeof(uint16_t) + UUIDCompression::PACKED_UUID_BYTE_LENGTH) + 1];
    size_t index = 0;
    for(auto & itr : peerDetailsHash)
    {
        if(itr.isAuthenticated)
        {
            memcpy(&msgRaw[index], &(itr.ipAddressRaw.sin_addr.s_addr),sizeof(uint32_t));
            index += sizeof(uint32_t);
            memcpy(&msgRaw[index], &(itr.ipAddressRaw.sin_port), sizeof(uint16_t));
            index += sizeof(uint16_t);
            index += UUIDCompression::packUUID(&msgRaw[index], &(itr.identifier));
        }
    }
    msgRaw[index+1] = 0;

    emit sendIpsPacket(bw, msgRaw, index, MSG_SYNC, localUUID, "");
}
void PXMPeerWorker::resultOfConnectionAttempt(evutil_socket_t socket, bool result, bufferevent *bev)
{
    QUuid uuid;
    for(auto &itr : peerDetailsHash)
    {
        if(itr.socket == socket)
        {
            uuid = itr.identifier;
        }
    }
    if(uuid.isNull())
    {
        return;
    }

    if(result)
    {
        bufferevent_setcb(bev, PXMServer::tcpReadUUID, NULL, PXMServer::tcpError, messServer);
        bufferevent_setwatermark(bev, EV_READ, sizeof(uint16_t), sizeof(uint16_t));
        bufferevent_enable(bev, EV_READ|EV_WRITE);
        if(peerDetailsHash[uuid].bw == nullptr)
            peerDetailsHash[uuid].bw = new BevWrapper();
        peerDetailsHash[uuid].bw->lockBev();
        if(peerDetailsHash[uuid].bw->getBev() != nullptr)
            bufferevent_free(peerDetailsHash[uuid].bw->getBev());
        peerDetailsHash[uuid].bw->setBev(bev);
        peerDetailsHash[uuid].bw->unlockBev();

        emit sendAuthPacket(peerDetailsHash.value(uuid).bw);
    }
    else
    {
        peerDetailsHash[uuid].bw->lockBev();
        peerDetailsHash[uuid].bw->setBev(nullptr);
        bufferevent_free(bev);
        evutil_closesocket(socket);
        peerDetailsHash[uuid].bw->unlockBev();
        peerDetailsHash[uuid].connectTo = false;
        peerDetailsHash[uuid].isAuthenticated = false;
        peerDetailsHash[uuid].socket = -1;
    }
}
void PXMPeerWorker::resultOfTCPSend(int levelOfSuccess, QUuid uuid, QString msg, bool print, BevWrapper *bw)
{
    if(print)
    {
        if(levelOfSuccess == 0)
        {
            msg.prepend(localHostname % ": ");
        }
        else if(levelOfSuccess < 0)
        {
            qDebug() << "Send Failure";
        }
        addMessageToPeer(msg, uuid, false, true);
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

    qDebug().noquote() << hname << "on port" << QString::number(port) << "authenticated!";

    if(peerDetailsHash[uuid].bw == nullptr)
    {
        peerDetailsHash[uuid].bw = new BevWrapper();
    }
    else if(peerDetailsHash[uuid].bw->getBev() != nullptr && peerDetailsHash[uuid].bw->getBev() != bev)
    {
        extraBufferevents.append(peerDetailsHash[uuid].bw->getBev());
    }

    peerDetailsHash[uuid].bw->lockBev();
    peerDetailsHash[uuid].bw->setBev(bev);
    peerDetailsHash[uuid].bw->unlockBev();
    peerDetailsHash[uuid].socket = s;
    peerDetailsHash[uuid].connectTo = true;
    peerDetailsHash[uuid].isAuthenticated = true;
    peerDetailsHash[uuid].identifier = uuid;
    peerDetailsHash[uuid].hostname = hname;
    peerDetailsHash[uuid].ipAddressRaw = addr;

    emit updateListWidget(uuid, peerDetailsHash.value(uuid).hostname);
    emit requestSyncPacket(peerDetailsHash.value(uuid).bw, uuid);
}
int PXMPeerWorker::recieveServerMessage(QString str, QUuid uuid, bufferevent *bev, bool global)
{
    if(uuid != localUUID)
    {
        if( !( peerDetailsHash.contains(uuid) ) )
        {
            qDebug() << "Message from invalid uuid, rejection";
            return -1;
        }

        if(peerDetailsHash.value(uuid).bw->getBev() != bev)
        {
            bool foundIt = false;
            QUuid uuidSpoofer = "";
            for(auto &itr : peerDetailsHash)
            {
                if(itr.bw->getBev() == bev)
                {
                    foundIt = true;
                    uuidSpoofer = itr.identifier;
                    continue;
                }
            }
            if(foundIt)
                addMessageToPeer("This user is trying to spoof another users uuid!", uuidSpoofer, true, false);
            else
                addMessageToPeer("Someone is trying to spoof this users uuid!", uuid, true, false);

            return -1;
        }
    }

    str.prepend(peerDetailsHash.value(uuid).hostname % ": ");

    if(global)
    {
        uuid = globalUUID;
    }

    addMessageToPeer(str, uuid, true, true);
    return 0;
}
void PXMPeerWorker::addMessageToAllPeers(QString str, bool alert, bool formatAsMessage)
{
    for(auto &itr : peerDetailsHash)
        addMessageToPeer(str, itr.identifier, alert, formatAsMessage);
}

int PXMPeerWorker::addMessageToPeer(QString str, QUuid uuid, bool alert, bool formatAsMessage)
{
    if(!peerDetailsHash.contains(uuid))
    {
        return -1;
    }

    if(formatAsMessage)
    {
        QDateTime dt = QDateTime::currentDateTime();
        str = QStringLiteral("(") % dt.time().toString("hh:mm:ss") % QStringLiteral(") ") % str;
    }

    peerDetailsHash[uuid].messages.append(new QString(str.toUtf8()));
    if(peerDetailsHash[uuid].messages.size() > MESSAGE_HISTORY_LENGTH)
    {
        delete peerDetailsHash[uuid].messages.takeFirst();
    }
    emit printToTextBrowser(str, uuid, alert);
    return 0;
}
void PXMPeerWorker::setSelfCommsBufferevent(bufferevent *bev)
{
    peerDetailsHash.value(localUUID).bw->lockBev();
    peerDetailsHash.value(localUUID).bw->setBev(bev);
    peerDetailsHash.value(localUUID).bw->unlockBev();

    updateListWidget(localUUID, localHostname);
}
void PXMPeerWorker::sendMsgAccessor(QByteArray msg, MESSAGE_TYPE type, QUuid uuid1, QUuid uuid2)
{
    switch (type) {
    case MSG_TEXT:
        emit sendMsg(peerDetailsHash.value(uuid2).bw, msg, MSG_TEXT, uuid1, uuid2);
        break;
    case MSG_GLOBAL:
        for(auto &itr : peerDetailsHash)
        {
            if(itr.connectTo)
                emit sendMsg(itr.bw, msg, MSG_GLOBAL, uuid1, uuid2);
        }
        break;
    default:
        qDebug() << "Bad message type";
        break;
    }
}
void PXMPeerWorker::printFullHistory(QUuid uuid)
{
    QString str = QString();

    for(auto &itr : peerDetailsHash.value(uuid).messages)
    {
        str.append(*itr % QStringLiteral("\n"));
    }
    str.chop(1);
    emit printToTextBrowser(str, uuid, false);
}

void PXMPeerWorker::setlibeventBackend(QString str)
{
    libeventBackend = str;
}
void PXMPeerWorker::printInfoToDebug()
{
    int i = 0;
    QString str;
    //hopefully reduce reallocs here
    str.reserve((300 + ( DEBUG_PADDING * 16) + (peerDetailsHash.size() * (260+(9*DEBUG_PADDING)))));

    qDebug() << str.capacity();
    QString pad = QString(DEBUG_PADDING, ' ');
    str.append(QStringLiteral("--------------------------------\n"));
    str.append(pad % QStringLiteral("---Program Info---\n"));
    str.append(pad % QStringLiteral("Program Name: ") % qApp->applicationName() % QChar('\n'));
    str.append(pad % QStringLiteral("Version: ") % qApp->applicationVersion() % QChar('\n'));

    str.append(pad % QStringLiteral("---Network Info---\n"));
    str.append(pad % QStringLiteral("Libevent Backend: ") % libeventBackend % QChar('\n')
              % pad % QStringLiteral("Multicast Address: ") % multicastAddress % QChar('\n')
              % pad % QStringLiteral("TCP Listener Port: ") % ourListenerPort % QChar('\n')
              % pad % QStringLiteral("UDP Listener Port: ") % ourUDPListenerPort % QChar('\n')
              % pad % QStringLiteral("Our UUID: ") % localUUID.toString() % QChar('\n')
              % pad % QStringLiteral("MulticastIsFunctioning: ") % QString::fromLocal8Bit((multicastIsFunctioning ? "true" : "false")) % QChar('\n')
              % pad % QStringLiteral("Sync waiting list: ") % QString::number(syncablePeers.length()) % QChar('\n'));

    str.append(pad % QStringLiteral("---Peer Details---\n"));

    for(auto &itr : peerDetailsHash)
    {
        str.append(pad % QStringLiteral("---Peer #") % QString::number(i) % "---\n");
        str.append(itr.toString(pad));
        i++;
    }

    str.append(pad % QStringLiteral("-------------\n"));
    str.append(pad % QStringLiteral("Total Peers: ") % QString::number(i) % QChar('\n'));
    str.append(pad % QStringLiteral("Extra Bufferevents: ") % QString::number(extraBufferevents.count()) % QChar('\n'));
    str.append(pad % QStringLiteral("--------------------------------\n"));
    str.squeeze();
    qDebug().noquote() << str;
    qDebug() << str.capacity();
}
void PXMPeerWorker::discoveryTimerPersistent()
{
    if(peerDetailsHash.count() < 3)
    {
        this->sendUDP("/discover", ourUDPListenerPort.toUShort());
    }
    else
    {
        qDebug() << "Found enough peers";
        discoveryTimer->stop();
    }
}
void PXMPeerWorker::discoveryTimerSingleShot()
{
    if(!multicastIsFunctioning)
    {
        emit warnBox(QStringLiteral("Network Problem"), QStringLiteral("Could not find anyone, even ourselves, on the network.\nThis could indicate a problem with your configuration.\n\nWe'll keep looking..."));
    }
    if(peerDetailsHash.count() < 3)
    {
        discoveryTimer->setInterval(30000);
        QObject::connect(discoveryTimer, &QTimer::timeout, this, &PXMPeerWorker::discoveryTimerPersistent);
        emit sendUDP("/discover", ourUDPListenerPort.toUShort());
        qDebug() << QStringLiteral("Retrying discovery packet, looking for other computers...");
        discoveryTimer->start();
    }
    else
    {
        qDebug() << "Found enough peers";
    }
}
void PXMPeerWorker::midnightTimerPersistent()
{
    QDateTime dt = QDateTime::currentDateTime();
    if(dt.time() <= QTime(0, MIDNIGHT_TIMER_INTERVAL_MINUTES, 0, 0))
    {
        QString str = QString();
        str.append("<br>");
        str.append("<center>");
        str.append(QString(7, QChar('-')));
        str.append(' ');
        str.append(dt.date().toString());
        str.append(' ');
        str.append(QString(7, QChar('-')));
        str.append("</center>");
        str.append("<br>");

        this->addMessageToAllPeers(str, false, false);
    }
}
void PXMPeerWorker::multicastIsFunctional()
{
    multicastIsFunctioning = true;
}
void PXMPeerWorker::serverSetupFailure()
{
    discoveryTimer->stop();
    emit warnBox(QStringLiteral("Server Setup Failure"), QStringLiteral("Failure setting up the server sockets and multicast group.  Check the debug logger for more information.\n\n"
                             "Settings for your network conditions will have to be adjusted and the program restarted."));
}
