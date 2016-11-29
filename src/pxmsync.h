#ifndef PXMSYNC_H
#define PXMSYNC_H

#include <QWidget>
#include <QMap>
#include <QUuid>

#include <event2/util.h>

#include <pxmdefinitions.h>

class PXMSync : public QObject
{
    Q_OBJECT
public:
    PXMSync(QObject *parent);
    int iterator = 0;
    void setsyncHash(QHash<QUuid, peerDetails> *hash);
    QHash<QUuid, peerDetails>::iterator hashIterator;
    void setIteratorToStart();
private:
    QHash<QUuid, peerDetails> *syncHash;
public slots:
    void syncNext();

signals:
    void requestIps(evutil_socket_t, QUuid);
    void syncComplete();
};

#endif // MESSSYNC_H
