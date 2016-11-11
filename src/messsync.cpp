#include "messsync.h"

MessSync::MessSync(QObject *parent, QHash<QUuid, struct peerDetails> *hash) : QThread(parent)
{
    peerDetailsHash = hash;
}
void MessSync::run()
{
    for(auto &itr : (*peerDetailsHash))
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
