#ifndef MESS_SERV_H
#define MESS_SERV_H
#include <sys/socket.h>
#include <QtCore>
#include <QThread>
#include <vector>


class mess_serv : public QThread
{
    Q_OBJECT
public:
    mess_serv();
    int listener();													//Listen for new connections and recieve from connected ones.  Select is used here.
    int accept_new(int socketfd, sockaddr_storage *their_addr);		//Accept a new connection from a new peer.  Assigns a socket to the connection
    int socketfd;													//global listener socket descriptor, need to change to local if possible
    void run();														//thread starter

    std::vector<int> socketDesc;
    int set_fdmax(int m);											//update the fdmax variable to indicate the highest socket number.  Needed for select()
    void update_fds(int s);											//add new sockets to the master fd_set
    fd_set master, read_fds, write_fds;
    int fdmax;
    void new_fds(int s);											//obsolete
signals:
    void mess_rec(const QString, const QString);								//return a message from a peer with their socket descriptor. REVISE
    void new_client(int, const QString);							//
    void peerQuit(int);												//Alert of a peer disconnect
private slots:
};

#endif // MESS_SERV_H
