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
    explicit PXMPeerWorker(QObject *parent, QString hostname, QUuid uuid, QString multicast, PXMServer *server, QUuid globaluuid);
    ~PXMPeerWorker();
    QVector<bufferevent*> extraBufferevents;
private:
    Q_DISABLE_COPY(PXMPeerWorker)
    QHash<QUuid,peerDetails>peerDetailsHash;
    QString localHostname;
    QString ourListenerPort;
    QString ourUDPListenerPort;
    QString libeventBackend;
    QString multicastAddress;
    QUuid localUUID;
    QUuid waitingOnSyncFrom;
    QUuid globalUUID;
    QTimer *syncTimer;
    QTimer *nextSyncTimer;
    bool areWeSyncing;
    PXMServer *realServer;
    PXMSync *syncer;
    QVector<BevWrapper*> bwShortLife;
    void sendIdentityMsg(BevWrapper *bw);
public slots:
    void setListenerPorts(unsigned short tcpport, unsigned short udpport);
    void syncPacketIterator(char *ipHeapArray, size_t len, QUuid senderUuid);
    void attemptConnection(sockaddr_in addr, QUuid uuid);
    void authenticationReceived(QString hname, unsigned short port, evutil_socket_t s, QUuid uuid, bufferevent *bev);
    void newTcpConnection(bufferevent *bev);
    void peerQuit(evutil_socket_t s, bufferevent *bev);
    void setPeerHostname(QString hname, QUuid uuid);
    void sendIps(BevWrapper *bw, QUuid uuid);
    void sendIpsBev(bufferevent *bev, QUuid uuid);
    void resultOfConnectionAttempt(evutil_socket_t socket, bool result, bufferevent *bev);
    void resultOfTCPSend(int levelOfSuccess, QUuid uuid, QString msg, bool print, BevWrapper *bw);
    void currentThreadInit();
    int addMessageToPeer(QString str, QUuid uuid, bool alert, bool formatAsMessage);
    void printInfoToDebug();
    void setlibeventBackend(QString str);
    int recieveServerMessage(QString str, QUuid uuid, bufferevent *bev, bool global);
    void addMessageToAllPeers(QString str, bool alert, bool formatAsMessage);
    void printFullHistory(QUuid uuid);
    void sendMsgAccessor(QByteArray msg, QByteArray type, QUuid uuid1, QUuid uuid2);
    void setSelfCommsBufferevent(bufferevent *bev);
private slots:
    void beginSync();
    void doneSync();
    void requestIps(BevWrapper *bw, QUuid uuid);
signals:
    void printToTextBrowser(QString, QUuid, bool);
    void updateListWidget(QUuid, QString);
    void sendMsg(BevWrapper*, QByteArray, QByteArray, QUuid, QUuid);
    void sendIpsPacket(BevWrapper*, char*, size_t len, QByteArray, QUuid, QUuid);
    void connectToPeer(evutil_socket_t, sockaddr_in, void*);
    void updateMessServFDS(evutil_socket_t);
    void setItalicsOnItem(QUuid, bool);
    void ipsReceivedFrom(QUuid);

};

#endif
