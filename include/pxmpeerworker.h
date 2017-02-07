#ifndef PXMPEERWORKER_H
#define PXMPEERWORKER_H

#include <QObject>
#include <QScopedPointer>

#include <event2/bufferevent.h>

#include "pxmpeers.h"
#include "pxmconsts.h"

class PXMPeerWorkerPrivate;

class PXMPeerWorker : public QObject
{
    Q_OBJECT
    QScopedPointer<PXMPeerWorkerPrivate> d_ptr;

   public:
    explicit PXMPeerWorker(QObject* parent,
                           QString username,
                           QUuid selfUUID,
                           QString multicast,
                           unsigned short tcpPort,
                           unsigned short udpPort,
                           QUuid globaluuid);
    ~PXMPeerWorker();
    PXMPeerWorker(PXMPeerWorker const&) = delete;
    PXMPeerWorker& operator=(PXMPeerWorker const&) = delete;
    PXMPeerWorker& operator=(PXMPeerWorker&&) noexcept = delete;
    PXMPeerWorker(PXMPeerWorker&&) noexcept            = delete;
    const int SYNC_TIMEOUT_MSECS                = 2000;
    const int SYNC_TIMER                        = 900000;
   public slots:
    void setListenerPorts(unsigned short tcpport, unsigned short udpport);
    void syncPacketIterator(QSharedPointer<unsigned char> syncPacket, size_t len, QUuid senderUuid);
    void attemptConnection(struct sockaddr_in addr, QUuid uuid);
    void authenticationReceived(QString hname,
                                unsigned short port,
                                QString version,
                                evutil_socket_t s,
                                QUuid uuid,
                                bufferevent* bev);
    void newIncomingConnection(bufferevent* bev);
    void peerQuit(evutil_socket_t s, bufferevent* bev);
    void peerNameChange(QString hname, QUuid uuid);
    void sendSyncPacket(QSharedPointer<Peers::BevWrapper> bw, QUuid uuid);
    void sendSyncPacketBev(const bufferevent *bev, QUuid uuid);
    void resultOfConnectionAttempt(evutil_socket_t socket, bool result,
                                   bufferevent* bev, QUuid uuid);
    void resultOfTCPSend(int levelOfSuccess, QUuid uuid, QString msg,
                         bool print, QSharedPointer<Peers::BevWrapper>);
    void currentThreadInit();
    int addMessageToPeer(QString str, QUuid uuid, bool alert, bool);
    void printInfoToDebug();
    void setlibeventBackend(QString str);
    int recieveServerMessage(QString str, QUuid uuid, const bufferevent* bev,
                             bool global);
    void addMessageToAllPeers(QString str, bool alert, bool formatAsMessage);
    void printFullHistory(QUuid uuid);
    void sendMsgAccessor(QByteArray msg, PXMConsts::MESSAGE_TYPE type,
                         QUuid uuid = QUuid());
    void setSelfCommsBufferevent(bufferevent* bev);
    void multicastIsFunctional();
    void serverSetupFailure(QString error);
    void setLocalHostname(QString);
    void sendUDPAccessor(const char* msg);
    void setInternalBufferevent(bufferevent* bev);
    void beginSync();

    // void restartServer();
   private slots:
    void doneSync();
    void requestSyncPacket(QSharedPointer<Peers::BevWrapper> bw, QUuid uuid);
    void discoveryTimerSingleShot();
    void midnightTimerPersistent();
    void discoveryTimerPersistent();
   signals:
    void printToTextBrowser(QSharedPointer<QString>, QUuid, bool);
    void updateListWidget(QUuid, QString);
    void sendMsg(QSharedPointer<Peers::BevWrapper>, QByteArray,
                 PXMConsts::MESSAGE_TYPE, QUuid = QUuid());
    void sendUDP(const char*, unsigned short);
    void sendIpsPacket(QSharedPointer<Peers::BevWrapper>, QSharedPointer<unsigned char>, size_t len,
                       PXMConsts::MESSAGE_TYPE, QUuid = QUuid());
    //void connectToPeer(evutil_socket_t, struct sockaddr_in,
    //                   QSharedPointer<Peers::BevWrapper>);
    void updateMessServFDS(evutil_socket_t);
    void setItalicsOnItem(QUuid, bool);
    void ipsReceivedFrom(QUuid);
    void warnBox(QString, QString);
    void fullHistory(QLinkedList<QString*>, QUuid);
};

#endif
