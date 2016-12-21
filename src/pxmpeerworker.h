#ifndef PXMPEERWORKER_H
#define PXMPEERWORKER_H

#include <QString>
#include <QDebug>
#include <QMutex>
#include <QUuid>
#include <QThread>
#include <QTimer>
#include <QStringBuilder>
#include <QBuffer>
#include <QApplication>
#include <QDateTime>

#include <sys/unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <event2/bufferevent.h>

#include "pxmserver.h"
#include "pxmsync.h"
#include "pxmdefinitions.h"
#include "pxmclient.h"
#include "timedvector.h"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <arpa/nameser.h>
#include <netinet/in.h>
#include <resolv.h>
#include <QSharedPointer>
#endif

class PXMPeerWorker : public QObject
{
    Q_OBJECT
public:
    explicit PXMPeerWorker(QObject *parent, initialSettings presets,
                           QUuid globaluuid);
    ~PXMPeerWorker();
    PXMPeerWorker(PXMPeerWorker const&) = delete;
    PXMPeerWorker& operator=(PXMPeerWorker const&) = delete;
    PXMPeerWorker& operator=(PXMPeerWorker&&) noexcept = delete;
    PXMPeerWorker(PXMPeerWorker&&) noexcept = delete;
private:
    QHash<QUuid,peerDetails>peerDetailsHash;
    QString localHostname;
    QString ourListenerPort;
    QString ourUDPListenerPort;
    QString libeventBackend;
    QString multicastAddress;
    QUuid localUUID;
    QUuid globalUUID;
    QTimer *syncTimer;
    QTimer *nextSyncTimer;
    QTimer *discoveryTimer;
    QTimer *midnightTimer;
    bool areWeSyncing;
    bool multicastIsFunctioning;
    PXMSync *syncer;
    QVector<BevWrapper*> bwShortLife;
    PXMServer *messServer;
    PXMClient *messClient;
    bufferevent *closeBev;
    QVector<bufferevent*> extraBufferevents;
    TimedVector<QUuid> *syncablePeers;

    void sendAuthPacket(BevWrapper *bw);
    void startServerThread();
    void startClient();
public slots:
    void setListenerPorts(unsigned short tcpport, unsigned short udpport);
    void syncPacketIterator(char *ipHeapArray, size_t len, QUuid senderUuid);
    void attemptConnection(sockaddr_in addr, QUuid uuid);
    void authenticationReceived(QString hname, unsigned short port,
                                evutil_socket_t s, QUuid uuid,
                                bufferevent *bev);
    void newTcpConnection(bufferevent *bev);
    void peerQuit(evutil_socket_t s, bufferevent *bev);
    void setPeerHostname(QString hname, QUuid uuid);
    void sendSyncPacket(BevWrapper *bw, QUuid uuid);
    void sendSyncPacketBev(bufferevent *bev, QUuid uuid);
    void resultOfConnectionAttempt(evutil_socket_t socket, bool result,
                                   bufferevent *bev);
    void resultOfTCPSend(int levelOfSuccess, QUuid uuid, QString msg,
                         bool print, BevWrapper *bw);
    void currentThreadInit();
    int addMessageToPeer(QString str, QUuid uuid, bool alert,
                         bool formatAsMessage);
    void printInfoToDebug();
    void setlibeventBackend(QString str);
    int recieveServerMessage(QString str, QUuid uuid, bufferevent *bev,
                             bool global);
    void addMessageToAllPeers(QString str, bool alert, bool formatAsMessage);
    void printFullHistory(QUuid uuid);
    void sendMsgAccessor(QByteArray msg, PXMConsts::MESSAGE_TYPE type,
                         QUuid uuid1, QUuid uuid2);
    void setSelfCommsBufferevent(bufferevent *bev);
    void discoveryTimerPersistent();
    void multicastIsFunctional();
    void serverSetupFailure();
private slots:
    void beginSync();
    void doneSync();
    void requestSyncPacket(BevWrapper *bw, QUuid uuid);
    void discoveryTimerSingleShot();
    void midnightTimerPersistent();
    void setCloseBufferevent(bufferevent *bev);
signals:
    void printToTextBrowser(QString, QUuid, bool);
    void updateListWidget(QUuid, QString);
    void sendMsg(BevWrapper*, QByteArray, PXMConsts::MESSAGE_TYPE,
                 QUuid, QUuid);
    void sendUDP(const char*, unsigned short);
    void sendIpsPacket(BevWrapper*, char*, size_t len,
                       PXMConsts::MESSAGE_TYPE, QUuid, QUuid);
    void connectToPeer(evutil_socket_t, sockaddr_in, bufferevent*);
    void updateMessServFDS(evutil_socket_t);
    void setItalicsOnItem(QUuid, bool);
    void ipsReceivedFrom(QUuid);
    void warnBox(QString, QString);
    void fullHistory(QLinkedList<QString*>, QUuid);
};

#endif
