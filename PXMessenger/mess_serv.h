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
    void run();
    int socketfd;

    std::vector<int> socketDesc;
signals:
    void mess_rec(const QString);
private slots:
    void mess_rec3(const QString s);
};

#endif // MESS_SERV_H
