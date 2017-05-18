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
class PXMPeerWorkerPrivate : public QObject
{
    // Q_OBJECT
    Q_DISABLE_COPY(PXMPeerWorkerPrivate)
    Q_DECLARE_PUBLIC(PXMPeerWorker)

    const unsigned int SYNCREQUEST_TIMEOUT_MSECS = 2000;

   public:
    // PXMPeerWorkerPrivate(PXMPeerWorker* q) : QObject(q), q_ptr(q) { this->init(); }
    PXMPeerWorkerPrivate(PXMPeerWorker* q,
                         QString username,
                         QUuid selfUUID,
                         QString multicast,
                         unsigned short tcpPort,
                         unsigned short udpPort,
                         QUuid globaluuid)
        : QObject(q),
          q_ptr(q),
          localHostname(username),
          localUUID(selfUUID),
          multicastAddress(multicast),
          globalUUID(globaluuid),
          syncablePeers(new TimedVector<QUuid>(SYNCREQUEST_TIMEOUT_MSECS)),
          serverTCPPort(tcpPort),
          serverUDPPort(udpPort)
    {
        this->init();
    }
    ~PXMPeerWorkerPrivate()
    {
        using namespace PXMServer;
        syncTimer->stop();
        nextSyncTimer->stop();
        // delete d_ptr->syncablePeers;

        for (auto& itr : peersHash) {
            // qDeleteAll(itr.messages);
            evutil_closesocket(itr.socket);
        }
        // Strange memory interaction with libevent, for now set ourSelfComms
        // bufferevent to null to prevent it being auto freed when its removed
        // from the hash
        peersHash[localUUID].bw->setBev(nullptr);
        // This must be done before PXMServer is shutdown;
        peersHash.clear();

        if (server != 0 && server->isRunning()) {
            INTERNAL_MSG exit                 = INTERNAL_MSG::EXIT;
            char exitMsg[INTERNAL_MSG_LENGTH] = {};
            memcpy(&exitMsg[0], &exit, sizeof(exit));
            bufferevent_write(internalBev, &exitMsg[0], INTERNAL_MSG_LENGTH);
            server->wait(5000);
        }
    }

   private:
    PXMPeerWorker* const q_ptr;
    // Data Members

    const size_t MIDNIGHT_TIMER_INTERVAL_MINUTES = 1;
    const int SYNC_TIMER_MSECS                   = 900000;
    QHash<QUuid, Peers::PeerData> peersHash;
    QString localHostname;
    QUuid localUUID;
    QString multicastAddress;
    QString libeventBackend;
    QString libeventVers;
    QUuid globalUUID;
    QTimer* syncTimer;
    QTimer* nextSyncTimer;
    QTimer* discoveryTimer;
    QTimer* discoveryTimerSingle;
    QTimer* midnightTimer;
    PXMSync* syncer;
    PXMServer::ServerThread* server;
    PXMClient* client;
    bufferevent* internalBev;
    QVector<QSharedPointer<Peers::BevWrapper>> extraBevs;
    QScopedPointer<TimedVector<QUuid>> syncablePeers;
    unsigned short serverTCPPort;
    unsigned short serverUDPPort;
    bool areWeSyncing;
    bool multicastIsFunctioning;

    // Functions
    void sendAuthPacket(QSharedPointer<Peers::BevWrapper> bw);
    // int formatMessage(QString& str, QUuid uuid, QString color);
    QByteArray createSyncPacket(size_t& index);
    void startServer();
    void connectClient();
    int msgHandler(QByteArray msg, QUuid uuid, const bufferevent* bev, bool global);
    void authHandler(const QString hostname,
                     const unsigned short port,
                     const QString version,
                     const evutil_socket_t socket,
                     const QUuid uuid,
                     bufferevent* bev);
    void syncHandler(QSharedPointer<unsigned char> syncPacket, size_t len, QUuid senderUuid);
    void nameHandler(QString hname, const QUuid uuid);
    void syncRequestHandlerBev(const bufferevent* bev, const QUuid uuid);
    void init();
    // Slots
   private slots:
    void packetHandler(const QSharedPointer<unsigned char> packet,
                       const size_t len,
                       const PXMConsts::MESSAGE_TYPE type,
                       const QUuid uuid,
                       const bufferevent* bev);

    void newAcceptedConnection(bufferevent* bev);
    void attemptConnection(struct sockaddr_in addr, QUuid uuid);
    void resultOfConnectionAttempt(evutil_socket_t socket, const bool result, bufferevent* bev, const QUuid uuid);

    void peerQuit(evutil_socket_t s, bufferevent* bev);
    void addMessageToAllPeers(QString str, bool alert, bool formatAsMessage);

    void setSelfCommsBufferevent(bufferevent* bev);
    void setLocalHostname(QString);
    void setInternalBufferevent(bufferevent* bev);
    void setListenerPorts(unsigned short tcpport, unsigned short udpport);
    void setLibeventBackend(QString back, QString vers);

    void serverSetupFailure(QString error);

    void donePeerSync();
    void requestSyncPacket(QSharedPointer<Peers::BevWrapper> bw, QUuid uuid);
    void resultOfTCPSend(const int levelOfSuccess,
                         const QUuid uuid,
                         QString msg,
                         const bool print,
                         const QSharedPointer<Peers::BevWrapper>);
    void discoveryTimerSingleShot();
    void midnightTimerPersistent();
    void discoveryTimerPersistent();
    int addMessageToPeer(QSharedPointer<QString> str, QUuid uuid, bool alert, bool, bool global);

    void multicastIsFunctional();
    void setupTimers();
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
}
PXMPeerWorker::~PXMPeerWorker()
{
    qDebug() << "Shutdown of PXMPeerWorker Successful";
}
void PXMPeerWorkerPrivate::init()
{
    areWeSyncing = false;
    client       = nullptr;
    // Prevent race condition when starting threads, a bufferevent
    // for this (us) is coming soon.
    Peers::PeerData selfComms;
    selfComms.uuid        = localUUID;
    selfComms.hostname    = localHostname;
    selfComms.connectTo   = true;
    selfComms.isAuthed    = false;
    selfComms.progVersion = qApp->applicationVersion();
    peersHash.insert(localUUID, selfComms);

    Peers::PeerData globalPeer;
    globalPeer.uuid      = globalUUID;
    globalPeer.hostname  = "Global Chat";
    globalPeer.connectTo = true;
    globalPeer.isAuthed  = false;
    peersHash.insert(globalUUID, globalPeer);

    syncer = new PXMSync(this, peersHash);
    QObject::connect(syncer, &PXMSync::requestIps, this, &PXMPeerWorkerPrivate::requestSyncPacket);
    QObject::connect(syncer, &PXMSync::syncComplete, this, &PXMPeerWorkerPrivate::donePeerSync);

    in_addr multicast_in_addr;
    multicast_in_addr.s_addr = inet_addr(multicastAddress.toLatin1().constData());
    client                   = new PXMClient(this, multicast_in_addr, localUUID);
    server = new PXMServer::ServerThread(this, localUUID, multicast_in_addr, serverTCPPort, serverUDPPort);

    connectClient();
    startServer();

    this->setupTimers();
    syncTimer->start();
    discoveryTimerSingle->start();
    midnightTimer->start();
}

void PXMPeerWorkerPrivate::setInternalBufferevent(bufferevent* bev)
{
    internalBev = bev;
}
void PXMPeerWorkerPrivate::setupTimers()
{
    srand(static_cast<unsigned int>(time(NULL)));

    syncTimer = new QTimer(this);
    syncTimer->setInterval((rand() % 300000) + SYNC_TIMER_MSECS);
    QObject::connect(syncTimer, &QTimer::timeout, q_ptr, &PXMPeerWorker::beginPeerSync);

    nextSyncTimer = new QTimer(this);
    nextSyncTimer->setInterval(2000);
    QObject::connect(nextSyncTimer, &QTimer::timeout, syncer, &PXMSync::syncNext);

    discoveryTimerSingle = new QTimer(this);
    discoveryTimerSingle->setSingleShot(true);
    discoveryTimerSingle->setInterval(5000);
    QObject::connect(discoveryTimerSingle, &QTimer::timeout, this, &PXMPeerWorkerPrivate::discoveryTimerSingleShot);

    midnightTimer = new QTimer(this);
    midnightTimer->setInterval(MIDNIGHT_TIMER_INTERVAL_MINUTES * 60000);
    QObject::connect(midnightTimer, &QTimer::timeout, this, &PXMPeerWorkerPrivate::midnightTimerPersistent);

    discoveryTimer = new QTimer(this);
}
void PXMPeerWorkerPrivate::startServer()
{
    using namespace PXMServer;
    QObject::connect(server, &ServerThread::authHandler, this, &PXMPeerWorkerPrivate::authHandler,
                     Qt::QueuedConnection);
    QObject::connect(server, &ServerThread::packetHandler, this, &PXMPeerWorkerPrivate::packetHandler,
                     Qt::QueuedConnection);
    QObject::connect(server, &QThread::finished, server, &QObject::deleteLater);
    QObject::connect(server, &ServerThread::newTCPConnection, this, &PXMPeerWorkerPrivate::newAcceptedConnection,
                     Qt::QueuedConnection);
    QObject::connect(server, &ServerThread::peerQuit, this, &PXMPeerWorkerPrivate::peerQuit, Qt::QueuedConnection);
    QObject::connect(server, &ServerThread::attemptConnection, this, &PXMPeerWorkerPrivate::attemptConnection,
                     Qt::QueuedConnection);
    QObject::connect(server, &ServerThread::setPeerHostname, this, &PXMPeerWorkerPrivate::nameHandler,
                     Qt::QueuedConnection);
    QObject::connect(server, &ServerThread::setListenerPorts, this, &PXMPeerWorkerPrivate::setListenerPorts,
                     Qt::QueuedConnection);
    QObject::connect(server, &ServerThread::libeventBackend, this, &PXMPeerWorkerPrivate::setLibeventBackend,
                     Qt::QueuedConnection);
    QObject::connect(server, &ServerThread::setInternalBufferevent, this, &PXMPeerWorkerPrivate::setInternalBufferevent,
                     Qt::QueuedConnection);
    QObject::connect(server, &ServerThread::setSelfCommsBufferevent, this,
                     &PXMPeerWorkerPrivate::setSelfCommsBufferevent, Qt::QueuedConnection);
    QObject::connect(server, &ServerThread::serverSetupFailure, this, &PXMPeerWorkerPrivate::serverSetupFailure,
                     Qt::QueuedConnection);
    QObject::connect(server, &ServerThread::sendUDP, q_ptr, &PXMPeerWorker::sendUDP, Qt::QueuedConnection);
    QObject::connect(server, &ServerThread::resultOfConnectionAttempt, this,
                     &PXMPeerWorkerPrivate::resultOfConnectionAttempt, Qt::QueuedConnection);
    QObject::connect(server, &PXMServer::ServerThread::multicastIsFunctional, this,
                     &PXMPeerWorkerPrivate::multicastIsFunctional, Qt::QueuedConnection);
    server->start();
}
void PXMPeerWorkerPrivate::connectClient()
{
    QObject::connect(client, &PXMClient::resultOfTCPSend, this, &PXMPeerWorkerPrivate::resultOfTCPSend,
                     Qt::QueuedConnection);
    QObject::connect(q_ptr, &PXMPeerWorker::sendMsg, client, &PXMClient::sendMsgSlot, Qt::QueuedConnection);
    QObject::connect(q_ptr, &PXMPeerWorker::sendUDP, client, &PXMClient::sendUDP, Qt::QueuedConnection);
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
void PXMPeerWorkerPrivate::packetHandler(const QSharedPointer<unsigned char> packet,
                                         const size_t len,
                                         const PXMConsts::MESSAGE_TYPE type,
                                         const QUuid uuid,
                                         const bufferevent* bev)
{
    using namespace PXMConsts;
    switch (type) {
        case MSG_TEXT: {
            qDebug().noquote() << "Message from" << uuid.toString();
            QByteArray message = QByteArray::fromRawData((char*)packet.data(), len);
            message.detach();
            qDebug().noquote() << "MSG :" << message;
            msgHandler(message, uuid, bev, false);
            break;
        }
        case MSG_SYNC:
            qDebug().noquote() << "SYNC received from" << uuid.toString();
            syncHandler(packet, len, uuid);
            break;
        case MSG_SYNC_REQUEST:
            qDebug().noquote() << "SYNC_REQUEST received" << QString::fromUtf8((char*)packet.data(), len) << "from"
                               << uuid.toString();
            syncRequestHandlerBev(bev, uuid);
            break;
        case MSG_GLOBAL: {
            qDebug().noquote() << "Global message from" << uuid.toString();
            QByteArray message = QByteArray::fromRawData((char*)packet.data(), len);
            message.detach();
            qDebug().noquote() << "GLOBAL :" << message;
            msgHandler(message, uuid, bev, true);
            break;
        }
        case MSG_NAME:
            qDebug().noquote() << "NAME :" << QString::fromUtf8((char*)packet.data(), len) << "from" << uuid.toString();
            nameHandler(QString::fromUtf8((char*)packet.data(), len), uuid);
            break;
        case MSG_AUTH:
            qWarning().noquote() << "AUTH packet recieved after already "
                                    "authenticated, disregarding...";
            break;
        default:
            qWarning().noquote() << "Bad message type in the packet, "
                                    "discarding the rest";
            break;
    }
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
        peersHash[uuid].hostname = hname.left(PXMConsts::MAX_HOSTNAME_LENGTH + PXMConsts::MAX_COMPUTER_NAME_LENGTH);
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
        if (client && index != 0) {
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
        if (levelOfSuccess != 0) {
            // formatMessage(msg, localUUID, Peers::selfColor);
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
int PXMPeerWorkerPrivate::msgHandler(QByteArray msg, QUuid uuid, const bufferevent* bev, bool global)
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

    QSharedPointer<QString> msgStr = QSharedPointer<QString>(new QString(msg));
    /*
    if (global) {
        if (uuid == localUUID) {
            formatMessage(*msgStr.data(), uuid, Peers::selfColor);
        } else {
            formatMessage(*msgStr.data(), uuid, peersHash.value(uuid).textColor);
        }

        uuid = globalUUID;
    } else {
        formatMessage(*msgStr.data(), uuid, Peers::peerColor);
    }
    */

    // GLOBAL IS NOT WORKING, need to add something as uuid has to be from sender
    addMessageToPeer(msgStr, uuid, true, true, global);
    return 0;
}
void PXMPeerWorkerPrivate::addMessageToAllPeers(QString str, bool alert, bool formatAsMessage)
{
    for (Peers::PeerData& itr : peersHash) {
        q_ptr->addMessageToPeer(str, itr.uuid, alert, formatAsMessage);
    }
}

/*
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
*/

int PXMPeerWorker::addMessageToPeer(QString str, QUuid uuid, bool alert, bool fromServer)
{
    if (!d_ptr->peersHash.contains(uuid)) {
        return -1;
    }

    QSharedPointer<QString> pStr(new QString(str.toUtf8()));
    emit msgRecieved(pStr, d_ptr->peersHash.value(uuid).hostname, uuid, alert, fromServer, false);
    return 0;
}
int PXMPeerWorkerPrivate::addMessageToPeer(QSharedPointer<QString> str, QUuid uuid, bool alert, bool, bool global)
{
    if (!peersHash.contains(uuid)) {
        return -1;
    }

    emit q_ptr->msgRecieved(str, peersHash.value(uuid).hostname, uuid, alert, false, global);
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
        (400 + (PXMConsts::DEBUG_PADDING * 16) + (d_ptr->peersHash.size() * (260 + (9 * PXMConsts::DEBUG_PADDING)))));

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
