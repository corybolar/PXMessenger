#ifndef MESS_SERV_H
#define MESS_SERV_H

#include <QThread>
#include <ctime>
#include <peerlist.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <QtCore>
#include <iostream>
#include <QWidget>

#ifdef __unix__
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <ctime>
#endif

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

//The first port is for the TCP listener, the second is for the UDP listener
//Different data types because we are still using getaddrinfo to load
//the tcp socket structures vs manually loading the UDP structures.
#define PORT "13653"
#define PORT_DISCOVER 13654
#define BACKLOG 20

class mess_serv : public QThread
{
    Q_OBJECT
    int udpRecieve(int i);
    int tcpRecieve(int i);
    int newConnection(int i);
public:
    mess_serv(QWidget *parent);
    int listener();													//Listen for new connections and recieve from connected ones.  Select is used here.
    int accept_new(int socketfd, sockaddr_storage *their_addr);		//Accept a new connection from a new peer.  Assigns a socket to the connection
    void run();														//thread starter

    int set_fdmax(int m);											//update the fdmax variable to indicate the highest socket number.  Needed for select()
    void update_fds(int s);											//add new sockets to the master fd_set
    //Maybe change these to locals and pass them around instead?
    fd_set master, read_fds, write_fds;
    int fdmax;

    void new_fds(int s);											//obsolete

signals:
    void mess_rec(const QString, const QString);					//return a message from a peer with their socket descriptor. REVISE
    void new_client(int, const QString);							//
    void peerQuit(int);												//Alert of a peer disconnect
    void mess_peers(QString hname, QString ipaddr);					//return info of discovered peers hostname and ip address
    void potentialReconnect(QString);								//return hostname of a potential reconnected peer
    void exitRecieved(QString);
    void sendIps(int);
    void sendName(int);
    void ipCheck(QString);
private slots:
    void recievePeerList(peerClass *peers);
};

#endif // MESS_SERV_H
