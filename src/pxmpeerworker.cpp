#include <QApplication>
#include <QDebug>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QStringBuilder>
#include <QThread>
#include <QTimer>
#include <pxmpeerworker.h>

#include "pxmclient.h"
#include "pxmserver.h"
#include "pxmsync.h"
#include "timedvector.h"

#include <event2/event.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <arpa/nameser.h>
#include <netdb.h>
#include <netinet/in.h>
#include <resolv.h>
#include <sys/socket.h>
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
          syncablePeers(new TimedVector<QUuid>(PXMPeerWorker::SYNC_TIMEOUT_MSECS, SECONDS)),
          serverTCPPort(tcpPort),
          serverUDPPort(udpPort)
    {
    }
    PXMPeerWorker* const q_ptr;
    // Data Members

    QHash<QUuid, Peers::PeerData> peerDetailsHash;
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
    bufferevent* closeBev;
    QVector<QSharedPointer<Peers::BevWrapper>> extraBufferevents;
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
    // Init
    // Windows socket init

    d_ptr->areWeSyncing = false;
    // d_ptr->syncablePeers = new TimedVector<QUuid>(SYNC_TIMEOUT_MSECS, SECONDS);
    d_ptr->messClient = nullptr;
    // End of Init

    // Prevent race condition when starting threads, a bufferevent
    // for this (us) is coming soon.
    Peers::PeerData selfComms;
    selfComms.identifier      = d_ptr->localUUID;
    selfComms.hostname        = d_ptr->localHostname;
    selfComms.connectTo       = true;
    selfComms.isAuthenticated = false;
    selfComms.progVersion     = qApp->applicationVersion();
    d_ptr->peerDetailsHash.insert(d_ptr->localUUID, selfComms);

    Peers::PeerData globalPeer;
    globalPeer.identifier      = d_ptr->globalUUID;
    globalPeer.hostname        = "Global Chat";
    globalPeer.connectTo       = true;
    globalPeer.isAuthenticated = false;
    d_ptr->peerDetailsHash.insert(d_ptr->globalUUID, globalPeer);
}
PXMPeerWorker::~PXMPeerWorker()
{
    using namespace PXMServer;
    d_ptr->syncTimer->stop();
    d_ptr->nextSyncTimer->stop();
    // delete d_ptr->syncablePeers;

    for (auto& itr : d_ptr->peerDetailsHash) {
        // qDeleteAll(itr.messages);
        evutil_closesocket(itr.socket);
    }
    // This must be done before PXMServer is shutdown;
    d_ptr->peerDetailsHash.clear();

    if (d_ptr->messServer != 0 && d_ptr->messServer->isRunning()) {
        INTERNAL_MSG exit = INTERNAL_MSG::EXIT;
        bufferevent_write(d_ptr->closeBev, &exit, sizeof(INTERNAL_MSG));
        d_ptr->messServer->wait(5000);
    }

    qDebug() << "Shutdown of PXMPeerWorker Successful";
}
void PXMPeerWorker::setCloseBufferevent(bufferevent* bev)
{
    d_ptr->closeBev = bev;
}
void PXMPeerWorker::currentThreadInit()
{
    d_ptr->syncer = new PXMSync(this, d_ptr->peerDetailsHash);
    QObject::connect(d_ptr->syncer, &PXMSync::requestIps, this, &PXMPeerWorker::requestSyncPacket);
    QObject::connect(d_ptr->syncer, &PXMSync::syncComplete, this, &PXMPeerWorker::doneSync);

    srand(static_cast<unsigned int>(time(NULL)));

    d_ptr->syncTimer = new QTimer(this);
    d_ptr->syncTimer->setInterval((rand() % 300000) + SYNC_TIMER);
    QObject::connect(d_ptr->syncTimer, &QTimer::timeout, this, &PXMPeerWorker::beginSync);
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
    d_ptr->messServer =
        new PXMServer::ServerThread(this, d_ptr->serverTCPPort, d_ptr->serverUDPPort, multicast_in_addr);

    d_ptr->connectClient();
    d_ptr->startServer();
}
void PXMPeerWorkerPrivate::startServer()
{
    messServer->setLocalUUID(localUUID);
    QObject::connect(messServer, &PXMServer::ServerThread::authenticationReceived, q_ptr,
                     &PXMPeerWorker::authenticationReceived, Qt::QueuedConnection);
    QObject::connect(messServer, &PXMServer::ServerThread::messageRecieved, q_ptr, &PXMPeerWorker::recieveServerMessage,
                     Qt::QueuedConnection);
    QObject::connect(messServer, &QThread::finished, messServer, &QObject::deleteLater);
    QObject::connect(messServer, &PXMServer::ServerThread::newTCPConnection, q_ptr,
                     &PXMPeerWorker::newIncomingConnection, Qt::QueuedConnection);
    QObject::connect(messServer, &PXMServer::ServerThread::peerQuit, q_ptr, &PXMPeerWorker::peerQuit,
                     Qt::QueuedConnection);
    QObject::connect(messServer, &PXMServer::ServerThread::attemptConnection, q_ptr, &PXMPeerWorker::attemptConnection,
                     Qt::QueuedConnection);
    QObject::connect(messServer, &PXMServer::ServerThread::sendSyncPacket, q_ptr, &PXMPeerWorker::sendSyncPacketBev,
                     Qt::QueuedConnection);
    QObject::connect(messServer, &PXMServer::ServerThread::syncPacketIterator, q_ptr,
                     &PXMPeerWorker::syncPacketIterator, Qt::QueuedConnection);
    QObject::connect(messServer, &PXMServer::ServerThread::setPeerHostname, q_ptr, &PXMPeerWorker::peerNameChange,
                     Qt::QueuedConnection);
    QObject::connect(messServer, &PXMServer::ServerThread::setListenerPorts, q_ptr, &PXMPeerWorker::setListenerPorts,
                     Qt::QueuedConnection);
    QObject::connect(messServer, &PXMServer::ServerThread::libeventBackend, q_ptr, &PXMPeerWorker::setlibeventBackend,
                     Qt::QueuedConnection);
    QObject::connect(messServer, &PXMServer::ServerThread::setCloseBufferevent, q_ptr,
                     &PXMPeerWorker::setCloseBufferevent, Qt::QueuedConnection);
    QObject::connect(messServer, &PXMServer::ServerThread::setSelfCommsBufferevent, q_ptr,
                     &PXMPeerWorker::setSelfCommsBufferevent, Qt::QueuedConnection);
    QObject::connect(messServer, &PXMServer::ServerThread::multicastIsFunctional, q_ptr,
                     &PXMPeerWorker::multicastIsFunctional, Qt::QueuedConnection);
    QObject::connect(messServer, &PXMServer::ServerThread::serverSetupFailure, q_ptr,
                     &PXMPeerWorker::serverSetupFailure, Qt::QueuedConnection);
    QObject::connect(messServer, &PXMServer::ServerThread::sendUDP, q_ptr, &PXMPeerWorker::sendUDP,
                     Qt::QueuedConnection);
    QObject::connect(messServer, &PXMServer::ServerThread::nameChange, q_ptr, &PXMPeerWorker::peerNameChange,
                     Qt::QueuedConnection);
    messServer->start();
}
void PXMPeerWorkerPrivate::connectClient()
{
    QObject::connect(messClient, &PXMClient::resultOfConnectionAttempt, q_ptr,
                     &PXMPeerWorker::resultOfConnectionAttempt, Qt::QueuedConnection);
    QObject::connect(messClient, &PXMClient::resultOfTCPSend, q_ptr, &PXMPeerWorker::resultOfTCPSend,
                     Qt::QueuedConnection);
    QObject::connect(q_ptr, &PXMPeerWorker::sendMsg, messClient, &PXMClient::sendMsgSlot, Qt::QueuedConnection);
    QObject::connect(q_ptr, &PXMPeerWorker::connectToPeer, messClient, &PXMClient::connectToPeer, Qt::QueuedConnection);
    QObject::connect(q_ptr, &PXMPeerWorker::sendIpsPacket, messClient, &PXMClient::sendIpsSlot, Qt::QueuedConnection);
    QObject::connect(q_ptr, &PXMPeerWorker::sendUDP, messClient, &PXMClient::sendUDP, Qt::QueuedConnection);
}

void PXMPeerWorker::setListenerPorts(unsigned short tcpport, unsigned short udpport)
{
    d_ptr->serverTCPPort = tcpport;
    d_ptr->serverUDPPort = udpport;
}
void PXMPeerWorker::doneSync()
{
    d_ptr->areWeSyncing = false;
    d_ptr->nextSyncTimer->stop();
    qInfo() << "Finished Syncing peers";
}
void PXMPeerWorker::beginSync()
{
    if (d_ptr->areWeSyncing) {
        return;
    }

    qInfo() << "Beginning Sync of connected peers";
    d_ptr->areWeSyncing = true;
    d_ptr->syncer->setsyncHash(d_ptr->peerDetailsHash);
    d_ptr->syncer->setIteratorToStart();
    d_ptr->syncer->syncNext();
    d_ptr->nextSyncTimer->start();
}
void PXMPeerWorker::syncPacketIterator(QSharedPointer<unsigned char> syncPacket, size_t len, QUuid senderUuid)
{
    if (!d_ptr->syncablePeers->contains(senderUuid)) {
        qWarning() << "Sync packet from bad uuid -- timeout or not requested" << senderUuid;
        return;
    }
    qInfo() << "Sync packet from" << senderUuid.toString();

    size_t index = 0;
    while (index + UUIDCompression::PACKED_UUID_LENGTH + 6 <= len) {
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        memcpy(&(addr.sin_addr.s_addr), &syncPacket.data()[index], sizeof(uint32_t));
        index += sizeof(uint32_t);
        memcpy(&(addr.sin_port), &syncPacket.data()[index], sizeof(uint16_t));
        index += sizeof(uint16_t);
        QUuid uuid = UUIDCompression::unpackUUID(reinterpret_cast<unsigned char*>(&syncPacket.data()[index]));
        index += UUIDCompression::PACKED_UUID_LENGTH;

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
void PXMPeerWorker::newIncomingConnection(bufferevent* bev)
{
    QSharedPointer<Peers::BevWrapper> bw(new Peers::BevWrapper);
    bw->setBev(bev);
    d_ptr->extraBufferevents.push_back(bw);
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
    qInfo() << "Requesting ips from" << d_ptr->peerDetailsHash.value(uuid).hostname;
    emit sendMsg(bw, QByteArray(), MSG_SYNC_REQUEST);
}
void PXMPeerWorker::attemptConnection(struct sockaddr_in addr, QUuid uuid)
{
    if (d_ptr->peerDetailsHash.value(uuid).connectTo || uuid.isNull()) {
        return;
    }

    evutil_socket_t socketfd = socket(AF_INET, SOCK_STREAM, 0);
    evutil_make_socket_nonblocking(socketfd);
    if (d_ptr->messServer->base) {
        struct bufferevent* bev = bufferevent_socket_new(d_ptr->messServer->base, socketfd, BEV_OPT_THREADSAFE);
        d_ptr->peerDetailsHash[uuid].bw->lockBev();
        d_ptr->peerDetailsHash[uuid].bw->setBev(bev);
        d_ptr->peerDetailsHash[uuid].bw->unlockBev();
        d_ptr->peerDetailsHash[uuid].identifier   = uuid;
        d_ptr->peerDetailsHash[uuid].socket       = socketfd;
        d_ptr->peerDetailsHash[uuid].connectTo    = true;
        d_ptr->peerDetailsHash[uuid].ipAddressRaw = addr;

        emit connectToPeer(socketfd, addr, d_ptr->peerDetailsHash[uuid].bw);
    } else {
        qCritical() << "Server event_base is not initialized, cannot attempt connections";
        evutil_closesocket(socketfd);
    }
}
void PXMPeerWorker::peerNameChange(QString hname, QUuid uuid)
{
    if (d_ptr->peerDetailsHash.contains(uuid)) {
        d_ptr->peerDetailsHash[uuid].hostname =
            hname.left(PXMConsts::MAX_HOSTNAME_LENGTH + PXMConsts::MAX_COMPUTER_NAME);
        emit updateListWidget(uuid, d_ptr->peerDetailsHash.value(uuid).hostname);
    }
}
void PXMPeerWorker::peerQuit(evutil_socket_t s, bufferevent* bev)
{
    for (Peers::PeerData& itr : d_ptr->peerDetailsHash) {
        if (itr.bw->getBev() == bev) {
            d_ptr->peerDetailsHash[itr.identifier].connectTo       = false;
            d_ptr->peerDetailsHash[itr.identifier].isAuthenticated = false;
            d_ptr->peerDetailsHash[itr.identifier].bw->lockBev();
            qInfo().noquote() << "Peer:" << itr.identifier.toString() << "has disconnected";
            d_ptr->peerDetailsHash[itr.identifier].bw->setBev(nullptr);
            bufferevent_free(bev);
            d_ptr->peerDetailsHash[itr.identifier].bw->unlockBev();
            evutil_closesocket(itr.socket);
            d_ptr->peerDetailsHash[itr.identifier].socket = -1;
            emit setItalicsOnItem(itr.identifier, 1);
            return;
        }
    }
    for (QSharedPointer<Peers::BevWrapper>& itr : d_ptr->extraBufferevents) {
        d_ptr->extraBufferevents.removeAll(itr);
    }
    qInfo().noquote() << "Non-Authed Peer has quit";
    bufferevent_free(bev);
    evutil_closesocket(s);
}
void PXMPeerWorker::sendSyncPacketBev(bufferevent* bev, QUuid uuid)
{
    if (d_ptr->peerDetailsHash.contains(uuid) && d_ptr->peerDetailsHash.value(uuid).bw->getBev() == bev) {
        this->sendSyncPacket(d_ptr->peerDetailsHash.value(uuid).bw, uuid);
    } else {
        qCritical() << "sendIpsBev:error";
    }
}
void PXMPeerWorker::sendSyncPacket(QSharedPointer<Peers::BevWrapper> bw, QUuid uuid)
{
    qInfo() << "Sending ips to" << d_ptr->peerDetailsHash.value(uuid).hostname;
    char* msgRaw = new char[static_cast<size_t>(d_ptr->peerDetailsHash.size()) *
                                (sizeof(uint32_t) + sizeof(uint16_t) + UUIDCompression::PACKED_UUID_LENGTH) +
                            1];
    size_t index = 0;
    for (Peers::PeerData& itr : d_ptr->peerDetailsHash) {
        if (itr.isAuthenticated) {
            memcpy(&msgRaw[index], &(itr.ipAddressRaw.sin_addr.s_addr), sizeof(uint32_t));
            index += sizeof(uint32_t);
            memcpy(&msgRaw[index], &(itr.ipAddressRaw.sin_port), sizeof(uint16_t));
            index += sizeof(uint16_t);
            index += UUIDCompression::packUUID(&msgRaw[index], itr.identifier);
        }
    }
    msgRaw[index + 1] = 0;

    if (d_ptr->messClient) {
        emit sendIpsPacket(bw, msgRaw, index, MSG_SYNC);
    } else {
        qCritical() << "messClient not initialized";
        delete[] msgRaw;
    }
}
void PXMPeerWorker::resultOfConnectionAttempt(evutil_socket_t socket, bool result, bufferevent* bev)
{
    QUuid uuid = QUuid();
    for (Peers::PeerData& itr : d_ptr->peerDetailsHash) {
        if (itr.bw->getBev() == bev) {
            uuid = itr.identifier;
        }
    }
    if (uuid.isNull()) {
        return;
    }

    if (result) {
        qInfo() << "Successful connection attempt to" << uuid.toString();
        bufferevent_setcb(bev, PXMServer::ServerThread::tcpAuth, NULL, PXMServer::ServerThread::tcpError,
                          d_ptr->messServer);
        bufferevent_setwatermark(bev, EV_READ, PXMServer::PACKET_HEADER_LENGTH, PXMServer::PACKET_HEADER_LENGTH);
        bufferevent_enable(bev, EV_READ | EV_WRITE);
        d_ptr->peerDetailsHash[uuid].bw->lockBev();
        if (d_ptr->peerDetailsHash.value(uuid).bw->getBev() != nullptr &&
            d_ptr->peerDetailsHash.value(uuid).bw->getBev() != bev) {
            bufferevent_free(d_ptr->peerDetailsHash.value(uuid).bw->getBev());
            d_ptr->peerDetailsHash.value(uuid).bw->setBev(bev);
        }
        d_ptr->peerDetailsHash.value(uuid).bw->unlockBev();

        d_ptr->sendAuthPacket(d_ptr->peerDetailsHash.value(uuid).bw);
    } else {
        qWarning() << "Unsuccessful connection attempt to " << uuid.toString();
        d_ptr->peerDetailsHash[uuid].bw->lockBev();
        d_ptr->peerDetailsHash[uuid].bw->setBev(nullptr);
        bufferevent_free(bev);
        evutil_closesocket(socket);
        d_ptr->peerDetailsHash[uuid].bw->unlockBev();
        d_ptr->peerDetailsHash[uuid].connectTo       = false;
        d_ptr->peerDetailsHash[uuid].isAuthenticated = false;
        d_ptr->peerDetailsHash[uuid].socket          = -1;
    }
}
void PXMPeerWorker::resultOfTCPSend(int levelOfSuccess,
                                    QUuid uuid,
                                    QString msg,
                                    bool print,
                                    QSharedPointer<Peers::BevWrapper>)
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
void PXMPeerWorker::authenticationReceived(QString hname,
                                           unsigned short port,
                                           QString version,
                                           evutil_socket_t s,
                                           QUuid uuid,
                                           bufferevent* bev)
{
    if (uuid.isNull()) {
        evutil_closesocket(s);
        bufferevent_free(bev);
        return;
    }
    struct sockaddr_in addr;
    socklen_t socklen = sizeof(addr);
    memset(&addr, 0, socklen);

    getpeername(s, reinterpret_cast<struct sockaddr*>(&addr), &socklen);
    addr.sin_port = htons(port);

    qInfo().noquote() << hname << "on port" << QString::number(port) << "authenticated!";

    if (d_ptr->peerDetailsHash[uuid].bw->getBev() != nullptr && d_ptr->peerDetailsHash[uuid].bw->getBev() != bev) {
        peerQuit(-1, d_ptr->peerDetailsHash.value(uuid).bw->getBev());
    } else {
        for (QSharedPointer<Peers::BevWrapper>& itr : d_ptr->extraBufferevents) {
            if (itr->getBev() == bev) {
                d_ptr->peerDetailsHash[uuid].bw = itr;
                d_ptr->extraBufferevents.removeAll(itr);
                break;
            }
        }
    }
    d_ptr->peerDetailsHash[uuid].identifier   = uuid;
    d_ptr->peerDetailsHash[uuid].ipAddressRaw = addr;
    d_ptr->peerDetailsHash[uuid].hostname     = hname;
    d_ptr->peerDetailsHash[uuid].progVersion  = version;
    d_ptr->peerDetailsHash[uuid].bw->lockBev();
    d_ptr->peerDetailsHash[uuid].bw->setBev(bev);
    d_ptr->peerDetailsHash[uuid].bw->unlockBev();
    d_ptr->peerDetailsHash[uuid].socket          = s;
    d_ptr->peerDetailsHash[uuid].connectTo       = true;
    d_ptr->peerDetailsHash[uuid].isAuthenticated = true;

    emit updateListWidget(uuid, d_ptr->peerDetailsHash.value(uuid).hostname);
    emit requestSyncPacket(d_ptr->peerDetailsHash.value(uuid).bw, uuid);
}
int PXMPeerWorker::recieveServerMessage(QString str, QUuid uuid, bufferevent* bev, bool global)
{
    if (uuid != d_ptr->localUUID) {
        if (!(d_ptr->peerDetailsHash.contains(uuid))) {
            qWarning() << "Message from invalid uuid, rejection";
            return -1;
        }

        if (d_ptr->peerDetailsHash.value(uuid).bw->getBev() != bev) {
            bool foundIt      = false;
            QUuid uuidSpoofer = QUuid();
            for (auto& itr : d_ptr->peerDetailsHash) {
                if (itr.bw->getBev() == bev) {
                    foundIt     = true;
                    uuidSpoofer = itr.identifier;
                    continue;
                }
            }
            if (foundIt) {
                addMessageToPeer("This user is trying to spoof another users uuid!", uuidSpoofer, true, false);
            } else {
                addMessageToPeer("Someone is trying to spoof this users uuid!", uuid, true, false);
            }

            return -1;
        }
    }

    if (global) {
        if (uuid == d_ptr->localUUID) {
            d_ptr->formatMessage(str, uuid, Peers::selfColor);
        } else {
            d_ptr->formatMessage(str, uuid, d_ptr->peerDetailsHash.value(uuid).textColor);
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
    for (Peers::PeerData& itr : d_ptr->peerDetailsHash) {
        addMessageToPeer(str, itr.identifier, alert, formatAsMessage);
    }
}

int PXMPeerWorkerPrivate::formatMessage(QString& str, QUuid uuid, QString color)
{
    QRegularExpression qre("(<p.*?>)");
    QRegularExpressionMatch qrem = qre.match(str);
    int offset                   = qrem.capturedEnd(1);
    QDateTime dt                 = QDateTime::currentDateTime();
    QString date                 = QStringLiteral("(") % dt.time().toString("hh:mm:ss") % QStringLiteral(") ");
    str.insert(offset, QString("<span style=\"color: " % color % ";\">" % date % peerDetailsHash.value(uuid).hostname %
                               ":&nbsp;</span>"));

    return 0;
}

int PXMPeerWorker::addMessageToPeer(QString str, QUuid uuid, bool alert, bool)
{
    if (!d_ptr->peerDetailsHash.contains(uuid)) {
        return -1;
    }

    QSharedPointer<QString> pStr(new QString(str.toUtf8()));
    d_ptr->peerDetailsHash[uuid].messages.append(pStr);
    if (d_ptr->peerDetailsHash[uuid].messages.size() > MESSAGE_HISTORY_LENGTH) {
        d_ptr->peerDetailsHash[uuid].messages.takeFirst();
    }
    emit printToTextBrowser(pStr, uuid, alert);
    return 0;
}
void PXMPeerWorker::setSelfCommsBufferevent(bufferevent* bev)
{
    d_ptr->peerDetailsHash.value(d_ptr->localUUID).bw->lockBev();
    d_ptr->peerDetailsHash.value(d_ptr->localUUID).bw->setBev(bev);
    d_ptr->peerDetailsHash.value(d_ptr->localUUID).bw->unlockBev();

    updateListWidget(d_ptr->localUUID, d_ptr->localHostname);
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
                emit sendMsg(d_ptr->peerDetailsHash.value(uuid).bw, msg, MSG_TEXT, uuid);
            else
                qWarning() << "Bad recipient uuid for normal message";
            break;
        case MSG_GLOBAL:
            for (auto& itr : d_ptr->peerDetailsHash) {
                if (itr.isAuthenticated || itr.identifier == d_ptr->localUUID) {
                    emit sendMsg(itr.bw, msg, MSG_GLOBAL);
                }
            }
            break;
        case MSG_NAME:
            d_ptr->peerDetailsHash[d_ptr->localUUID].hostname = QString(msg);
            d_ptr->localHostname                              = QString(msg);
            for (auto& itr : d_ptr->peerDetailsHash) {
                if (itr.isAuthenticated || itr.identifier == d_ptr->localUUID) {
                    emit sendMsg(itr.bw, msg, MSG_NAME);
                }
            }
            break;
        default:
            qWarning() << "Bad message type in sendMsgAccessor";
            break;
    }
}
void PXMPeerWorker::printFullHistory(QUuid uuid)
{
    QString* str = new QString();

    for (QSharedPointer<QString> const& itr : d_ptr->peerDetailsHash.value(uuid).messages) {
        str->append(itr.data());
    }
    QSharedPointer<QString> pStr(str);

    emit printToTextBrowser(pStr, uuid, false);
}

void PXMPeerWorker::setlibeventBackend(QString str)
{
    d_ptr->libeventBackend = str;
}
void PXMPeerWorker::printInfoToDebug()
{
    // hopefully reduce reallocs here
    QString str;
    str.reserve((330 + (DEBUG_PADDING * 16) + (d_ptr->peerDetailsHash.size() * (260 + (9 * DEBUG_PADDING)))));

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
    for (Peers::PeerData& itr : d_ptr->peerDetailsHash) {
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
    if (d_ptr->peerDetailsHash.count() < 3) {
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
    if (d_ptr->peerDetailsHash.count() < 3) {
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
