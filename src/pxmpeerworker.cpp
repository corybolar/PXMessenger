#include <pxmpeerworker.h>
using namespace PXMConsts;

PXMPeerWorker::PXMPeerWorker(QObject *parent, initialSettings presets, QUuid globaluuid) : QObject(parent)
{
    //Init
    areWeSyncing = false;
    syncablePeers = new TimedVector<QUuid>(SYNC_TIMEOUT_MSECS, SECONDS);
    messClient = nullptr;
    //End of Init

    localHostname = presets.username;
    localUUID = presets.uuid;
    globalUUID = globaluuid;
    multicastAddress = presets.multicast;
    serverTCPPort = presets.tcpPort;
    serverUDPPort = presets.udpPort;

    //Prevent race condition when starting threads, a bufferevent
    //for this (us) is coming soon.
    Peers::PeerData selfComms;
    selfComms.identifier = localUUID;
    selfComms.hostname = localHostname;
    selfComms.connectTo = true;
    selfComms.isAuthenticated = false;
    peerDetailsHash.insert(localUUID, selfComms);

    Peers::PeerData globalPeer;
    globalPeer.identifier = globalUUID;
    globalPeer.hostname = "Global Chat";
    globalPeer.connectTo = true;
    globalPeer.isAuthenticated = false;
    peerDetailsHash.insert(globalUUID, globalPeer);
}
PXMPeerWorker::~PXMPeerWorker()
{
    syncTimer->stop();
    nextSyncTimer->stop();
    delete syncablePeers;

    for(auto &itr : peerDetailsHash)
    {
        qDeleteAll(itr.messages);
        evutil_closesocket(itr.socket);
    }
    //This must be done before PXMServer is shutdown;
    peerDetailsHash.clear();

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

    in_addr multicast_in_addr;
    multicast_in_addr.s_addr = inet_addr(multicastAddress.toLatin1().constData());
    messClient = new PXMClient(this, multicast_in_addr, localUUID);
    messServer = new PXMServer::ServerThread(this, serverTCPPort, serverUDPPort, multicast_in_addr);

    connectClient();
    startServer();
}
void PXMPeerWorker::startServer()
{
    messServer->setLocalUUID(localUUID);
    QObject::connect(messServer, &PXMServer::ServerThread::authenticationReceived, this, &PXMPeerWorker::authenticationReceived, Qt::QueuedConnection);
    QObject::connect(messServer, &PXMServer::ServerThread::messageRecieved, this, &PXMPeerWorker::recieveServerMessage, Qt::QueuedConnection);
    QObject::connect(messServer, &QThread::finished, messServer, &QObject::deleteLater);
    QObject::connect(messServer, &PXMServer::ServerThread::newTCPConnection, this, &PXMPeerWorker::newTcpConnection, Qt::QueuedConnection);
    QObject::connect(messServer, &PXMServer::ServerThread::peerQuit, this, &PXMPeerWorker::peerQuit, Qt::QueuedConnection);
    QObject::connect(messServer, &PXMServer::ServerThread::attemptConnection, this, &PXMPeerWorker::attemptConnection, Qt::QueuedConnection);
    QObject::connect(messServer, &PXMServer::ServerThread::sendSyncPacket, this, &PXMPeerWorker::sendSyncPacketBev, Qt::QueuedConnection);
    QObject::connect(messServer, &PXMServer::ServerThread::syncPacketIterator, this, &PXMPeerWorker::syncPacketIterator, Qt::QueuedConnection);
    QObject::connect(messServer, &PXMServer::ServerThread::setPeerHostname, this, &PXMPeerWorker::peerNameChange, Qt::QueuedConnection);
    QObject::connect(messServer, &PXMServer::ServerThread::setListenerPorts, this, &PXMPeerWorker::setListenerPorts, Qt::QueuedConnection);
    QObject::connect(messServer, &PXMServer::ServerThread::libeventBackend, this, &PXMPeerWorker::setlibeventBackend, Qt::QueuedConnection);
    QObject::connect(messServer, &PXMServer::ServerThread::setCloseBufferevent, this, &PXMPeerWorker::setCloseBufferevent, Qt::QueuedConnection);
    QObject::connect(messServer, &PXMServer::ServerThread::setSelfCommsBufferevent,
                     this, &PXMPeerWorker::setSelfCommsBufferevent, Qt::QueuedConnection);
    QObject::connect(messServer, &PXMServer::ServerThread::multicastIsFunctional, this, &PXMPeerWorker::multicastIsFunctional, Qt::QueuedConnection);
    QObject::connect(messServer, &PXMServer::ServerThread::serverSetupFailure, this, &PXMPeerWorker::serverSetupFailure, Qt::QueuedConnection);
    QObject::connect(messServer, &PXMServer::ServerThread::sendUDP, this, &PXMPeerWorker::sendUDP, Qt::QueuedConnection);
    QObject::connect(messServer, &PXMServer::ServerThread::nameChange, this, &PXMPeerWorker::peerNameChange, Qt::QueuedConnection);
    messServer->start();
}
void PXMPeerWorker::connectClient()
{
    QObject::connect(messClient, &PXMClient::resultOfConnectionAttempt,
                     this, &PXMPeerWorker::resultOfConnectionAttempt, Qt::QueuedConnection);
    QObject::connect(messClient, &PXMClient::resultOfTCPSend, this, &PXMPeerWorker::resultOfTCPSend, Qt::QueuedConnection);
    QObject::connect(this, &PXMPeerWorker::sendMsg, messClient, &PXMClient::sendMsgSlot, Qt::QueuedConnection);
    QObject::connect(this, &PXMPeerWorker::connectToPeer, messClient, &PXMClient::connectToPeer, Qt::QueuedConnection);
    QObject::connect(this, &PXMPeerWorker::sendIpsPacket, messClient, &PXMClient::sendIpsSlot, Qt::QueuedConnection);
    QObject::connect(this, &PXMPeerWorker::sendUDP, messClient, &PXMClient::sendUDP, Qt::QueuedConnection);
}

void PXMPeerWorker::setListenerPorts(unsigned short tcpport, unsigned short udpport)
{
    serverTCPPort = tcpport;
    serverUDPPort = udpport;
}
void PXMPeerWorker::doneSync()
{
    areWeSyncing = false;
    nextSyncTimer->stop();
    qInfo() << "Finished Syncing peers";
}
void PXMPeerWorker::beginSync()
{
    if(areWeSyncing)
        return;

    qInfo() << "Beginning Sync of connected peers";
    areWeSyncing = true;
    syncer->setsyncHash(&peerDetailsHash);
    syncer->setIteratorToStart();
    syncer->syncNext();
    nextSyncTimer->start();
}
void PXMPeerWorker::syncPacketIterator(char *ipHeapArray, size_t len, QUuid senderUuid)
{
    if(!syncablePeers->contains(senderUuid))
    {
        qWarning() << "Sync packet from bad uuid -- timeout or not requested" << senderUuid;
        delete [] ipHeapArray;
        return;
    }
    qInfo() << "Sync packet from" << senderUuid.toString();

    size_t index = 0;
    while(index + UUIDCompression::PACKED_UUID_LENGTH + 6 <= len)
    {
        sockaddr_in addr;
        addr.sin_family = AF_INET;
        memcpy(&(addr.sin_addr.s_addr), &ipHeapArray[index], sizeof(uint32_t));
        index += sizeof(uint32_t);
        memcpy(&(addr.sin_port), &ipHeapArray[index], sizeof(uint16_t));
        index += sizeof(uint16_t);
        QUuid uuid = UUIDCompression::unpackUUID((unsigned char*)&ipHeapArray[index]);
        index += UUIDCompression::PACKED_UUID_LENGTH;

        qInfo() << inet_ntoa(addr.sin_addr) << ":" << ntohs(addr.sin_port) << ":" << uuid.toString();
        attemptConnection(addr, uuid);
    }
    qInfo() << "End of sync packet";

    syncablePeers->remove(senderUuid);

    if(areWeSyncing)
    {
        nextSyncTimer->start(2000);
        syncer->syncNext();
    }

    delete [] ipHeapArray;
}
void PXMPeerWorker::newTcpConnection(bufferevent *bev)
{
    QSharedPointer<Peers::BevWrapper> bw(new Peers::BevWrapper);
    bw->setBev(bev);
    extraBufferevents.push_back(bw);
    this->sendAuthPacket(bw);
}
void PXMPeerWorker::sendAuthPacket(QSharedPointer<Peers::BevWrapper> bw)
{
    emit sendMsg(bw, (localHostname % QString::fromLatin1(PORT_SEPERATOR) % QString::number(serverTCPPort)).toUtf8(), MSG_AUTH);
}
void PXMPeerWorker::requestSyncPacket(QSharedPointer<Peers::BevWrapper> bw, QUuid uuid)
{
    syncablePeers->append(uuid);
    qInfo() << "Requesting ips from" << peerDetailsHash.value(uuid).hostname;
    emit sendMsg(bw, QByteArray(), MSG_SYNC_REQUEST);
}
void PXMPeerWorker::attemptConnection(sockaddr_in addr, QUuid uuid)
{
    if(peerDetailsHash.value(uuid).connectTo || uuid.isNull())
    {
        return;
    }

    evutil_socket_t socketfd = socket(AF_INET, SOCK_STREAM, 0);
    evutil_make_socket_nonblocking(socketfd);
    struct bufferevent *bev;
    if(PXMServer::ServerThread::base)
    {
        bev = bufferevent_socket_new(PXMServer::ServerThread::base, socketfd, BEV_OPT_THREADSAFE);
        peerDetailsHash[uuid].bw->lockBev();
        peerDetailsHash[uuid].bw->setBev(bev);
        peerDetailsHash[uuid].bw->unlockBev();
        peerDetailsHash[uuid].identifier = uuid;
        peerDetailsHash[uuid].socket = socketfd;
        peerDetailsHash[uuid].connectTo = true;
        peerDetailsHash[uuid].ipAddressRaw = addr;

        emit connectToPeer(socketfd, addr, peerDetailsHash[uuid].bw);
    }
    else
    {
        qCritical() << "Server event_base is not initialized, cannot attempt connections";
        evutil_closesocket(socketfd);
    }
}
void PXMPeerWorker::peerNameChange(QString hname, QUuid uuid)
{
    if(peerDetailsHash.contains(uuid))
    {
        peerDetailsHash[uuid].hostname = hname.left(PXMConsts::MAX_HOSTNAME_LENGTH + PXMConsts::MAX_COMPUTER_NAME);
        emit updateListWidget(uuid, peerDetailsHash.value(uuid).hostname);
    }
}
void PXMPeerWorker::peerQuit(evutil_socket_t s, bufferevent *bev)
{
    for(auto &itr : peerDetailsHash)
    {
        if(itr.bw->getBev() == bev)
        {
            peerDetailsHash[itr.identifier].connectTo = false;
            peerDetailsHash[itr.identifier].isAuthenticated = false;
            peerDetailsHash[itr.identifier].bw->lockBev();
            qInfo().noquote() << "Peer:" << itr.identifier.toString() << "has disconnected";
            peerDetailsHash[itr.identifier].bw->setBev(nullptr);
            bufferevent_free(bev);
            peerDetailsHash[itr.identifier].bw->unlockBev();
            evutil_closesocket(itr.socket);
            peerDetailsHash[itr.identifier].socket = -1;
            emit setItalicsOnItem(itr.identifier, 1);
            return;
        }
    }
    for(QSharedPointer<Peers::BevWrapper> &itr : extraBufferevents)
    {
        extraBufferevents.removeAll(itr);
    }
    qInfo().noquote() << "Non-Authed Peer has quit";
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
        qCritical() << "sendIpsBev:error";
    }
}
void PXMPeerWorker::sendSyncPacket(QSharedPointer<Peers::BevWrapper> bw, QUuid uuid)
{
    qInfo() << "Sending ips to" << peerDetailsHash.value(uuid).hostname;
    char *msgRaw = new char[peerDetailsHash.size() * (sizeof(uint32_t) + sizeof(uint16_t) + UUIDCompression::PACKED_UUID_LENGTH) + 1];
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

    if(messClient)
        emit sendIpsPacket(bw, msgRaw, index, MSG_SYNC);
    else
    {
        qCritical() << "messClient not initialized";
        delete[] msgRaw;
    }
}
void PXMPeerWorker::resultOfConnectionAttempt(evutil_socket_t socket, bool result, bufferevent *bev)
{
    QUuid uuid = QUuid();
    for(auto &itr : peerDetailsHash)
    {
        if(itr.bw->getBev() == bev)
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
        qInfo() << "Successful connection attempt to" << uuid.toString();
        bufferevent_setcb(bev, PXMServer::ServerThread::tcpReadUUID, NULL, PXMServer::ServerThread::tcpError, messServer);
        bufferevent_setwatermark(bev, EV_READ, PXMServer::PACKET_HEADER_LENGTH, PXMServer::PACKET_HEADER_LENGTH);
        bufferevent_enable(bev, EV_READ|EV_WRITE);
        peerDetailsHash[uuid].bw->lockBev();
        if(peerDetailsHash.value(uuid).bw->getBev() != nullptr && peerDetailsHash.value(uuid).bw->getBev() != bev)
        {
            bufferevent_free(peerDetailsHash.value(uuid).bw->getBev());
            peerDetailsHash.value(uuid).bw->setBev(bev);
        }
        peerDetailsHash.value(uuid).bw->unlockBev();

        sendAuthPacket(peerDetailsHash.value(uuid).bw);
    }
    else
    {
        qInfo() << "Unsuccessful connection attempt to " << uuid.toString();
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
void PXMPeerWorker::resultOfTCPSend(int levelOfSuccess, QUuid uuid, QString msg, bool print, QSharedPointer<Peers::BevWrapper> bw)
{
    if(print)
    {
        if(levelOfSuccess == 0)
        {
            msg.prepend(localHostname % ": ");
        }
        else if(levelOfSuccess < 0)
        {
            qInfo() << "Send Failure";
        }
        addMessageToPeer(msg, uuid, false, true);
    }
    /*
    if(bwShortLife.contains(bw))
    {
        delete bw;
    }
    */
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

    qInfo().noquote() << hname << "on port" << QString::number(port) << "authenticated!";

    if(peerDetailsHash[uuid].bw->getBev() != nullptr && peerDetailsHash[uuid].bw->getBev() != bev)
    {
        peerQuit(-1, peerDetailsHash.value(uuid).bw->getBev());
    }
    else
    {
        for(QSharedPointer<Peers::BevWrapper> &itr : extraBufferevents)
        {
            if(itr->getBev() == bev)
            {
                peerDetailsHash[uuid].bw = itr;
                extraBufferevents.removeAll(itr);
                break;
            }
        }
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
            qWarning() << "Message from invalid uuid, rejection";
            return -1;
        }

        if(peerDetailsHash.value(uuid).bw->getBev() != bev)
        {
            bool foundIt = false;
            QUuid uuidSpoofer = QUuid();
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
void PXMPeerWorker::sendMsgAccessor(QByteArray msg, MESSAGE_TYPE type, QUuid uuid)
{
    switch (type) {
    case MSG_TEXT:
        if(!uuid.isNull())
            emit sendMsg(peerDetailsHash.value(uuid).bw, msg, MSG_TEXT, uuid);
        else
            qWarning() << "Bad recipient uuid for normal message";
        break;
    case MSG_GLOBAL:
        for(auto &itr : peerDetailsHash)
        {
            if(itr.isAuthenticated || itr.identifier == localUUID)
                emit sendMsg(itr.bw, msg, MSG_GLOBAL);
        }
        break;
    case MSG_NAME:
        peerDetailsHash[localUUID].hostname = QString(msg);
        localHostname = QString(msg);
        for(auto &itr : peerDetailsHash)
        {
            if(itr.isAuthenticated || itr.identifier == localUUID)
                emit sendMsg(itr.bw, msg, MSG_NAME);
        }
        break;
    default:
        qWarning() << "Bad message type in sendMsgAccessor";
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
    QString str;

    //hopefully reduce reallocs here
    str.reserve((330 + ( DEBUG_PADDING * 16) + (peerDetailsHash.size() * (260+(9*DEBUG_PADDING)))));

    str.append(QChar('\n') % QChar('\n') % QStringLiteral("---Program Info---\n")
               % QStringLiteral("Program Name: ") % qApp->applicationName() % QChar('\n')
               % QStringLiteral("Version: ") % qApp->applicationVersion() % QChar('\n')
               % QStringLiteral("---Network Info---\n")
               % QStringLiteral("Libevent Backend: ") % libeventBackend % QChar('\n')
               % QStringLiteral("Multicast Address: ") % multicastAddress % QChar('\n')
               % QStringLiteral("TCP Listener Port: ") % QString::number(serverTCPPort) % QChar('\n')
               % QStringLiteral("UDP Listener Port: ") % QString::number(serverUDPPort) % QChar('\n')
               % QStringLiteral("Our UUID: ") % localUUID.toString() % QChar('\n')
               % QStringLiteral("MulticastIsFunctioning: ")
               % QString::fromLocal8Bit((multicastIsFunctioning ? "true" : "false")) % QChar('\n')
               % QStringLiteral("Sync waiting list: ") % QString::number(syncablePeers->length()) % QChar('\n')
               % QStringLiteral("---Peer Details---\n"));

    int peerCount = 0;
    for(auto &itr : peerDetailsHash)
    {
        str.append(QStringLiteral("---Peer #") % QString::number(peerCount) % QStringLiteral("---\n") % itr.toString());
        peerCount++;
    }

    str.append(QStringLiteral("-------------\n")
               % QStringLiteral("Total Peers: ") % QString::number(peerCount) % QChar('\n'));
    str.squeeze();
    qInfo().noquote() << str;
}
void PXMPeerWorker::discoveryTimerPersistent()
{
    if(peerDetailsHash.count() < 3)
    {
        this->sendUDP("/discover", serverUDPPort);
    }
    else
    {
        qInfo() << "Found enough peers";
        discoveryTimer->stop();
    }
}
void PXMPeerWorker::discoveryTimerSingleShot()
{
    if(!multicastIsFunctioning)
    {
        emit warnBox(QStringLiteral("Network Problem"),
                     QStringLiteral("Could not find anyone, even ourselves, on "
                                    "the network.\nThis could indicate a "
                                    "problem with your configuration."
                                    "\n\nWe'll keep looking..."));
    }
    if(peerDetailsHash.count() < 3)
    {
        discoveryTimer->setInterval(30000);
        QObject::connect(discoveryTimer, &QTimer::timeout, this, &PXMPeerWorker::discoveryTimerPersistent);
        emit sendUDP("/discover", serverUDPPort);
        qInfo() << QStringLiteral("Retrying discovery packet, looking "
                                   "for other computers...");
        discoveryTimer->start();
    }
    else
    {
        qInfo() << "Found enough peers";
    }
}

void PXMPeerWorker::midnightTimerPersistent()
{
    QDateTime dt = QDateTime::currentDateTime();
    if(dt.time() <= QTime(0, MIDNIGHT_TIMER_INTERVAL_MINUTES, 0, 0))
    {
        QString str = QString();
        str.append("<br>"
                   "<center>" %
                   QString(7, QChar('-')) %
                   QChar(' ') %
                   dt.date().toString() %
                   QChar(' ') %
                   QString(7, QChar('-')) %
                   "</center>"
                   "<br>");

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
    emit warnBox(QStringLiteral("Server Setup Failure"),
                 QStringLiteral("Failure setting up the server sockets and "
                                "multicast group.  Check the debug logger for "
                                "more information.\n\n"
                                "Settings for your network conditions will "
                                "have to be adjusted and the program "
                                "restarted."));
}

void PXMPeerWorker::setLocalHostname(QString hname)
{
    localHostname = hname;
}
