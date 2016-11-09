#ifndef PEERLIST_H
#define PEERLIST_H
#include <QString>
#include <QObject>
#include <QDebug>
#include <QMutex>
#include <QUuid>
#include <QThread>

#include <sys/unistd.h>
#include <event2/bufferevent.h>
#include <mess_serv.h>

#ifdef __unix__
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <arpa/nameser.h>
#include <netinet/in.h>
#include <resolv.h>
#endif

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

struct peerDetails{
    bool isValid = false;
    bool isConnected = false;
    bool attemptingToConnect = false;
    bool messagePending = false;
    evutil_socket_t socketDescriptor = 0;
    QString portNumber = "-1";
    int listWidgetIndex = -1;
    QString ipAddress = "";
    QString hostname = "";
    QString textBox = "";
    QUuid identifier;
    bufferevent *bev = NULL;
};

class PeerWorkerClass : public QObject
{
    Q_OBJECT
public:
    explicit PeerWorkerClass(QObject *parent, QString hostname, QString uuid, MessengerServer *server);
    QHash<QUuid,peerDetails>peerDetailsHash;
    void setLocalHostName(QString name);
public slots:
    void setListenerPort(unsigned short port);
    void hostnameCheck(QString comp);
    void attemptConnection(QString portNumber, QString ipaddr, QString uuid);
    void updatePeerDetailsHash(QString hname, QString port, evutil_socket_t s, QUuid uuid, void *bevptr);
    void newTcpConnection(evutil_socket_t s, void *bev);
    void peerQuit(evutil_socket_t s);
    void setPeerHostname(QString hname, QUuid uuid);
    void sendIps(evutil_socket_t i);
    void resultOfConnectionAttempt(evutil_socket_t socket, bool result);
    void resultOfTCPSend(int levelOfSuccess, QString uuidString, QString msg, bool print);
private:
    Q_DISABLE_COPY(PeerWorkerClass)
    QString localHostname;
    QString ourListenerPort;
    QString localUUID;
    MessengerServer *realServer;
    void sendIdentityMsg(evutil_socket_t s);
signals:
    void printToTextBrowser(QString, QUuid, bool);
    void updateListWidget(QUuid);
    void sendMsg(evutil_socket_t, QString, QString, QUuid, QString);
    void connectToPeer(evutil_socket_t, QString, QString);
    void updateMessServFDS(evutil_socket_t);
    void setItalicsOnItem(QUuid, bool);
};

#endif // PEERLIST_H
