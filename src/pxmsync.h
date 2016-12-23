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
    QHash<QUuid, Peers::PeerData> *syncHash;
public:
    PXMSync(QObject *parent);
    void setsyncHash(QHash<QUuid, Peers::PeerData> *hash);
    QHash<QUuid, Peers::PeerData>::iterator hashIterator;
    void setIteratorToStart();
public slots:
    void syncNext();
signals:
    void requestIps(Peers::BevWrapper*, QUuid);
    void syncComplete();
};

#endif // MESSSYNC_H
