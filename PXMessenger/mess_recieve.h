#ifndef MESS_RECIEVE_H
#define MESS_RECIEVE_H
#include <QThread>
#include <QString>


class mess_recieve : public QThread
{
    Q_OBJECT
public:
    explicit mess_recieve(QString s, int new_fd);
    ~mess_recieve();

    void run();
private:
    QString hostip;
    int sock;
signals:
    void mess_rec2(const QString);
};

#endif // MESS_RECIEVE_H
