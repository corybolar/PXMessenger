#ifndef PXMSYNC_H
#define PXMSYNC_H

#include <QDebug>
#include <QHash>
#include <QThread>
#include <QWidget>

#include <event2/util.h>

#include <pxmdefinitions.h>

class PXMSync : public QThread
{
    Q_OBJECT
public:
    PXMSync(QObject *parent);
    void run();
    void setHash(QHash<QUuid, struct peerDetails> hash);
    volatile bool moveOnPlease;
private:
    QHash<QUuid, struct peerDetails> peerDetailsHash;
signals:
    void requestIps(evutil_socket_t, QUuid);
};

#endif // MESSSYNC_H
