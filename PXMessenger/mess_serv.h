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
    int listener();
    //static void * t_recieve(void *arguments);
    int accept_new(int socketfd, sockaddr_storage *their_addr);
    int socketfd;
    void run();

    std::vector<int> socketDesc;
    int set_fdmax(int m);
    void update_fds(int s);
    fd_set master, read_fds;
    void new_fds(int s);
signals:
    void mess_rec(const QString);
    void new_client(int, const QString);
private slots:
    void mess_rec3(const QString s);
};

#endif // MESS_SERV_H
