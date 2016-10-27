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
    int listWidgetIndex = 0;
    //char ipAddress[INET6_ADDRSTRLEN] = {};
    QString ipAddress = "";
    QString hostname = "";
    QString textBox = "";
    QUuid identifier;
};

class PeerWorkerClass : public QObject
{
    Q_OBJECT
public:
    explicit PeerWorkerClass(QObject *parent);
    //peerDetails knownPeersArray[255];
    QHash<QUuid,peerDetails>peerDetailsHash;
    void setLocalHostName(QString name);
    //int numberOfValidPeers = 0;
public slots:
    void hostnameCheck(QString comp);
    void listpeers(QString hname, QString ipaddr);
    void listpeers(QString hname, QString ipaddr, bool test, int s, QUuid uuid);
    void newTcpConnection(int s, QString ipaddr, QUuid uuid);
    void peerQuit(QUuid uuid);
    void peerQuit(int s);
    void setPeerHostname(QString hname, QUuid uuid);
    void sendIps(int i);
    void resultOfConnectionAttempt(int socket, bool result);
    void resultOfTCPSend(int levelOfSuccess, QString uuidString, QString msg, bool print);
private:
    Q_DISABLE_COPY(PeerWorkerClass)
    void assignSocket(peerDetails *p);

    QString localHostname;
signals:
    void printToTextBrowser(QString, QUuid, bool);
    void updateListWidget(int);
    void sendMsg(int, QString, QString, QString, QUuid);
    void connectToPeer(int, QString);
    void updateMessServFDS(int);
    void setItalicsOnItem(QUuid, bool);
};



#endif // PEERLIST_H
