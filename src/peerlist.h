#ifndef PEERLIST_H
#define PEERLIST_H
#include <QString>
#include <QObject>
#include <QDebug>
#include <QMutex>
#include <QUuid>

#include <sys/unistd.h>
#include <unordered_map>

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
    bool socketisValid = false;
    bool messagePending = false;
    int socketDescriptor = 0;
    QString portNumber = "-1";
    int listWidgetIndex = -1;
    QString ipAddress = "";
    QString hostname = "";
    QString textBox = "";
    QUuid identifier;
};

class PeerWorkerClass : public QObject
{
    Q_OBJECT
public:
    explicit PeerWorkerClass(QObject *parent, QString hostname, QString uuid, QString port);
    QHash<QUuid,peerDetails>peerDetailsHash;
    void setLocalHostName(QString name);
public slots:
    void hostnameCheck(QString comp);
    void attemptConnection(QString portNumber, QString ipaddr);
    void updatePeerDetailsHash(QString hname, QString ipaddr, QString port, int s, QUuid uuid);
    void newTcpConnection(int s, QString ipaddr);
    void peerQuit(QUuid uuid);
    void peerQuit(int s);
    void setPeerHostname(QString hname, QUuid uuid);
    void sendIps(int i);
    void resultOfConnectionAttempt(int socket, bool result);
    void resultOfTCPSend(int levelOfSuccess, QString uuidString, QString msg, bool print);
private:
    Q_DISABLE_COPY(PeerWorkerClass)
    QString localHostname;
    QString ourListenerPort;
    QString localUUID;
    void sendIdentityMsg(int s);
signals:
    void printToTextBrowser(QString, QUuid, bool);
    void updateListWidget(QUuid);
    void sendMsg(int, QString, QString, QString, QUuid, QString);
    void connectToPeer(int, QString, QString);
    void updateMessServFDS(int);
    void setItalicsOnItem(QUuid, bool);
};

#endif // PEERLIST_H
