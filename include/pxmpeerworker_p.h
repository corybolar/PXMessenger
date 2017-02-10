#ifndef PXMPEERWORKERPRIVATE_H
#define PXMPEERWORKERPRIVATE_H
#include <pxmpeerworker.h>
#include <timedvector.h>
#include <pxmserver.h>
#include <pxmsync.h>
#include <pxmclient.h>
#include <QObject>

class PXMPeerWorkerPrivate : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(PXMPeerWorkerPrivate)
    Q_DECLARE_PUBLIC(PXMPeerWorker)

    const unsigned int SYNCREQUEST_TIMEOUT_MSECS = 2000;

   public:
    PXMPeerWorkerPrivate(PXMPeerWorker* q) : QObject(q), q_ptr(q) {}
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

        if (messServer != 0 && messServer->isRunning()) {
            INTERNAL_MSG exit                 = INTERNAL_MSG::EXIT;
            char exitMsg[INTERNAL_MSG_LENGTH] = {};
            memcpy(&exitMsg[0], &exit, sizeof(exit));
            bufferevent_write(internalBev, &exitMsg[0], INTERNAL_MSG_LENGTH);
            messServer->wait(5000);
        }
    }
    void testConnect() {}
   private:
    PXMPeerWorker* const q_ptr;
    // Data Members

    const size_t MIDNIGHT_TIMER_INTERVAL_MINUTES = 1;
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
    int formatMessage(QString& str, QUuid uuid, QString color);
    QByteArray createSyncPacket(size_t& index);
    void startServer();
    void connectClient();
    // Slots
   private slots:
    int msgHandler(QSharedPointer<QString> msgPtr, QUuid uuid, const bufferevent* bev, bool global);
    void authHandler(const QString hostname,
                     const unsigned short port,
                     const QString version,
                     const int socket,
                     const QUuid uuid,
                     bufferevent* bev);
    void syncHandler(QSharedPointer<unsigned char> syncPacket, size_t len, QUuid senderUuid);
    void nameHandler(QString hname, const QUuid uuid);
    void syncRequestHandlerBev(const bufferevent* bev, const QUuid uuid);

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

    void multicastIsFunctional();
};
#endif // PXMPEERWORKERPRIVATE_H
