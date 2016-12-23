#include "pxmsync.h"

PXMSync::PXMSync(QObject *parent) : QObject(parent)
{

}
void PXMSync::syncNext()
{
    while(hashIterator != syncHash->end() && !(hashIterator.value().isAuthenticated))
    {
        hashIterator++;
    }
    if(hashIterator == syncHash->end())
    {
        emit syncComplete();
        return;
    }
    else
    {
        emit requestIps(hashIterator.value().bw, hashIterator.value().identifier);
    }
    hashIterator++;
}

void PXMSync::setsyncHash(QHash<QUuid, Peers::PeerData> *hash)
{
   syncHash = hash;
   hashIterator = QHash<QUuid, Peers::PeerData>::iterator();
}
void PXMSync::setIteratorToStart()
{
    hashIterator = syncHash->begin();
}
