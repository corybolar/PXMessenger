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

using namespace PXMConsts;

class PXMPeerWorkerPrivate
{
    Q_DISABLE_COPY(PXMPeerWorkerPrivate)
    Q_DECLARE_PUBLIC(PXMPeerWorker)

   public:
    PXMPeerWorkerPrivate(PXMPeerWorker* q) : q_ptr(q) {}
    PXMPeerWorkerPrivate(PXMPeerWorker* q,
                         QString username,
                         QUuid selfUUID,
                         QString multicast,
                         unsigned short tcpPort,
                         unsigned short udpPort,
                         QUuid globaluuid)
        : q_ptr(q),
          localHostname(username),
          localUUID(selfUUID),
          multicastAddress(multicast),
          globalUUID(globaluuid),
          syncablePeers(new TimedVector<QUuid>(q_ptr->SYNC_TIMEOUT_MSECS, SECONDS)),
          serverTCPPort(tcpPort),
          serverUDPPort(udpPort)
    {
    }
    PXMPeerWorker* const q_ptr;
    // Data Members

    QHash<QUuid, Peers::PeerData> peersHash;
    QString localHostname;
    QUuid localUUID;
    QString multicastAddress;
    QString libeventBackend;
    QUuid globalUUID;
    QTimer* syncTimer;
    QTimer* nextSyncTimer;
    QTimer* discoveryTimer;
    QTimer* discoveryTimerSingle;
    QTimer* midnightTimer;
    PXMSync* syncer;
    QVector<Peers::BevWrapper*> bwShortLife;
    PXMServer::ServerThread* messServer;
    PXMClient* messClient;
    bufferevent* internalBev;
    QVector<QSharedPointer<Peers::BevWrapper>> extraBevs;
    QScopedPointer<TimedVector<QUuid>> syncablePeers;
    unsigned short serverTCPPort;
    unsigned short serverUDPPort;
    bool areWeSyncing;
    bool multicastIsFunctioning;

    // Functions
    void sendAuthPacket(QSharedPointer<Peers::BevWrapper> bw);
    void startServer();
    void connectClient();
    int formatMessage(QString& str, QUuid uuid, QString color);

    // Slots
};
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
    using namespace PXMServer;
    d_ptr->syncTimer->stop();
    d_ptr->nextSyncTimer->stop();
    // delete d_ptr->syncablePeers;

    for (auto& itr : d_ptr->peersHash) {
        // qDeleteAll(itr.messages);
        evutil_closesocket(itr.socket);
    }
    // Strange memory interaction with libevent, for now set ourSelfComms
    // bufferevent to null to prevent it being auto freed when its removed
    // from the hash
    d_ptr->peersHash[d_ptr->localUUID].bw->setBev(nullptr);
    // This must be done before PXMServer is shutdown;
    d_ptr->peersHash.clear();

    if (d_ptr->messServer != 0 && d_ptr->messServer->isRunning()) {
        INTERNAL_MSG exit                 = INTERNAL_MSG::EXIT;
        char exitMsg[INTERNAL_MSG_LENGTH] = {};
        memcpy(&exitMsg[0], &exit, sizeof(exit));
        bufferevent_write(d_ptr->internalBev, &exitMsg[0], INTERNAL_MSG_LENGTH);
        d_ptr->messServer->wait(5000);
    }

    qDebug() << "Shutdown of PXMPeerWorker Successful";
}
void PXMPeerWorker::setInternalBufferevent(bufferevent* bev)
{
    d_ptr->internalBev = bev;
}
void PXMPeerWorker::currentThreadInit()
{
    d_ptr->syncer = new PXMSync(this, d_ptr->peersHash);
    QObject::connect(d_ptr->syncer, &PXMSync::requestIps, this, &PXMPeerWorker::requestSyncPacket);
    QObject::connect(d_ptr->syncer, &PXMSync::syncComplete, this, &PXMPeerWorker::donePeerSync);

    srand(static_cast<unsigned int>(time(NULL)));

    d_ptr->syncTimer = new QTimer(this);
    d_ptr->syncTimer->setInterval((rand() % 300000) + SYNC_TIMER);
    QObject::connect(d_ptr->syncTimer, &QTimer::timeout, this, &PXMPeerWorker::beginPeerSync);
    d_ptr->syncTimer->start();

    d_ptr->nextSyncTimer = new QTimer(this);
    d_ptr->nextSyncTimer->setInterval(2000);
    QObject::connect(d_ptr->nextSyncTimer, &QTimer::timeout, d_ptr->syncer, &PXMSync::syncNext);

    d_ptr->discoveryTimerSingle = new QTimer(this);
    d_ptr->discoveryTimerSingle->setSingleShot(true);
    d_ptr->discoveryTimerSingle->setInterval(5000);
    QObject::connect(d_ptr->discoveryTimerSingle, &QTimer::timeout, this, &PXMPeerWorker::discoveryTimerSingleShot);
    d_ptr->discoveryTimerSingle->start();

    d_ptr->midnightTimer = new QTimer(this);
    d_ptr->midnightTimer->setInterval(MIDNIGHT_TIMER_INTERVAL_MINUTES * 60000);
    QObject::connect(d_ptr->midnightTimer, &QTimer::timeout, this, &PXMPeerWorker::midnightTimerPersistent);
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
    QObject::connect(messServer, &PXMServer::ServerThread::authHandler, q_ptr, &PXMPeerWorker::authHandler,
                     Qt::QueuedConnection);
    QObject::connect(messServer, &PXMServer::ServerThread::msgHandler, q_ptr, &PXMPeerWorker::msgHandler,
                     Qt::QueuedConnection);
    QObject::connect(messServer, &QThread::finished, messServer, &QObject::deleteLater);
    QObject::connect(messServer, &PXMServer::ServerThread::newTCPConnection, q_ptr,
                     &PXMPeerWorker::newAcceptedConnection, Qt::QueuedConnection);
    QObject::connect(messServer, &PXMServer::ServerThread::peerQuit, q_ptr, &PXMPeerWorker::peerQuit,
                     Qt::QueuedConnection);
    QObject::connect(messServer, &PXMServer::ServerThread::attemptConnection, q_ptr, &PXMPeerWorker::attemptConnection,
                     Qt::QueuedConnection);
    QObject::connect(messServer, &PXMServer::ServerThread::syncRequestHandler, q_ptr,
                     &PXMPeerWorker::syncRequestHandlerBev, Qt::QueuedConnection);
    QObject::connect(messServer, &PXMServer::ServerThread::syncHandler, q_ptr, &PXMPeerWorker::syncHandler,
                     Qt::QueuedConnection);
    QObject::connect(messServer, &PXMServer::ServerThread::setPeerHostname, q_ptr, &PXMPeerWorker::nameHandler,
                     Qt::QueuedConnection);
    QObject::connect(messServer, &PXMServer::ServerThread::setListenerPorts, q_ptr, &PXMPeerWorker::setListenerPorts,
                     Qt::QueuedConnection);
    QObject::connect(messServer, &PXMServer::ServerThread::libeventBackend, q_ptr, &PXMPeerWorker::setlibeventBackend,
                     Qt::QueuedConnection);
    QObject::connect(messServer, &PXMServer::ServerThread::setInternalBufferevent, q_ptr,
                     &PXMPeerWorker::setInternalBufferevent, Qt::QueuedConnection);
    QObject::connect(messServer, &PXMServer::ServerThread::setSelfCommsBufferevent, q_ptr,
                     &PXMPeerWorker::setSelfCommsBufferevent, Qt::QueuedConnection);
    QObject::connect(messServer, &PXMServer::ServerThread::multicastIsFunctional, q_ptr,
                     &PXMPeerWorker::multicastIsFunctional, Qt::QueuedConnection);
    QObject::connect(messServer, &PXMServer::ServerThread::serverSetupFailure, q_ptr,
                     &PXMPeerWorker::serverSetupFailure, Qt::QueuedConnection);
    QObject::connect(messServer, &PXMServer::ServerThread::sendUDP, q_ptr, &PXMPeerWorker::sendUDP,
                     Qt::QueuedConnection);
    QObject::connect(messServer, &PXMServer::ServerThread::nameHandler, q_ptr, &PXMPeerWorker::nameHandler,
                     Qt::QueuedConnection);
    QObject::connect(messServer, &PXMServer::ServerThread::resultOfConnectionAttempt, q_ptr,
                     &PXMPeerWorker::resultOfConnectionAttempt, Qt::QueuedConnection);
    messServer->start();
}
void PXMPeerWorkerPrivate::connectClient()
{
    QObject::connect(messClient, &PXMClient::resultOfTCPSend, q_ptr, &PXMPeerWorker::resultOfTCPSend,
                     Qt::QueuedConnection);
    QObject::connect(q_ptr, &PXMPeerWorker::sendMsg, messClient, &PXMClient::sendMsgSlot, Qt::QueuedConnection);
    QObject::connect(q_ptr, &PXMPeerWorker::sendSyncPacket, messClient, &PXMClient::sendIpsSlot, Qt::QueuedConnection);
    QObject::connect(q_ptr, &PXMPeerWorker::sendUDP, messClient, &PXMClient::sendUDP, Qt::QueuedConnection);
}

void PXMPeerWorker::setListenerPorts(unsigned short tcpport, unsigned short udpport)
{
    d_ptr->serverTCPPort = tcpport;
    d_ptr->serverUDPPort = udpport;
}
void PXMPeerWorker::donePeerSync()
{
    d_ptr->areWeSyncing = false;
    d_ptr->nextSyncTimer->stop();
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
void PXMPeerWorker::syncHandler(QSharedPointer<unsigned char> syncPacket, size_t len, QUuid senderUuid)
{
    if (!d_ptr->syncablePeers->contains(senderUuid)) {
        qWarning() << "Sync packet from bad uuid -- timeout or not "
                      "requested"
                   << senderUuid;
        return;
    }
    qInfo() << "Sync packet from" << senderUuid.toString();

    size_t index = 0;
    while (index + NetCompression::PACKED_UUID_LENGTH + 6 <= len) {
        struct sockaddr_in addr;
        index += NetCompression::unpackSockaddr_in(&syncPacket.data()[index], addr);
        addr.sin_family = AF_INET;
        QUuid uuid      = QUuid();
        index += NetCompression::unpackUUID(&syncPacket.data()[index], uuid);

        qInfo() << inet_ntoa(addr.sin_addr) << ":" << ntohs(addr.sin_port) << ":" << uuid.toString();
        attemptConnection(addr, uuid);
    }
    qInfo() << "End of sync packet";

    d_ptr->syncablePeers->remove(senderUuid);

    if (d_ptr->areWeSyncing) {
        d_ptr->nextSyncTimer->start(2000);
        d_ptr->syncer->syncNext();
    }
}
void PXMPeerWorker::newAcceptedConnection(bufferevent* bev)
{
    QSharedPointer<Peers::BevWrapper> bw(new Peers::BevWrapper);
    bw->setBev(bev);
    d_ptr->extraBevs.push_back(bw);
    d_ptr->sendAuthPacket(bw);
}
void PXMPeerWorkerPrivate::sendAuthPacket(QSharedPointer<Peers::BevWrapper> bw)
{
    // Auth packet format "Hostname:::12345:::001.001.001
    emit q_ptr->sendMsg(bw, (localHostname % QString::fromLatin1(AUTH_SEPERATOR) % QString::number(serverTCPPort) %
                             QString::fromLatin1(AUTH_SEPERATOR) % qApp->applicationVersion())
                                .toUtf8(),
                        MSG_AUTH);
}
void PXMPeerWorker::requestSyncPacket(QSharedPointer<Peers::BevWrapper> bw, QUuid uuid)
{
    d_ptr->syncablePeers->append(uuid);
    qInfo() << "Requesting ips from" << d_ptr->peersHash.value(uuid).hostname;
    emit sendMsg(bw, QByteArray(), MSG_SYNC_REQUEST);
}
void PXMPeerWorker::attemptConnection(struct sockaddr_in addr, QUuid uuid)
{
    if (d_ptr->peersHash.value(uuid).connectTo || uuid.isNull()) {
        return;
    }

    d_ptr->peersHash[uuid].uuid      = uuid;
    d_ptr->peersHash[uuid].connectTo = true;
    d_ptr->peersHash[uuid].addrRaw   = addr;

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
    bufferevent_write(d_ptr->internalBev, &bevPtr[0], PXMServer::INTERNAL_MSG_LENGTH);
}
void PXMPeerWorker::nameHandler(QString hname, const QUuid uuid)
{
    if (d_ptr->peersHash.contains(uuid)) {
        d_ptr->peersHash[uuid].hostname = hname.left(PXMConsts::MAX_HOSTNAME_LENGTH + PXMConsts::MAX_COMPUTER_NAME);
        emit newAuthedPeer(uuid, d_ptr->peersHash.value(uuid).hostname);
    }
}
void PXMPeerWorker::peerQuit(evutil_socket_t s, bufferevent* bev)
{
    for (Peers::PeerData& itr : d_ptr->peersHash) {
        if (itr.bw->getBev() == bev) {
            d_ptr->peersHash[itr.uuid].connectTo = false;
            d_ptr->peersHash[itr.uuid].isAuthed  = false;
            d_ptr->peersHash[itr.uuid].bw->lockBev();
            qInfo().noquote() << "Peer:" << itr.uuid.toString() << "has disconnected";
            d_ptr->peersHash[itr.uuid].bw->setBev(nullptr);
            bufferevent_free(bev);
            d_ptr->peersHash[itr.uuid].bw->unlockBev();
            evutil_closesocket(itr.socket);
            d_ptr->peersHash[itr.uuid].socket = -1;
            emit connectionStatusChange(itr.uuid, 1);
            return;
        }
    }
    for (QSharedPointer<Peers::BevWrapper>& itr : d_ptr->extraBevs) {
        d_ptr->extraBevs.removeAll(itr);
    }
    qInfo().noquote() << "Non-Authed Peer has quit";
    evutil_closesocket(s);
}
void PXMPeerWorker::syncRequestHandlerBev(const bufferevent* bev, const QUuid uuid)
{
    if (d_ptr->peersHash.contains(uuid) && d_ptr->peersHash.value(uuid).bw->getBev() == bev) {
        size_t index                         = 0;
        QSharedPointer<unsigned char> packet = createSyncPacket(index);
        if (d_ptr->messClient && index != 0) {
            qInfo() << "Sending ips to" << d_ptr->peersHash.value(uuid).hostname;
            emit sendSyncPacket(d_ptr->peersHash.value(uuid).bw, packet, index, MSG_SYNC);
        } else {
            qCritical() << "messClient not initialized";
        }
    } else {
        qCritical() << "sendIpsBev:error";
    }
}
QSharedPointer<unsigned char> PXMPeerWorker::createSyncPacket(size_t& index)
{
    QSharedPointer<unsigned char> msgRaw(
        new unsigned char[static_cast<size_t>(d_ptr->peersHash.size()) *
                              (NetCompression::PACKED_SOCKADDR_IN_LENGTH + NetCompression::PACKED_UUID_LENGTH) +
                          1]);
    index = 0;
    for (Peers::PeerData& itr : d_ptr->peersHash) {
        if (itr.isAuthed) {
            index += NetCompression::packSockaddr_in(&msgRaw.data()[index], itr.addrRaw);
            index += NetCompression::packUUID(&msgRaw.data()[index], itr.uuid);
        }
    }
    msgRaw.data()[index + 1] = 0;
    return msgRaw;
}
void PXMPeerWorker::resultOfConnectionAttempt(evutil_socket_t socket,
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
        bufferevent_write(d_ptr->internalBev, &bevPtr[0], PXMServer::INTERNAL_MSG_LENGTH);

        d_ptr->peersHash[uuid].bw->lockBev();
        if (d_ptr->peersHash.value(uuid).bw->getBev() != nullptr && d_ptr->peersHash.value(uuid).bw->getBev() != bev) {
            bufferevent_free(d_ptr->peersHash.value(uuid).bw->getBev());
        }

        d_ptr->peersHash.value(uuid).bw->setBev(bev);
        d_ptr->peersHash.value(uuid).bw->unlockBev();

        d_ptr->sendAuthPacket(d_ptr->peersHash.value(uuid).bw);
    } else {
        qWarning() << "Unsuccessful connection attempt to " << uuid.toString();
        d_ptr->peersHash[uuid].bw->lockBev();
        d_ptr->peersHash[uuid].bw->setBev(nullptr);
        bufferevent_free(bev);
        evutil_closesocket(socket);
        d_ptr->peersHash[uuid].bw->unlockBev();
        d_ptr->peersHash[uuid].connectTo = false;
        d_ptr->peersHash[uuid].isAuthed  = false;
        d_ptr->peersHash[uuid].socket    = -1;
    }
}
void PXMPeerWorker::resultOfTCPSend(const int levelOfSuccess,
                                    const QUuid uuid,
                                    QString msg,
                                    const bool print,
                                    const QSharedPointer<Peers::BevWrapper>)
{
    if (print) {
        if (levelOfSuccess == 0) {
            d_ptr->formatMessage(msg, d_ptr->localUUID, Peers::selfColor);
        } else {
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
void PXMPeerWorker::authHandler(const QString hostname,
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

    if (d_ptr->peersHash[uuid].bw->getBev() != nullptr && d_ptr->peersHash[uuid].bw->getBev() != bev) {
        peerQuit(-1, d_ptr->peersHash.value(uuid).bw->getBev());
    } else {
        for (QSharedPointer<Peers::BevWrapper>& itr : d_ptr->extraBevs) {
            if (itr->getBev() == bev) {
                d_ptr->peersHash[uuid].bw = itr;
                d_ptr->extraBevs.removeAll(itr);
                break;
            }
        }
    }
    d_ptr->peersHash[uuid].uuid        = uuid;
    d_ptr->peersHash[uuid].addrRaw     = addr;
    d_ptr->peersHash[uuid].hostname    = hostname;
    d_ptr->peersHash[uuid].progVersion = version;
    d_ptr->peersHash[uuid].bw->lockBev();
    d_ptr->peersHash[uuid].bw->setBev(bev);
    d_ptr->peersHash[uuid].bw->unlockBev();
    d_ptr->peersHash[uuid].socket    = socket;
    d_ptr->peersHash[uuid].connectTo = true;
    d_ptr->peersHash[uuid].isAuthed  = true;

    emit newAuthedPeer(uuid, d_ptr->peersHash.value(uuid).hostname);
    emit requestSyncPacket(d_ptr->peersHash.value(uuid).bw, uuid);
}
int PXMPeerWorker::msgHandler(QString str, QUuid uuid, const bufferevent* bev, bool global)
{
    if (uuid != d_ptr->localUUID) {
        if (!(d_ptr->peersHash.contains(uuid))) {
            qWarning() << "Message from invalid uuid, rejection";
            return -1;
        }

        if (d_ptr->peersHash.value(uuid).bw->getBev() != bev) {
            bool foundIt      = false;
            QUuid uuidSpoofer = QUuid();
            for (auto& itr : d_ptr->peersHash) {
                if (itr.bw->getBev() == bev) {
                    foundIt     = true;
                    uuidSpoofer = itr.uuid;
                    continue;
                }
            }
            if (foundIt) {
                addMessageToPeer(
                    "This user is trying to spoof another "
                    "users uuid!",
                    uuidSpoofer, true, false);
            } else {
                addMessageToPeer(
                    "Someone is trying to spoof this users "
                    "uuid!",
                    uuid, true, false);
            }

            return -1;
        }
    }

    if (global) {
        if (uuid == d_ptr->localUUID) {
            d_ptr->formatMessage(str, uuid, Peers::selfColor);
        } else {
            d_ptr->formatMessage(str, uuid, d_ptr->peersHash.value(uuid).textColor);
        }

        uuid = d_ptr->globalUUID;
    } else {
        d_ptr->formatMessage(str, uuid, Peers::peerColor);
    }

    addMessageToPeer(str, uuid, true, true);
    return 0;
}
void PXMPeerWorker::addMessageToAllPeers(QString str, bool alert, bool formatAsMessage)
{
    for (Peers::PeerData& itr : d_ptr->peersHash) {
        addMessageToPeer(str, itr.uuid, alert, formatAsMessage);
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
void PXMPeerWorker::setSelfCommsBufferevent(bufferevent* bev)
{
    d_ptr->peersHash.value(d_ptr->localUUID).bw->lockBev();
    d_ptr->peersHash.value(d_ptr->localUUID).bw->setBev(bev);
    d_ptr->peersHash.value(d_ptr->localUUID).bw->unlockBev();

    newAuthedPeer(d_ptr->localUUID, d_ptr->localHostname);
    // addMessageToPeer("<!DOCTYPE html><html><body><style>h2, p {margin:"
    // "0;}h2 {text-align: center;}p {text-align: left;}</style><h2>" %
    // d_ptr->localHostname % "</h2></body></html>",
    // d_ptr->localUUID, false, false);
}
void PXMPeerWorker::sendMsgAccessor(QByteArray msg, MESSAGE_TYPE type, QUuid uuid)
{
    switch (type) {
        case MSG_TEXT:
            if (!uuid.isNull())
                emit sendMsg(d_ptr->peersHash.value(uuid).bw, msg, MSG_TEXT, uuid);
            else
                qWarning() << "Bad recipient uuid for normal message";
            break;
        case MSG_GLOBAL:
            for (auto& itr : d_ptr->peersHash) {
                if (itr.isAuthed || itr.uuid == d_ptr->localUUID) {
                    emit sendMsg(itr.bw, msg, MSG_GLOBAL);
                }
            }
            break;
        case MSG_NAME:
            d_ptr->peersHash[d_ptr->localUUID].hostname = QString(msg);
            d_ptr->localHostname                        = QString(msg);
            for (auto& itr : d_ptr->peersHash) {
                if (itr.isAuthed || itr.uuid == d_ptr->localUUID) {
                    emit sendMsg(itr.bw, msg, MSG_NAME);
                }
            }
            break;
        default:
            qWarning() << "Bad message type in sendMsgAccessor";
            break;
    }
}

void PXMPeerWorker::setlibeventBackend(QString str)
{
    d_ptr->libeventBackend = str;
}
void PXMPeerWorker::printInfoToDebug()
{
    // hopefully reduce reallocs here
    QString str;
    str.reserve((330 + (DEBUG_PADDING * 16) + (d_ptr->peersHash.size() * (260 + (9 * DEBUG_PADDING)))));

    str.append(QChar('\n') % QStringLiteral("---Program Info---\n") % QStringLiteral("Program Name: ") %
               qApp->applicationName() % QChar('\n') % QStringLiteral("Version: ") % qApp->applicationVersion() %
               QChar('\n') % QStringLiteral("---Network Info---\n") % QStringLiteral("Libevent Backend: ") %
               d_ptr->libeventBackend % QChar('\n') % QStringLiteral("Multicast Address: ") % d_ptr->multicastAddress %
               QChar('\n') % QStringLiteral("TCP Listener Port: ") % QString::number(d_ptr->serverTCPPort) %
               QChar('\n') % QStringLiteral("UDP Listener Port: ") % QString::number(d_ptr->serverUDPPort) %
               QChar('\n') % QStringLiteral("Our UUID: ") % d_ptr->localUUID.toString() % QChar('\n') %
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
void PXMPeerWorker::discoveryTimerPersistent()
{
    if (d_ptr->peersHash.count() < 3) {
        emit sendUDP("/discover", d_ptr->serverUDPPort);
    } else {
        qInfo() << "Found enough peers";
        d_ptr->discoveryTimer->stop();
    }
}
void PXMPeerWorker::discoveryTimerSingleShot()
{
    if (!d_ptr->multicastIsFunctioning) {
        warnBox(QStringLiteral("Network Problem"), QStringLiteral("Could not find anyone, even ourselves, on "
                                                                  "the network.\nThis could indicate a "
                                                                  "problem with your configuration."
                                                                  "\n\nWe'll keep looking..."));
    }
    if (d_ptr->peersHash.count() < 3) {
        d_ptr->discoveryTimer->setInterval(30000);
        QObject::connect(d_ptr->discoveryTimer, &QTimer::timeout, this, &PXMPeerWorker::discoveryTimerPersistent);
        emit sendUDP("/discover", d_ptr->serverUDPPort);
        qInfo().noquote() << QStringLiteral(
            "Retrying discovery packet, looking "
            "for other computers...");
        d_ptr->discoveryTimer->start();
    } else {
        qInfo() << "Found enough peers";
    }
}

void PXMPeerWorker::midnightTimerPersistent()
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

void PXMPeerWorker::multicastIsFunctional()
{
    d_ptr->multicastIsFunctioning = true;
}

void PXMPeerWorker::serverSetupFailure(QString error)
{
    d_ptr->discoveryTimer->stop();
    d_ptr->discoveryTimerSingle->stop();
    emit warnBox(QStringLiteral("Server Setup Failure"), QStringLiteral("Server Setup failure:") % "\n" % error % "\n" %
                                                             QStringLiteral("Settings for your network conditions will "
                                                                            "have to be adjusted and the program "
                                                                            "restarted."));
}

void PXMPeerWorker::setLocalHostname(QString hname)
{
    d_ptr->localHostname = hname;
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
