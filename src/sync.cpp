#include "sync.h"

#include <QUuid>

#include "peers.h"

PXMSync::PXMSync(QObject* parent, QHash<QUuid, Peers::PeerData>& hash) : QObject(parent), syncHash(hash)
{
}
void PXMSync::syncNext()
{
    while (hashIterator != syncHash.end() && !(hashIterator.value().isAuthed)) {
        hashIterator++;
    }
    if (hashIterator == syncHash.end()) {
        emit syncComplete();
        return;
    } else {
        emit requestIps(hashIterator.value().bw, hashIterator.value().uuid);
    }
    hashIterator++;
}

void PXMSync::setsyncHash(const QHash<QUuid, Peers::PeerData>& hash)
{
    syncHash     = hash;
    hashIterator = QHash<QUuid, Peers::PeerData>::iterator();
}
void PXMSync::setIteratorToStart()
{
    hashIterator = syncHash.begin();
}
