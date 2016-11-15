#include "pxmsync.h"

PXMSync::PXMSync(QObject *parent) : QThread(parent)
{
}
void PXMSync::run()
{
    for(auto &itr : peerDetailsHash)
    {
        if(itr.isConnected == false)
            continue;
        moveOnPlease = false;
        qDebug() << "requesting Ips from" << itr.hostname;
        emit requestIps(itr.socketDescriptor, itr.identifier);
        int count = 0;
        while(!moveOnPlease && (count < 5) && !(this->isInterruptionRequested()) )
        {
            count++;
            this->usleep(500000);
        }
    }
}

void PXMSync::setHash(QHash<QUuid, peerDetails> hash)
{
   this->peerDetailsHash = hash;
}
