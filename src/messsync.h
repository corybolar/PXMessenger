#ifndef MESSSYNC_H
#define MESSSYNC_H
#include <QObject>
#include <QDebug>
#include <QHash>
#include <QThread>
#include <QWidget>
#include <event2/util.h>
#include <mess_structs.h>


class MessSync : public QThread
{
    Q_OBJECT
public:
    MessSync(QObject *parent, QHash<QUuid, struct peerDetails> *hash);
    void run();
    volatile bool moveOnPlease;
private:
    QHash<QUuid, struct peerDetails> *peerDetailsHash;
signals:
    void requestIps(evutil_socket_t, QUuid);
};

#endif // MESSSYNC_H
