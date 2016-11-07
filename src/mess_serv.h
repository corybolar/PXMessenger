#ifndef MESS_SERV_H
#define MESS_SERV_H

#include <QThread>
#include <QWidget>
#include <ctime>
//#include <peerlist.h>
#include <sys/types.h>
#include <sys/time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <QtCore>
#include <iostream>
#include <event2/event.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/util.h>
#include <assert.h>

#ifdef __unix__
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <ctime>
#include <fcntl.h>
#endif

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

#define PORT_DISCOVER 13649
#define BACKLOG 20
#define TCP_BUFFER_LENGTH 10000

class MessengerServer : public QThread
{
    Q_OBJECT

public:
    MessengerServer(QWidget *parent);
    int listener();													//Listen for new connections and recieve from connected ones.  Select is used here.
    void run();														//thread starter
    void setLocalHostname(QString hostname);
    void setLocalUUID(QString uuid);
    ~MessengerServer();
    struct event_base *base;
    static void tcpError(bufferevent *buf, short error, void *ctx);
    static void tcpRead(bufferevent *bev, void *ctx);
public slots:
private:
    static void udpRecieve(int i, short y, void *args);
    static void accept_new(int socketfd, short event, void *arg);		//Accept a new connection from a new peer.  Assigns a socket to the connection
    //Maybe change these to locals and pass them around instead?
    char *tcpBuffer;
    QString localHostname;
    QString localUUID;
    QString ourListenerPort;

    int singleMessageIterator(int i, char *buf, char *ipstr);
    int getPortNumber(int socket);
    int setupUDPSocket(int s_listen);
    int setupTCPSocket();
    int setSocketToNonBlocking(int socket);
signals:
    void messageRecieved(const QString, QUuid, bool);					//return a message from a peer with their socket descriptor. REVISE
    void newConnectionRecieved(int);							//
    void recievedUUIDForConnection(QString, QString, QString, int, QUuid);
    void peerQuit(int);												//Alert of a peer disconnect
    void updNameRecieved(QString hname, QString ipaddr, QString uuid);					//return info of discovered peers hostname and ip address
    void potentialReconnect(QString);								//return hostname of a potential reconnected peer
    void exitRecieved(QString);
    void sendIps(int);
    void sendName(int, QString, QString);
    void hostnameCheck(QString);
    void setPeerHostname(QString, QUuid);
    void sendMsg(int, QString, QString, QString, QUuid, QString);
    void sendUdp(QString);
    void setListenerPort(QString);
};

#endif // MESS_SERV_H
