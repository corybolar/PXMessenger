#include <peerlist.h>

PeerWorkerClass::PeerWorkerClass(QObject *parent) : QObject(parent)
{
    char tempLocalName[128];
    gethostname(tempLocalName, sizeof tempLocalName);
    localHostname = QString::fromUtf8(tempLocalName);
}
/**
 * @brief 		use qsort to sort the peerlist structs alphabetically by their hostname
 * @param len	number of peers, this value is held in the peersLen member of Window
 */
void PeerWorkerClass::sortPeersByHostname(int len)
{
    qsort(knownPeersArray, len, sizeof(peerDetails), qsortCompare);
}
/**
 * @brief 		Compare function for the qsort call, could be a one liner
 * @param a		pointer to a member of peers (struct peerlist)
 * @param b		pointer to a member of peers (struct peerlist)
 * @return 		0 if equal, <1 if a before b, >1 if b before a
 */
int PeerWorkerClass::qsortCompare(const void *a, const void *b)
{
    peerDetails *pA = (peerDetails *)a;
    peerDetails *pB = (peerDetails *)b;

    return pA->hostname.compare(pB->hostname, Qt::CaseInsensitive);
}
void PeerWorkerClass::setLocalHostName(QString name)
{
    localHostname = name;

}
void PeerWorkerClass::assignSocket(struct peerDetails *p)
{
    p->socketDescriptor = socket(AF_INET, SOCK_STREAM, 0);
    if(p->socketDescriptor < 0)
        perror("socket:");
    else
        p->socketisValid = 1;
    return;
}
void PeerWorkerClass::hostnameCheck(QString comp)
{
    QStringList temp = comp.split('@');
    QString hname = temp[0];
    QString ipaddr = temp[1];
    for(int i = 0; i < numberOfValidPeers; i++)
    {
        if(hname.compare(knownPeersArray[i].hostname) == 0)
        {
            return;
        }
    }
    listpeers(hname, ipaddr);
    return;
}
void PeerWorkerClass::newTcpConnection(int s, QString ipaddr)
{
    for(int i = 0; i < numberOfValidPeers; i++)
    {
        if(knownPeersArray[i].ipAddress == ipaddr)
        {
            knownPeersArray[i].socketDescriptor = s;
            //this->assignSocket(&(peers_class->peers[i]));
            knownPeersArray[i].isConnected = true;
            emit setItalicsOnItem(i, 0);
            return;
        }
    }
    //If we got here it means this new peer is not in the list, where he came from we'll never know.
    //But actually, when this happens we need to get his hostname.  Temporarily we will make his hostname
    //his ip address but will ask him for his name later on.
    listpeers(ipaddr, ipaddr, false, s);
}
void PeerWorkerClass::listpeers(QString hname, QString ipaddr)
{
    this->listpeers(hname, ipaddr, true, 0);
}
void PeerWorkerClass::setPeerHostname(QString hname, QString ipaddr)
{
    for(int i = 0; i < numberOfValidPeers; i++)
    {
        if(ipaddr.compare(knownPeersArray[i].ipAddress) == 0)
        {
            knownPeersArray[i].hostname = hname;
            emit updateListWidget(numberOfValidPeers);
            return;
        }
    }
}
void PeerWorkerClass::peerQuit(int s)
{
    for(int i = 0; i < numberOfValidPeers; i++)
    {
        if(knownPeersArray[i].socketDescriptor == s)
        {
            knownPeersArray[i].isConnected = 0;
            knownPeersArray[i].socketisValid = 0;
            emit setItalicsOnItem(i, 1);
            //emit printToTextBrowser(knownPeersArray[i].hostname + " on " + QString::fromUtf8(knownPeersArray[i].ipAddress) + " disconnected", i, false);
            this->assignSocket(&knownPeersArray[i]);
            return;
        }
    }
}
void PeerWorkerClass::sendIps(int i)
{
    QString type = "/ip:";
    QString msg = "";
    for(int k = 0; k < numberOfValidPeers; k++)
    {
        if(knownPeersArray[k].isConnected)
        {
            msg.append(knownPeersArray[k].hostname);
            msg.append("@");
            msg.append(knownPeersArray[k].ipAddress);
            msg.append(":");
        }
    }
    emit sendMsg(i, msg, localHostname, type, "");
}
void PeerWorkerClass::resultOfConnectionAttempt(int socket, bool result)
{
    for(int i = 0; i < numberOfValidPeers; i++)
    {
        if(knownPeersArray[i].socketDescriptor == socket)
        {
            if(!result)
            {
                knownPeersArray[i].isConnected = true;
                knownPeersArray[i].socketisValid = true;
                emit updateMessServFDS(knownPeersArray[i].socketDescriptor);
                if(localHostname != knownPeersArray[i].hostname)
                {
                    emit sendMsg(knownPeersArray[i].socketDescriptor, "", "", "/request", "");
                }
            }
            else
            {

                knownPeersArray[i].isConnected = false;
                emit printToTextBrowser("Could not connect to " + knownPeersArray[i].hostname + " on socket " + QString::number(knownPeersArray[i].socketDescriptor), i, false);
                emit setItalicsOnItem(i, 1);
            }
            return;
        }
    }
}
void PeerWorkerClass::resultOfTCPSend(int levelOfSuccess, int socket, QString msg, bool print)
{
    if(print)
    {
        int i;
        for(i = 0; i < numberOfValidPeers; i++)
        {
            if(knownPeersArray[i].socketDescriptor == socket)
            {
                if(levelOfSuccess < 0)
                {
                    msg = "Message was not sent successfully, Broken Pipe.  Peer likely disconnected";
                    peerQuit(socket);
                }
                else if(levelOfSuccess > 0)
                {
                    msg.append("\nThe previous message was only paritally sent.  This was very bad\nContact the administrator of this program immediately\nNumber of bytes sent: " + QString::number(levelOfSuccess));
                }
                else if(levelOfSuccess == 0)
                {

                }
                emit printToTextBrowser(msg, i, true);

                return;
            }
        }
    }
    if(levelOfSuccess<0)
        peerQuit(socket);
}
/**
 * @brief				This is the function called when mess_discover recieves a udp packet starting with "/name:"
 * 						here we check to see if the ip of the selected host is already in the peers array and if not
 * 						add it.  peers is then sorted and displayed to the gui.  QStrings are used for ease of
 * 						passing through the QT signal and slots mechanism.
 * @param hname			Hostname of peer to compare to existing hostnames
 * @param ipaddr		IP address of peer to compare to existing IP addresses
 */
void PeerWorkerClass::listpeers(QString hname, QString ipaddr, bool isThisFromUDP, int s)
{
    //MessengerWindow *parentWindow = qobject_cast<MessengerWindow>(this->parent());
    QByteArray ba = ipaddr.toLocal8Bit();
    int i = 0;
    for( ; ( knownPeersArray[i].isValid ); i++ )
    {
        if( (ipaddr.compare(knownPeersArray[i].ipAddress) ) == 0 )
        {
            return;
        }
    }

    for(auto &itr : peerDetailsHash)
    {
        if(itr.ipAddress == ipaddr)
            return;
    }

    knownPeersArray[i].hostname = hname;
    knownPeersArray[i].isValid = true;
    knownPeersArray[i].ipAddress = ipaddr;
    numberOfValidPeers++;

    if( isThisFromUDP )
    {
        assignSocket(&(knownPeersArray[i]));
        emit connectToPeer(knownPeersArray[i].socketDescriptor, knownPeersArray[i].ipAddress);
    }
    else
    {
        knownPeersArray[i].socketDescriptor = s;
        knownPeersArray[i].isConnected = true;
        emit sendMsg(knownPeersArray[i].socketDescriptor, "", "", "/request", "");
    }

    qDebug() << "hostname: " << knownPeersArray[i].hostname << " @ ip:" << knownPeersArray[i].ipAddress;

    if(hname == ipaddr)
    {
        qDebug() << "need name, sending namerequest to" << ipaddr;
        sendMsg(knownPeersArray[i].socketDescriptor, "", "", "/namerequest", "");
    }

    //UUID TESTING BELOW THIS
    QUuid temp = QUuid::createUuid();
    knownPeersArray[i].identifier = temp;

    peerDetailsHash.insert(knownPeersArray[i].hostname, knownPeersArray[i]);
    for(auto &itr : peerDetailsHash)
    {
        qDebug() << itr.hostname;
    }
    qDebug() << peerDetailsHash.size();
    sortPeersByHostname(numberOfValidPeers);
    emit updateListWidget(numberOfValidPeers);
}
