/** @file pxmpeerworker.h
 * @brief public header for pxmpeerworker.cpp
 *
 * This is the manager for pxmserver and pxmclient.  Meant to be used in it own
 * thread but will spawn another thread for the server to work from.
 */

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

    friend class PXMPeerWorkerPrivate;
    const QScopedPointer<PXMPeerWorkerPrivate> d_ptr;
public:
    explicit PXMPeerWorker(QObject* parent,
                           QString username,
                           QUuid selfUUID,
                           QString multicast,
                           unsigned short tcpPort,
                           unsigned short udpPort,
                           QUuid globaluuid);
    ~PXMPeerWorker();
    PXMPeerWorker(PXMPeerWorker const&) 	       = delete;
    PXMPeerWorker& operator=(PXMPeerWorker const&)     = delete;
    PXMPeerWorker& operator=(PXMPeerWorker&&) noexcept = delete;
    PXMPeerWorker(PXMPeerWorker&&) noexcept            = delete;
   public slots:
    int addMessageToPeer(QString str, QUuid uuid, bool alert, bool fromServer);

    void sendMsgAccessor(QByteArray msg, PXMConsts::MESSAGE_TYPE type,
                         QUuid uuid = QUuid());
    void sendUDPAccessor(const char* msg);

    //void setupTimers();
    void printInfoToDebug();
    void beginPeerSync();
    // void restartServer();
   signals:
    void msgRecieved(QSharedPointer<QString>, QString, QUuid, bool, bool);
    void newAuthedPeer(QUuid, QString);
    void sendMsg(QSharedPointer<Peers::BevWrapper>, QByteArray, size_t,
                 PXMConsts::MESSAGE_TYPE, QUuid = QUuid());
    void sendUDP(const char*, unsigned short);
    void connectionStatusChange(QUuid, bool);
    void ipsReceivedFrom(QUuid);
    void warnBox(QString, QString);
};

#endif
