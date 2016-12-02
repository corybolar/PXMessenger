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
#endif

class PXMPeerWorker : public QObject
{
    Q_OBJECT
public:
    explicit PXMPeerWorker(QObject *parent, QString hostname, QString uuid, PXMServer *server);
    QHash<QUuid,peerDetails>peerDetailsHash;
    void setLocalHostName(QString name);
    ~PXMPeerWorker();
    size_t packUuid(char *buf, QUuid *uuid);
    QVector<bufferevent*> extraBufferevents;
public slots:
    void setListenerPort(unsigned short port);
    void hostnameCheck(char *ipHeapArray, size_t len, QUuid senderUuid);
    void attemptConnection(sockaddr_in addr, QUuid uuid);
    void authenticationReceived(QString hname, unsigned short port, evutil_socket_t s, QUuid uuid, bufferevent *bev);
    void newTcpConnection(bufferevent *bev);
    void peerQuit(evutil_socket_t s, bufferevent *bev);
    void setPeerHostname(QString hname, QUuid uuid);
    void sendIps(bufferevent *bev, QUuid uuid);
    void resultOfConnectionAttempt(evutil_socket_t socket, bool result, bufferevent *bev);
    void resultOfTCPSend(int levelOfSuccess, QUuid uuid, QString msg, bool print);
private:
    Q_DISABLE_COPY(PXMPeerWorker)
    QString localHostname;
    QString ourListenerPort;
    QUuid localUUID;
    QUuid waitingOnIpsFrom;
    QTimer *syncTimer;
    QTimer *nextSyncTimer;
    bool areWeSyncing;
    PXMServer *realServer;
    PXMSync *syncer;
    void sendIdentityMsg(bufferevent *bev);
signals:
    void printToTextBrowser(QString, QUuid, bool, bool);
    void updateListWidget(QUuid);
    void sendMsg(bufferevent*, QByteArray, QByteArray, QUuid, QUuid);
    void sendIpsPacket(bufferevent*, char*, size_t len, QByteArray, QUuid, QUuid);
    void connectToPeer(evutil_socket_t, sockaddr_in, void*);
    void updateMessServFDS(evutil_socket_t);
    void setItalicsOnItem(QUuid, bool);
    void ipsReceivedFrom(QUuid);
private slots:
    void beginSync();
    void doneSync();
    void requestIps(bufferevent *bev, QUuid uuid);
};

#endif
