#include <pxmpeerworker.h>

#include <QApplication>
#include <QDebug>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QStringBuilder>
#include <QThread>
#include <QTimer>
#include <QSharedPointer>

#include "pxmclient.h"
#include "pxmserver.h"
#include "pxmsync.h"
#include "pxmpeerworker_p.h"
#include "timedvector.h"

#include <event2/event.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#elif __unix__
#include <arpa/inet.h>
#include <arpa/nameser.h>
#include <netdb.h>
#include <netinet/in.h>
#include <resolv.h>
#include <sys/socket.h>
#else
#error "include headers for socket implementation"
#endif

static_assert(sizeof(uint16_t) == 2, "uint16_t not defined as 2 bytes");
static_assert(sizeof(uint32_t) == 4, "uint32_t not defined as 4 bytes");

PXMPeerWorker::PXMPeerWorker(QObject* parent,
                             QString username,
                             QUuid selfUUID,
                             QString multicast,
                             unsigned short tcpPort,
                             unsigned short udpPort,
                             QUuid globaluuid)
    : QObject(parent),
      d_ptr(new PXMPeerWorkerPrivate(this, username, selfUUID, multicast, tcpPort, udpPort, globaluuid))
{
    d_ptr->areWeSyncing = false;
    d_ptr->messClient   = nullptr;
    // End of Init

    // Prevent race condition when starting threads, a bufferevent
    // for this (us) is coming soon.
    Peers::PeerData selfComms;
    selfComms.uuid        = d_ptr->localUUID;
    selfComms.hostname    = d_ptr->localHostname;
    selfComms.connectTo   = true;
    selfComms.isAuthed    = false;
    selfComms.progVersion = qApp->applicationVersion();
    d_ptr->peersHash.insert(d_ptr->localUUID, selfComms);

    Peers::PeerData globalPeer;
    globalPeer.uuid      = d_ptr->globalUUID;
    globalPeer.hostname  = "Global Chat";
    globalPeer.connectTo = true;
    globalPeer.isAuthed  = false;
    d_ptr->peersHash.insert(d_ptr->globalUUID, globalPeer);
}
PXMPeerWorker::~PXMPeerWorker()
{
    qDebug() << "Shutdown of PXMPeerWorker Successful";
}
void PXMPeerWorkerPrivate::setInternalBufferevent(bufferevent* bev)
{
    internalBev = bev;
}
void PXMPeerWorker::init()
{
    d_ptr->syncer = new PXMSync(this, d_ptr->peersHash);
    QObject::connect(d_ptr->syncer, &PXMSync::requestIps, d_ptr.data(), &PXMPeerWorkerPrivate::requestSyncPacket);
    QObject::connect(d_ptr->syncer, &PXMSync::syncComplete, d_ptr.data(), &PXMPeerWorkerPrivate::donePeerSync);

    srand(static_cast<unsigned int>(time(NULL)));

    d_ptr->syncTimer = new QTimer(this);
    d_ptr->syncTimer->setInterval((rand() % 300000) + SYNC_TIMER_MSECS);
    QObject::connect(d_ptr->syncTimer, &QTimer::timeout, this, &PXMPeerWorker::beginPeerSync);
    d_ptr->syncTimer->start();

    d_ptr->nextSyncTimer = new QTimer(this);
    d_ptr->nextSyncTimer->setInterval(2000);
    QObject::connect(d_ptr->nextSyncTimer, &QTimer::timeout, d_ptr->syncer, &PXMSync::syncNext);

    d_ptr->discoveryTimerSingle = new QTimer(this);
    d_ptr->discoveryTimerSingle->setSingleShot(true);
    d_ptr->discoveryTimerSingle->setInterval(5000);
    QObject::connect(d_ptr->discoveryTimerSingle, &QTimer::timeout, d_ptr.data(),
                     &PXMPeerWorkerPrivate::discoveryTimerSingleShot);
    d_ptr->discoveryTimerSingle->start();

    d_ptr->midnightTimer = new QTimer(this);
    d_ptr->midnightTimer->setInterval(d_ptr->MIDNIGHT_TIMER_INTERVAL_MINUTES * 60000);
    QObject::connect(d_ptr->midnightTimer, &QTimer::timeout, d_ptr.data(),
                     &PXMPeerWorkerPrivate::midnightTimerPersistent);
    d_ptr->midnightTimer->start();

    d_ptr->discoveryTimer = new QTimer(this);

    in_addr multicast_in_addr;
    multicast_in_addr.s_addr = inet_addr(d_ptr->multicastAddress.toLatin1().constData());
    d_ptr->messClient        = new PXMClient(this, multicast_in_addr, d_ptr->localUUID);
    d_ptr->messServer = new PXMServer::ServerThread(this, d_ptr->localUUID, multicast_in_addr, d_ptr->serverTCPPort,
                                                    d_ptr->serverUDPPort);

    d_ptr->connectClient();
    d_ptr->startServer();
}
void PXMPeerWorkerPrivate::startServer()
{
    using namespace PXMServer;
    QObject::connect(messServer, &ServerThread::authHandler, this, &PXMPeerWorkerPrivate::authHandler,
                     Qt::QueuedConnection);
    QObject::connect(messServer, &ServerThread::msgHandler, this, &PXMPeerWorkerPrivate::msgHandler,
                     Qt::QueuedConnection);
    QObject::connect(messServer, &QThread::finished, messServer, &QObject::deleteLater);
    QObject::connect(messServer, &ServerThread::newTCPConnection, this, &PXMPeerWorkerPrivate::newAcceptedConnection,
                     Qt::QueuedConnection);
    QObject::connect(messServer, &ServerThread::peerQuit, this, &PXMPeerWorkerPrivate::peerQuit, Qt::QueuedConnection);
    QObject::connect(messServer, &ServerThread::attemptConnection, this, &PXMPeerWorkerPrivate::attemptConnection,
                     Qt::QueuedConnection);
    QObject::connect(messServer, &ServerThread::syncRequestHandler, this, &PXMPeerWorkerPrivate::syncRequestHandlerBev,
                     Qt::QueuedConnection);
    QObject::connect(messServer, &ServerThread::syncHandler, this, &PXMPeerWorkerPrivate::syncHandler,
                     Qt::QueuedConnection);
    QObject::connect(messServer, &ServerThread::setPeerHostname, this, &PXMPeerWorkerPrivate::nameHandler,
                     Qt::QueuedConnection);
    QObject::connect(messServer, &ServerThread::setListenerPorts, this, &PXMPeerWorkerPrivate::setListenerPorts,
                     Qt::QueuedConnection);
    QObject::connect(messServer, &ServerThread::libeventBackend, this, &PXMPeerWorkerPrivate::setLibeventBackend,
                     Qt::QueuedConnection);
    QObject::connect(messServer, &ServerThread::setInternalBufferevent, this,
                     &PXMPeerWorkerPrivate::setInternalBufferevent, Qt::QueuedConnection);
    QObject::connect(messServer, &ServerThread::setSelfCommsBufferevent, this,
                     &PXMPeerWorkerPrivate::setSelfCommsBufferevent, Qt::QueuedConnection);
    QObject::connect(messServer, &ServerThread::serverSetupFailure, this, &PXMPeerWorkerPrivate::serverSetupFailure,
                     Qt::QueuedConnection);
    QObject::connect(messServer, &ServerThread::sendUDP, q_ptr, &PXMPeerWorker::sendUDP, Qt::QueuedConnection);
    QObject::connect(messServer, &ServerThread::nameHandler, this, &PXMPeerWorkerPrivate::nameHandler,
                     Qt::QueuedConnection);
    QObject::connect(messServer, &ServerThread::resultOfConnectionAttempt, this,
                     &PXMPeerWorkerPrivate::resultOfConnectionAttempt, Qt::QueuedConnection);
    QObject::connect(messServer, &PXMServer::ServerThread::multicastIsFunctional, this,
                     &PXMPeerWorkerPrivate::multicastIsFunctional, Qt::QueuedConnection);
    messServer->start();
}
void PXMPeerWorkerPrivate::connectClient()
{
    QObject::connect(messClient, &PXMClient::resultOfTCPSend, this, &PXMPeerWorkerPrivate::resultOfTCPSend,
                     Qt::QueuedConnection);
    QObject::connect(q_ptr, &PXMPeerWorker::sendMsg, messClient, &PXMClient::sendMsgSlot, Qt::QueuedConnection);
    QObject::connect(q_ptr, &PXMPeerWorker::sendUDP, messClient, &PXMClient::sendUDP, Qt::QueuedConnection);
}

void PXMPeerWorkerPrivate::setListenerPorts(unsigned short tcpport, unsigned short udpport)
{
    serverTCPPort = tcpport;
    serverUDPPort = udpport;
}
void PXMPeerWorkerPrivate::donePeerSync()
{
    areWeSyncing = false;
    nextSyncTimer->stop();
    qInfo() << "Finished Syncing peers";
}
void PXMPeerWorker::beginPeerSync()
{
    if (d_ptr->areWeSyncing) {
        return;
    }

    qInfo() << "Beginning Sync of connected peers";
    d_ptr->areWeSyncing = true;
    d_ptr->syncer->setsyncHash(d_ptr->peersHash);
    d_ptr->syncer->setIteratorToStart();
    d_ptr->syncer->syncNext();
    d_ptr->nextSyncTimer->start();
}
void PXMPeerWorkerPrivate::syncHandler(QSharedPointer<unsigned char> syncPacket, size_t len, QUuid senderUuid)
{
    if (!syncablePeers->contains(senderUuid)) {
        qWarning() << "Sync packet from bad uuid -- timeout or not "
                      "requested"
                   << senderUuid.toString();
        return;
    }
    qDebug() << "Sync packet from" << senderUuid.toString();

    size_t index = 0;
    while (index + NetCompression::PACKED_UUID_LENGTH + 6 <= len) {
        struct sockaddr_in addr;
        index += NetCompression::unpackSockaddr_in(&syncPacket.data()[index], addr);
        addr.sin_family = AF_INET;
        QUuid uuid      = QUuid();
        index += NetCompression::unpackUUID(&syncPacket.data()[index], uuid);

        qDebug().noquote() << inet_ntoa(addr.sin_addr) << ":" << ntohs(addr.sin_port) << ":" << uuid.toString();
        attemptConnection(addr, uuid);
    }
    qDebug().noquote() << "End of sync packet";

    syncablePeers->remove(senderUuid);

    if (areWeSyncing) {
        nextSyncTimer->stop();
        nextSyncTimer->start(2000);
        syncer->syncNext();
    }
}
void PXMPeerWorkerPrivate::newAcceptedConnection(bufferevent* bev)
{
    QSharedPointer<Peers::BevWrapper> bw(new Peers::BevWrapper);
    bw->setBev(bev);
    extraBevs.push_back(bw);
    sendAuthPacket(bw);
}
void PXMPeerWorkerPrivate::sendAuthPacket(QSharedPointer<Peers::BevWrapper> bw)
{
    using namespace PXMConsts;
    // Auth packet format "Hostname:::12345:::001.001.001
    QByteArray authPacket((localHostname % QString::fromLatin1(AUTH_SEPERATOR) % QString::number(serverTCPPort) %
                           QString::fromLatin1(AUTH_SEPERATOR) % qApp->applicationVersion())
                              .toUtf8());
    emit q_ptr->sendMsg(bw, authPacket, authPacket.size(), MSG_AUTH);
}
void PXMPeerWorkerPrivate::requestSyncPacket(QSharedPointer<Peers::BevWrapper> bw, QUuid uuid)
{
    syncablePeers->append(uuid);
    qDebug() << "Requesting sync from" << peersHash.value(uuid).hostname;
    emit q_ptr->sendMsg(bw, QByteArray(), 0, PXMConsts::MSG_SYNC_REQUEST);
}
void PXMPeerWorkerPrivate::attemptConnection(struct sockaddr_in addr, QUuid uuid)
{
    if (peersHash.value(uuid).connectTo || uuid.isNull()) {
        return;
    }

    peersHash[uuid].uuid      = uuid;
    peersHash[uuid].connectTo = true;
    peersHash[uuid].addrRaw   = addr;

    // Tell Server to connect
    size_t index                                         = 0;
    PXMServer::INTERNAL_MSG addBev                       = PXMServer::INTERNAL_MSG::CONNECT_TO_ADDR;
    unsigned char bevPtr[PXMServer::INTERNAL_MSG_LENGTH] = {};
    memcpy(&bevPtr[index], &addBev, sizeof(addBev));
    index += sizeof(addBev);
    memcpy(&bevPtr[index], &(addr.sin_addr.s_addr), sizeof(addr.sin_addr.s_addr));
    index += sizeof(addr.sin_addr.s_addr);
    memcpy(&bevPtr[index], &(addr.sin_port), sizeof(addr.sin_port));
    index += sizeof(addr.sin_port);
    index += NetCompression::packUUID(&bevPtr[index], uuid);
    bufferevent_write(internalBev, &bevPtr[0], PXMServer::INTERNAL_MSG_LENGTH);
}
void PXMPeerWorkerPrivate::nameHandler(QString hname, const QUuid uuid)
{
    if (peersHash.contains(uuid)) {
        peersHash[uuid].hostname = hname.left(PXMConsts::MAX_HOSTNAME_LENGTH + PXMConsts::MAX_COMPUTER_NAME);
        emit q_ptr->newAuthedPeer(uuid, peersHash.value(uuid).hostname);
    }
}
void PXMPeerWorkerPrivate::peerQuit(evutil_socket_t s, bufferevent* bev)
{
    for (Peers::PeerData& itr : peersHash) {
        if (itr.bw->getBev() == bev) {
            peersHash[itr.uuid].connectTo = false;
            peersHash[itr.uuid].isAuthed  = false;
            peersHash[itr.uuid].bw->lockBev();
            qInfo().noquote() << "Peer:" << itr.uuid.toString() << "has disconnected";
            peersHash[itr.uuid].bw->setBev(nullptr);
            bufferevent_free(bev);
            peersHash[itr.uuid].bw->unlockBev();
            evutil_closesocket(itr.socket);
            peersHash[itr.uuid].socket = -1;
            emit q_ptr->connectionStatusChange(itr.uuid, 1);
            return;
        }
    }
    for (QSharedPointer<Peers::BevWrapper>& itr : extraBevs) {
        extraBevs.removeAll(itr);
    }
    qWarning().noquote() << "Non-Authed Peer has quit";
    evutil_closesocket(s);
}
void PXMPeerWorkerPrivate::syncRequestHandlerBev(const bufferevent* bev, const QUuid uuid)
{
    if (peersHash.contains(uuid) && peersHash.value(uuid).bw->getBev() == bev) {
        size_t index = 0;
        QByteArray packet(createSyncPacket(index));
        if (messClient && index != 0) {
            qDebug() << "Sending sync to" << peersHash.value(uuid).hostname;
            emit q_ptr->sendMsg(peersHash.value(uuid).bw, packet, index, PXMConsts::MSG_SYNC);
        } else {
            qCritical() << "messClient not initialized";
        }
    } else {
        qCritical() << "sendIpsBev:error";
    }
}
QByteArray PXMPeerWorkerPrivate::createSyncPacket(size_t& index)
{
    unsigned char msgRaw[static_cast<size_t>(peersHash.size()) *
                             (NetCompression::PACKED_SOCKADDR_IN_LENGTH + NetCompression::PACKED_UUID_LENGTH) +
                         1];

    index = 0;
    for (Peers::PeerData& itr : peersHash) {
        if (itr.isAuthed) {
            index += NetCompression::packSockaddr_in(&msgRaw[index], itr.addrRaw);
            index += NetCompression::packUUID(&msgRaw[index], itr.uuid);
        }
    }
    msgRaw[index + 1]     = 0;
    QByteArray syncPacket = QByteArray::fromRawData((char*)msgRaw, index);
    // Make deep copy
    syncPacket.detach();

    return syncPacket;
}
void PXMPeerWorkerPrivate::resultOfConnectionAttempt(evutil_socket_t socket,
                                                     const bool result,
                                                     bufferevent* bev,
                                                     const QUuid uuid)
{
    if (uuid.isNull()) {
        return;
    }

    if (result) {
        qInfo() << "Successful connection attempt to" << uuid.toString();
        // send internal comms for this bev to be enabled
        PXMServer::INTERNAL_MSG addBev                       = PXMServer::INTERNAL_MSG::ADD_DEFAULT_BEV;
        unsigned char bevPtr[PXMServer::INTERNAL_MSG_LENGTH] = {};
        memcpy(&bevPtr[0], &addBev, sizeof(addBev));
        memcpy(&bevPtr[sizeof(addBev)], &bev, sizeof(void*));
        bufferevent_write(internalBev, &bevPtr[0], PXMServer::INTERNAL_MSG_LENGTH);

        peersHash[uuid].bw->lockBev();
        if (peersHash.value(uuid).bw->getBev() != nullptr && peersHash.value(uuid).bw->getBev() != bev) {
            bufferevent_free(peersHash.value(uuid).bw->getBev());
        }

        peersHash.value(uuid).bw->setBev(bev);
        peersHash.value(uuid).bw->unlockBev();

        sendAuthPacket(peersHash.value(uuid).bw);
    } else {
        qWarning() << "Unsuccessful connection attempt to " << uuid.toString();
        peersHash[uuid].bw->lockBev();
        peersHash[uuid].bw->setBev(nullptr);
        bufferevent_free(bev);
        evutil_closesocket(socket);
        peersHash[uuid].bw->unlockBev();
        peersHash[uuid].connectTo = false;
        peersHash[uuid].isAuthed  = false;
        peersHash[uuid].socket    = -1;
    }
}
void PXMPeerWorkerPrivate::resultOfTCPSend(const int levelOfSuccess,
                                           const QUuid uuid,
                                           QString msg,
                                           const bool print,
                                           const QSharedPointer<Peers::BevWrapper>)
{
    if (print) {
        if (levelOfSuccess == 0) {
            formatMessage(msg, localUUID, Peers::selfColor);
        } else {
            qInfo() << "Send Failure";
        }
        q_ptr->addMessageToPeer(msg, uuid, false, true);
    }
    /*
    if(bwShortLife.contains(bw))
    {
    delete bw;
    }
    */
}
void PXMPeerWorkerPrivate::authHandler(const QString hostname,
                                       const unsigned short port,
                                       const QString version,
                                       const evutil_socket_t socket,
                                       const QUuid uuid,
                                       bufferevent* bev)
{
    if (uuid.isNull()) {
        evutil_closesocket(socket);
        bufferevent_free(bev);
        return;
    }
    struct sockaddr_in addr;
    socklen_t socklen = sizeof(addr);
    memset(&addr, 0, socklen);

    getpeername(socket, reinterpret_cast<struct sockaddr*>(&addr), &socklen);
    addr.sin_port = htons(port);

    qInfo().noquote() << hostname << "on port" << QString::number(port) << "authenticated!";

    if (peersHash[uuid].bw->getBev() != nullptr && peersHash[uuid].bw->getBev() != bev) {
        peerQuit(-1, peersHash.value(uuid).bw->getBev());
    } else {
        for (QSharedPointer<Peers::BevWrapper>& itr : extraBevs) {
            if (itr->getBev() == bev) {
                peersHash[uuid].bw = itr;
                extraBevs.removeAll(itr);
                break;
            }
        }
    }
    peersHash[uuid].uuid        = uuid;
    peersHash[uuid].addrRaw     = addr;
    peersHash[uuid].hostname    = hostname;
    peersHash[uuid].progVersion = version;
    peersHash[uuid].bw->lockBev();
    peersHash[uuid].bw->setBev(bev);
    peersHash[uuid].bw->unlockBev();
    peersHash[uuid].socket    = socket;
    peersHash[uuid].connectTo = true;
    peersHash[uuid].isAuthed  = true;

    emit q_ptr->newAuthedPeer(uuid, peersHash.value(uuid).hostname);
    emit requestSyncPacket(peersHash.value(uuid).bw, uuid);
}
int PXMPeerWorkerPrivate::msgHandler(QSharedPointer<QString> msgPtr, QUuid uuid, const bufferevent* bev, bool global)
{
    if (uuid != localUUID) {
        if (!(peersHash.contains(uuid))) {
            qWarning() << "Message from invalid uuid, rejection";
            return -1;
        }

        if (peersHash.value(uuid).bw->getBev() != bev) {
            bool foundIt      = false;
            QUuid uuidSpoofer = QUuid();
            for (auto& itr : peersHash) {
                if (itr.bw->getBev() == bev) {
                    foundIt     = true;
                    uuidSpoofer = itr.uuid;
                    continue;
                }
            }
            if (foundIt) {
                q_ptr->addMessageToPeer(
                    "This user is trying to spoof another "
                    "users uuid!",
                    uuidSpoofer, true, false);
            } else {
                q_ptr->addMessageToPeer(
                    "Someone is trying to spoof this users "
                    "uuid!",
                    uuid, true, false);
            }

            return -1;
        }
    }

    if (global) {
        if (uuid == localUUID) {
            formatMessage(*msgPtr.data(), uuid, Peers::selfColor);
        } else {
            formatMessage(*msgPtr.data(), uuid, peersHash.value(uuid).textColor);
        }

        uuid = globalUUID;
    } else {
        formatMessage(*msgPtr.data(), uuid, Peers::peerColor);
    }

    q_ptr->addMessageToPeer(msgPtr, uuid, true, true);
    return 0;
}
void PXMPeerWorkerPrivate::addMessageToAllPeers(QString str, bool alert, bool formatAsMessage)
{
    for (Peers::PeerData& itr : peersHash) {
        q_ptr->addMessageToPeer(str, itr.uuid, alert, formatAsMessage);
    }
}

int PXMPeerWorkerPrivate::formatMessage(QString& str, QUuid uuid, QString color)
{
    QRegularExpression qre("(<p.*?>)");
    QRegularExpressionMatch qrem = qre.match(str);
    int offset                   = qrem.capturedEnd(1);
    QDateTime dt                 = QDateTime::currentDateTime();
    QString date                 = QStringLiteral("(") % dt.time().toString("hh:mm:ss") % QStringLiteral(") ");
    str.insert(offset, QString("<span style=\"white-space: nowrap\" style=\"color: " % color % ";\">" % date %
                               peersHash.value(uuid).hostname % ":&nbsp;</span>"));

    return 0;
}

int PXMPeerWorker::addMessageToPeer(QString str, QUuid uuid, bool alert, bool)
{
    if (!d_ptr->peersHash.contains(uuid)) {
        return -1;
    }

    QSharedPointer<QString> pStr(new QString(str.toUtf8()));
    emit msgRecieved(pStr, uuid, alert);
    return 0;
}
int PXMPeerWorker::addMessageToPeer(QSharedPointer<QString> str, QUuid uuid, bool alert, bool)
{
    if (!d_ptr->peersHash.contains(uuid)) {
        return -1;
    }

    emit msgRecieved(str, uuid, alert);
    return 0;
}
void PXMPeerWorkerPrivate::setSelfCommsBufferevent(bufferevent* bev)
{
    peersHash.value(localUUID).bw->lockBev();
    peersHash.value(localUUID).bw->setBev(bev);
    peersHash.value(localUUID).bw->unlockBev();

    q_ptr->newAuthedPeer(localUUID, localHostname);
    // addMessageToPeer("<!DOCTYPE html><html><body><style>h2, p {margin:"
    // "0;}h2 {text-align: center;}p {text-align: left;}</style><h2>" %
    // d_ptr->localHostname % "</h2></body></html>",
    // d_ptr->localUUID, false, false);
}
void PXMPeerWorker::sendMsgAccessor(QByteArray msg, PXMConsts::MESSAGE_TYPE type, QUuid uuid)
{
    using namespace PXMConsts;
    switch (type) {
        case MSG_TEXT:
            if (!uuid.isNull())
                emit sendMsg(d_ptr->peersHash.value(uuid).bw, msg, msg.size(), MSG_TEXT, uuid);
            else
                qWarning() << "Bad recipient uuid for normal message";
            break;
        case MSG_GLOBAL:
            for (auto& itr : d_ptr->peersHash) {
                if (itr.isAuthed || itr.uuid == d_ptr->localUUID) {
                    emit sendMsg(itr.bw, msg, msg.size(), MSG_GLOBAL);
                }
            }
            break;
        case MSG_NAME:
            d_ptr->peersHash[d_ptr->localUUID].hostname = QString(msg);
            d_ptr->localHostname                        = QString(msg);
            for (auto& itr : d_ptr->peersHash) {
                if (itr.isAuthed || itr.uuid == d_ptr->localUUID) {
                    emit sendMsg(itr.bw, msg, msg.size(), MSG_NAME);
                }
            }
            break;
        default:
            qWarning() << "Bad message type in sendMsgAccessor";
            break;
    }
}

void PXMPeerWorkerPrivate::setLibeventBackend(QString back, QString vers)
{
    libeventBackend = back;
    libeventVers    = vers;
}
void PXMPeerWorker::printInfoToDebug()
{
    // hopefully reduce reallocs here
    QString str;
    str.reserve(
        (330 + (PXMConsts::DEBUG_PADDING * 16) + (d_ptr->peersHash.size() * (260 + (9 * PXMConsts::DEBUG_PADDING)))));

    str.append(QChar('\n') % QStringLiteral("---Program Info---\n") % QStringLiteral("Program Name: ") %
               qApp->applicationName() % QChar('\n') % QStringLiteral("Version: ") % qApp->applicationVersion() %
               QChar('\n') % QStringLiteral("Qt Version: ") % qVersion() % QChar('\n') %
               QStringLiteral("---Network Info---\n") % QStringLiteral("Libevent Version: ") % d_ptr->libeventVers %
               QChar('\n') % QStringLiteral("Libevent Backend: ") % d_ptr->libeventBackend % QChar('\n') %
               QStringLiteral("Multicast Address: ") % d_ptr->multicastAddress % QChar('\n') %
               QStringLiteral("TCP Listener Port: ") % QString::number(d_ptr->serverTCPPort) % QChar('\n') %
               QStringLiteral("UDP Listener Port: ") % QString::number(d_ptr->serverUDPPort) % QChar('\n') %
               QStringLiteral("Our UUID: ") % d_ptr->localUUID.toString() % QChar('\n') %
               QStringLiteral("MulticastIsFunctioning: ") %
               QString::fromLocal8Bit((d_ptr->multicastIsFunctioning ? "true" : "false")) % QChar('\n') %
               QStringLiteral("Sync waiting list: ") % QString::number(d_ptr->syncablePeers->length()) % QChar('\n') %
               QStringLiteral("---Peer Details---\n"));

    int peerCount = 0;
    for (Peers::PeerData& itr : d_ptr->peersHash) {
        str.append(QStringLiteral("---Peer #") % QString::number(peerCount) % QStringLiteral("---\n") %
                   itr.toInfoString());
        peerCount++;
    }

    str.append(QStringLiteral("-------------\n") % QStringLiteral("Total Peers: ") % QString::number(peerCount) %
               QChar('\n'));
    str.squeeze();
    qInfo().noquote() << str;
}
void PXMPeerWorkerPrivate::discoveryTimerPersistent()
{
    if (peersHash.count() < 3) {
        emit q_ptr->sendUDP("/discover", serverUDPPort);
    } else {
        qDebug() << "Found enough peers";
        discoveryTimer->stop();
    }
}
void PXMPeerWorkerPrivate::discoveryTimerSingleShot()
{
    if (!multicastIsFunctioning) {
        emit q_ptr->warnBox(QStringLiteral("Network Problem"),
                            QStringLiteral("Could not find anyone, even ourselves, on "
                                           "the network.\nThis could indicate a "
                                           "problem with your configuration."
                                           "\n\nWe'll keep looking..."));
    }
    if (peersHash.count() < 3) {
        discoveryTimer->setInterval(30000);
        QObject::connect(discoveryTimer, &QTimer::timeout, this, &PXMPeerWorkerPrivate::discoveryTimerPersistent);
        emit q_ptr->sendUDP("/discover", serverUDPPort);
        qInfo().noquote() << QStringLiteral(
            "Retrying discovery packet, looking "
            "for other computers...");
        discoveryTimer->start();
    } else {
        qDebug() << "Found enough peers";
    }
}

void PXMPeerWorkerPrivate::midnightTimerPersistent()
{
    QDateTime dt = QDateTime::currentDateTime();
    if (dt.time() <= QTime(0, MIDNIGHT_TIMER_INTERVAL_MINUTES, 0, 0)) {
        QString str = QString();
        str.append(
            "<br>"
            "<center>" %
            QString(7, QChar('-')) % QChar(' ') % dt.date().toString() % QChar(' ') % QString(7, QChar('-')) %
            "</center>"
            "<br>");

        emit addMessageToAllPeers(str, false, false);
    }
}

void PXMPeerWorkerPrivate::multicastIsFunctional()
{
    multicastIsFunctioning = true;
}

void PXMPeerWorkerPrivate::serverSetupFailure(QString error)
{
    discoveryTimer->stop();
    discoveryTimerSingle->stop();
    emit q_ptr->warnBox(QStringLiteral("Server Setup Failure"),
                        QStringLiteral("Server Setup failure:") % "\n" % error % "\n" %
                            QStringLiteral("Settings for your network conditions will "
                                           "have to be adjusted and the program "
                                           "restarted."));
}

void PXMPeerWorkerPrivate::setLocalHostname(QString hname)
{
    localHostname = hname;
}

void PXMPeerWorker::sendUDPAccessor(const char* msg)
{
    emit sendUDP(msg, d_ptr->serverUDPPort);
}
/*void PXMPeerWorker::restartServer(unsigned short tcpPort, unsigned udpPort,
QString multicast)
{
    if(d_ptr->messServer)
    {
    for(auto &itr : d_ptr->peerDetailsHash)
    {
        //qDeleteAll(itr.messages);
        evutil_closesocket(itr.socket);
    }
    //This must be done before PXMServer is shutdown;
    d_ptr->peerDetailsHash.clear();

    if(d_ptr->messServer != 0 && d_ptr->messServer->isRunning())
    {
        bufferevent_write(d_ptr->closeBev, "/exit", 5);
        d_ptr->messServer->wait(5000);
    }
    }
}
*/

#include "moc_pxmpeerworker.cpp"
