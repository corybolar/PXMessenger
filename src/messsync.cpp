#include "messsync.h"

MessSync::MessSync(QObject *parent) : QThread(parent)
{
}
void MessSync::run()
{
    for(auto &itr : peerDetailsHash)
    {
        moveOnPlease = false;
        qDebug() << "requesting Ips from" << itr.hostname;
        emit requestIps(itr.socketDescriptor, itr.identifier);
        int count = 0;
        while(!moveOnPlease && (count < 10) && !(this->isInterruptionRequested()) )
        {
            count++;
            this->usleep(500000);
        }
    }
}

void MessSync::setHash(QHash<QUuid, peerDetails> hash)
{
   this->peerDetailsHash = hash;
}
