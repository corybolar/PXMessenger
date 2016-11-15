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
void PXMPeerWorker::hostnameCheck(QString comp, QUuid senderUuid)
{
    if(senderUuid != waitingOnIpsFrom)
    {
        return;
    }
    waitingOnIpsFrom = QUuid();
    if(syncer->isRunning())
        syncer->moveOnPlease = true;
    QStringList fullIpPacket = comp.split("]", QString::SkipEmptyParts);
    for(auto &str : fullIpPacket)
    {
        QStringList sectionalIpPacket = str.split(':');

        if(sectionalIpPacket.length() != 3)
            continue;

        if(sectionalIpPacket[0].at(0) == '[')
            sectionalIpPacket[0].remove(0,1);

        if(peerDetailsHash.contains(sectionalIpPacket[2]) && peerDetailsHash.value(sectionalIpPacket[2]).isConnected)
            return;
        else
        {
            sockaddr_in addr;
            addr.sin_family = AF_INET;
            addr.sin_port = htons(sectionalIpPacket[1].toULong());

#ifdef _WIN32
            char *ipaddr;
            ipaddr = sectionalIpPacket[0].toLatin1().data();
            WSAStringToAddress(reinterpret_cast<LPWSTR>(ipaddr), AF_INET, NULL, (struct sockaddr*)&addr, reinterpret_cast<LPINT>(sizeof(addr)));
#else
            inet_pton(AF_INET, sectionalIpPacket[0].toLatin1(), &addr.sin_addr);
#endif
            attemptConnection(addr, sectionalIpPacket[2]);
        }
    }
}
void PXMPeerWorker::newTcpConnection(evutil_socket_t s, struct sockaddr_in ss)
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
    emit sendMsg(s, "", QStringLiteral("/request"), localUUID, "");
}
void PXMPeerWorker::attemptConnection(sockaddr_in addr, QUuid uuid)
{
    if(peerDetailsHash.value(uuid).attemptingToConnect || uuid.isNull())
    {
        return;
    }
    evutil_socket_t s = socket(AF_INET, SOCK_STREAM, 0);

    if(peerDetailsHash.contains(uuid))
    {
        peerDetailsHash[uuid].identifier = uuid;
        peerDetailsHash[uuid].socketDescriptor = s;
        peerDetailsHash[uuid].attemptingToConnect = true;
        peerDetailsHash[uuid].ipAddressRaw = addr;
    }
    else
    {
        peerDetails newPeer;
        newPeer.identifier = uuid;
        newPeer.socketDescriptor = s;
        newPeer.ipAddressRaw = addr;
        newPeer.attemptingToConnect = true;
        peerDetailsHash.insert(newPeer.identifier, newPeer);
    }
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
            peerDetailsHash[itr.identifier].attemptingToConnect = false;
            peerDetailsHash[itr.identifier].isConnected = false;
            peerDetailsHash[itr.identifier].socketDescriptor = -1;
            peerDetailsHash[itr.identifier].bev = NULL;
            emit setItalicsOnItem(itr.identifier, 1);
            this->attemptConnection(itr.ipAddressRaw, itr.identifier.toString());
            return;
        }
    }
}
void PXMPeerWorker::sendIps(evutil_socket_t i)
{
    QString msgRaw;
    for(auto & itr : peerDetailsHash)
    {
        if(itr.isConnected)
        {
            msgRaw.append("[");
            msgRaw.append(QString::fromUtf8(inet_ntoa((&(itr.ipAddressRaw))->sin_addr)));
            msgRaw.append(":");
            msgRaw.append(QString::number(ntohs(itr.ipAddressRaw.sin_port)));
            msgRaw.append(":");
            msgRaw.append(itr.identifier.toString());
            msgRaw.append("]");
        }
    }
    emit sendMsg(i, msgRaw, "/ip", localUUID, "");
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
        peerDetailsHash[uuid].attemptingToConnect = false;
        struct bufferevent *bev;
        evutil_make_socket_nonblocking(socket);
        bev = bufferevent_socket_new(realServer->base, socket, BEV_OPT_CLOSE_ON_FREE);
        bufferevent_setcb(bev, realServer->tcpReadUUID, NULL, realServer->tcpError, (void*)realServer);
        bufferevent_setwatermark(bev, EV_READ, 0, TCP_BUFFER_WATERMARK);
        bufferevent_enable(bev, EV_READ);
        if(peerDetailsHash[uuid].bev != nullptr)
            bufferevent_free(peerDetailsHash[uuid].bev);
        peerDetailsHash[uuid].bev = bev;

        emit sendIdentityMsg(socket);
    }
    else
    {
        peerDetailsHash[uuid].attemptingToConnect = false;
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
        if(levelOfSuccess > 0)
        {
            msg.append(QStringLiteral("\nThe previous message was only paritally sent.  This was very bad\nContact the administrator of this program immediately\nNumber of bytes sent: ") % QString::number(levelOfSuccess));
        }
        if(levelOfSuccess == 0)
        {

        }
        emit printToTextBrowser(msg, uuid, print);

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
    else if(peerDetailsHash.contains(uuid) && peerDetailsHash.value(uuid).isConnected)
    {
        return;
    }
    bufferevent *bev = (bufferevent*)bevptr;
    struct sockaddr_in addr;
    socklen_t socklen = sizeof(addr);
    char *ipaddr;
    memset(&addr, 0, socklen);

    //Get ip address of sender
    getpeername(s, (struct sockaddr*)&addr, &socklen);
    ipaddr = inet_ntoa((&addr)->sin_addr);
    //bufferevent_setcb(static_cast<bufferevent*>(bevptr), realServer->tcpRead, NULL, realServer->tcpError, (void*)realServer);
    if( !( peerDetailsHash.value(uuid).isConnected ) )
    {
        qDebug() << "hostname:" << hname << "@ ip:" << ipaddr << "on port" << port << "authenticated!";

        peerDetailsHash[uuid].socketDescriptor = s;
        peerDetailsHash[uuid].isConnected = true;
        peerDetailsHash[uuid].attemptingToConnect = true;
        peerDetailsHash[uuid].identifier = uuid;
        peerDetailsHash[uuid].hostname = hname;
        peerDetailsHash[uuid].bev = bev;
        peerDetailsHash[uuid].ipAddressRaw = addr;

        emit setItalicsOnItem(uuid, 0);
    }
    else
    {
       qDebug() << "hostname:" << hname << "@ ip:" << ipaddr << "on port" << port << "authenticated!";

        peerDetails newPeer;

        newPeer.socketDescriptor = s;
        newPeer.isConnected = true;
        newPeer.attemptingToConnect = true;
        newPeer.identifier = uuid;
        newPeer.hostname = hname;
        newPeer.bev = bev;
        newPeer.ipAddressRaw = addr;

        peerDetailsHash.insert(uuid, newPeer);
    }
    emit requestIps(s, uuid);
    emit updateListWidget(uuid);
}
