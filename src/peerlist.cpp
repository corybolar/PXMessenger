#include <peerlist.h>

PeerClass::PeerClass(QObject *parent) : QObject(parent)
{
    gethostname(localHostname, sizeof localHostname);
}
/**
 * @brief 		use qsort to sort the peerlist structs alphabetically by their hostname
 * @param len	number of peers, this value is held in the peersLen member of Window
 */
void PeerClass::sortPeersByHostname(int len)
{
    qsort(knownPeersArray, len, sizeof(peerDetails), qsortCompare);
}
/**
 * @brief 		Compare function for the qsort call, could be a one liner
 * @param a		pointer to a member of peers (struct peerlist)
 * @param b		pointer to a member of peers (struct peerlist)
 * @return 		0 if equal, <1 if a before b, >1 if b before a
 */
int PeerClass::qsortCompare(const void *a, const void *b)
{
    peerDetails *pA = (peerDetails *)a;
    peerDetails *pB = (peerDetails *)b;

    return pA->hostname.compare(pB->hostname, Qt::CaseInsensitive);
}

void PeerClass::setLocalHostName(QString name)
{

}

void PeerClass::assignSocket(struct peerDetails *p)
{
    p->socketDescriptor = socket(AF_INET, SOCK_STREAM, 0);
    if(p->socketDescriptor < 0)
        perror("socket:");
    else
        p->socketisValid = 1;
    return;
}
void PeerClass::hostnameCheck(QString comp)
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
void PeerClass::newTcpConnection(int s, QString ipaddr)
{
    for(int i = 0; i < numberOfValidPeers; i++)
    {
        if(strcmp(knownPeersArray[i].ipAddress, ipaddr.toStdString().c_str()) == 0)
        {
            knownPeersArray[i].socketDescriptor = s;
            //this->assignSocket(&(peers_class->peers[i]));
            knownPeersArray[i].isConnected = true;
            emit setItalicsOnItem(i, 1);
            /*  addRemoveItalicsOnItem(int i) to MessengerWindow
     *
    if(messListWidget->item(i)->font().italic())
    {
    this->printToTextBrowser(peers_class->knownPeersArray[i].hostname + " on " + ipaddr + " reconnected", i, false);
    QFont mfont = messListWidget->item(i)->font();
    mfont.setItalic(false);
    messListWidget->item(i)->setFont(mfont);
    }
    */
            //qDebug() << "hi";
            return;
        }
    }
    //If we got here it means this new peer is not in the list, where he came from we'll never know.
    //But actually, when this happens we need to get his hostname.  Temporarily we will make his hostname
    //his ip address but will ask him for his name later on.
    listpeers(ipaddr, ipaddr, true, s);

}
void PeerClass::listpeers(QString hname, QString ipaddr)
{
    this->listpeers(hname, ipaddr, false, 0);
}
void PeerClass::setPeerHostname(QString hname, QString ipaddr)
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
void PeerClass::peerQuit(int s)
{
    for(int i = 0; i < numberOfValidPeers; i++)
    {
        if(knownPeersArray[i].socketDescriptor == s)
        {
            knownPeersArray[i].isConnected = 0;
            knownPeersArray[i].socketisValid = 0;
            emit setItalicsOnItem(i, 0);
            //emit printToTextBrowser(knownPeersArray[i].hostname + " on " + QString::fromUtf8(knownPeersArray[i].ipAddress) + " disconnected", i, false);
            this->assignSocket(&knownPeersArray[i]);
            return;
        }
    }
}
void PeerClass::sendIps(int i)
{
    char type[5] = "/ip:";
    char msg2[((numberOfValidPeers * 16) * 2)] = {};
    for(int k = 0; k < numberOfValidPeers; k++)
    {
        if(knownPeersArray[k].isConnected)
        {
            strcat(msg2, knownPeersArray[k].hostname.toStdString().c_str());
            strcat(msg2, "@");
            strcat(msg2, knownPeersArray[k].ipAddress);
            strcat(msg2, ":");
        }
    }
    //messClient->send_msg(i, msg2, localHostname, type);
    emit sendMsg(i, QString::fromUtf8(msg2), QString::fromUtf8(localHostname), QString::fromUtf8(type));
}
void PeerClass::resultOfConnectionAttempt(int socket, bool result)
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
                if(strcmp(localHostname, knownPeersArray[i].hostname.toStdString().c_str()))
                {
                    emit sendMsg(knownPeersArray[i].socketDescriptor, "", "", "/request");
                }
            }
            else
            {

                knownPeersArray[i].isConnected = false;
                emit printToTextBrowser("Could not connect to " + knownPeersArray[i].hostname + " on socket " + QString::number(knownPeersArray[i].socketDescriptor), i, false);
                emit setItalicsOnItem(i, 0);
            }
            return;
        }
    }
}
void PeerClass::resultOfTCPSend(int levelOfSuccess, int socket, QString msg, bool print)
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
void PeerClass::listpeers(QString hname, QString ipaddr, bool test, int s)
{
    //MessengerWindow *parentWindow = qobject_cast<MessengerWindow>(this->parent());
    QByteArray ba = ipaddr.toLocal8Bit();
    const char *ipstr = ba.data();
    int i = 0;
    for( ; ( knownPeersArray[i].isValid ); i++ )
    {
        if( (ipaddr.compare(knownPeersArray[i].ipAddress) ) == 0 )
        {
            return;
        }
    }
    knownPeersArray[i].hostname = hname;
    knownPeersArray[i].isValid = true;
    strcpy(knownPeersArray[i].ipAddress, ipstr);
    numberOfValidPeers++;
    if( !test )
    {
        assignSocket(&(knownPeersArray[i]));
        emit connectToPeer(knownPeersArray[i].socketDescriptor, QString::fromUtf8(knownPeersArray[i].ipAddress));
        /*if(parentWindow->messClient->c_connect(knownPeersArray[i].socketDescriptor, knownPeersArray[i].ipAddress) >= 0)
    {
    knownPeersArray[i].isConnected = true;
    parentWindow->messServer->update_fds(knownPeersArray[i].socketDescriptor);
    if(strcmp(parentWindow->localHostname, knownPeersArray[i].hostname.toStdString().c_str()))
    {
    emit sendMsg(knownPeersArray[i].socketDescriptor, "", "", "/request");
    }
    }
    */
    }
    else
    {
        knownPeersArray[i].socketDescriptor = s;
        knownPeersArray[i].isConnected = true;
        emit sendMsg(knownPeersArray[i].socketDescriptor, "", "", "/request");
    }

    qDebug() << "hostname: " << knownPeersArray[i].hostname << " @ ip:" << QString::fromUtf8(knownPeersArray[i].ipAddress);

    if(!(hname.compare(ipaddr)))
    {
        struct sockaddr_storage addr;
        socklen_t socklen = sizeof(addr);
        char ipstr2[INET6_ADDRSTRLEN];
        char service[20];

        getpeername(knownPeersArray[i].socketDescriptor, (struct sockaddr*)&addr, &socklen);
        //struct sockaddr_in *temp = (struct sockaddr_in *)&addr;
        //inet_ntop(AF_INET, &temp->sin_addr, ipstr2, sizeof ipstr2);
        getnameinfo((struct sockaddr*)&addr, socklen, ipstr2, sizeof(ipstr2), service, sizeof(service), NI_NUMERICHOST);
        qDebug() << "need name, sending namerequest to" << ipaddr;
        sendMsg(knownPeersArray[i].socketDescriptor, "", "", "/namerequest");
    }
    sortPeersByHostname(numberOfValidPeers);
    emit updateListWidget(numberOfValidPeers);
}
