#include <peerlist.h>

PeerWorkerClass::PeerWorkerClass(QObject *parent, QString hostname) : QObject(parent)
{
    localHostname = hostname;
}
void PeerWorkerClass::setLocalHostName(QString name)
{
    localHostname = name;

}
void PeerWorkerClass::hostnameCheck(QString comp)
{
    QStringList temp = comp.split(':');
    QString hname = temp[0];
    hname = hname.remove(hname.length(), 1);
    QString ipaddr = temp[1];
    ipaddr.chop(1);
    for(auto &itr : peerDetailsHash)
    {
        if(itr.hostname == hname)
            return;
    }
    updatePeerDetailsHash(hname, ipaddr, true, 0, "");
    return;
}
void PeerWorkerClass::newTcpConnection(int s, QString ipaddr, QUuid uuid)
{
    for(auto &itr : peerDetailsHash)
    {
        if(itr.ipAddress == ipaddr)
        {
            peerDetails p = peerDetailsHash.take(itr.identifier);

            p.ipAddress = ipaddr;
            p.socketDescriptor = s;
            p.identifier = uuid;

            peerDetailsHash.insert(uuid, p);
            emit sendMsg(s, "", localHostname, "/uuid", uuid);
            emit sendMsg(s, "", "", "/request", uuid);
            emit updateListWidget(0, uuid);
            emit setItalicsOnItem(p.identifier,0);
            return;
        }
    }
    //If we got here it means this new peer is not in the list, where he came from we'll never know.
    //But actually, when this happens we need to get his hostname.  Temporarily we will make his hostname
    //his ip address but will ask him for his name later on.

    //This line gives him the uuid of the connection.  He doesn't have this because
    //we created this uuid.  The server in the relationship always creates the uuid
    emit sendMsg(s, "", localHostname, "/uuid", uuid);

    updatePeerDetailsHash(ipaddr, ipaddr, false, s, uuid);
}
void PeerWorkerClass::updatePeerDetailsHash(QString hname, QString ipaddr)
{
    //assignSocket(&(knownPeersArray[i]));
    for(auto &itr : peerDetailsHash)
    {
        if(itr.ipAddress == ipaddr)
        {
            return;
        }
    }

    int s = socket(AF_INET, SOCK_STREAM, 0);
    emit connectToPeer(s, ipaddr);
    //this->updatePeerDetailsHash(hname, ipaddr, true, 0, "");
}

void PeerWorkerClass::setPeerHostname(QString hname, QUuid uuid)
{

    if(peerDetailsHash[uuid].isValid)
    {
        peerDetailsHash[uuid].hostname = hname;
        emit updateListWidget(peerDetailsHash[uuid].listWidgetIndex, uuid);
    }
    else
        peerDetailsHash.remove(uuid);
    return;
}
void PeerWorkerClass::peerQuit(int s)
{
    for(auto &itr : peerDetailsHash)
    {
        if(itr.socketDescriptor == s)
        {
            this->peerQuit(itr.identifier);
#ifdef _WIN32
        closesocket(itr.socketDescriptor);
#else
        close(itr.socketDescriptor);
#endif
            int s1 = socket(AF_INET, SOCK_STREAM, 0);
            emit connectToPeer(s1, itr.ipAddress);
            return;
        }
    }
}

void PeerWorkerClass::peerQuit(QUuid uuid)
{
    if(peerDetailsHash.value(uuid).isValid)
    {
        peerDetailsHash[uuid].isConnected = 0;
        peerDetailsHash[uuid].socketisValid = 0;
        emit setItalicsOnItem(uuid, 1);
    }
    else
        peerDetailsHash.remove(uuid);
}
void PeerWorkerClass::sendIps(int i)
{
    QString type = "/ip:";
    QString msg = "";
    QUuid uuid;
    for(auto & itr : peerDetailsHash)
    {
        if(itr.isConnected && (itr.hostname != itr.ipAddress))
        {
            msg.append("[");
            msg.append(itr.hostname);
            msg.append(":");
            msg.append(itr.ipAddress);
            msg.append("]");
        }
        if(itr.socketDescriptor == i)
        {
            uuid = itr.identifier;
        }
    }
    emit sendMsg(i, msg, localHostname, type, uuid);
}
void PeerWorkerClass::resultOfConnectionAttempt(int socket, bool result)
{
    if(!result)
    {
        emit updateMessServFDS(socket);
    }
}
void PeerWorkerClass::resultOfTCPSend(int levelOfSuccess, QString uuidString, QString msg, bool print)
{
    QUuid uuid = uuidString;
    if(print)
    {
        if(levelOfSuccess < 0)
        {
            msg = "Message was not sent successfully, Broken Pipe.  Peer likely disconnected";
            peerQuit(uuid);
        }
        if(levelOfSuccess > 0)
        {
            msg.append("\nThe previous message was only paritally sent.  This was very bad\nContact the administrator of this program immediately\nNumber of bytes sent: " + QString::number(levelOfSuccess));
        }
        if(levelOfSuccess == 0)
        {

        }
        emit printToTextBrowser(msg, uuid, true);
        return;
    }
    if(levelOfSuccess<0)
        peerQuit(uuid);
}
/**
 * @brief				This is the function called when mess_discover recieves a udp packet starting with "/name:"
 * 						here we check to see if the ip of the selected host is already in the peers array and if not
 * 						add it.  peers is then sorted and displayed to the gui.  QStrings are used for ease of
 * 						passing through the QT signal and slots mechanism.
 * @param hname			Hostname of peer to compare to existing hostnames
 * @param ipaddr		IP address of peer to compare to existing IP addresses
 */
void PeerWorkerClass::updatePeerDetailsHash(QString hname, QString ipaddr, bool haveWeNotHeardOfThisPeer, int s, QUuid uuid)
{
    peerDetails newPeer;
    for(auto &itr : peerDetailsHash)
    {
        if(itr.ipAddress == ipaddr)
        {
            if(itr.hostname != hname)
            {
                itr.hostname = hname;
            }
            if(itr.identifier != uuid)
            {
                qDebug() << "Warning: changing uuid for a peer, they should be the same here";
                peerDetails p = peerDetailsHash.take(itr.identifier);
                p.identifier = uuid;
                p.socketDescriptor = s;
                p.socketisValid = true;
                p.isConnected = true;
                peerDetailsHash.insert(uuid, p);
                emit sendMsg(s, "", "", "/request", uuid);
                emit updateListWidget(0, uuid);
                emit setItalicsOnItem(p.identifier,0);
            }
            return;
        }
    }
    newPeer.socketDescriptor = s;
    newPeer.isConnected = true;
    newPeer.socketisValid = true;
    newPeer.isValid = true;
    newPeer.ipAddress = ipaddr;
    newPeer.identifier = uuid;

    qDebug() << "hostname: " << newPeer.hostname << " @ ip:" << newPeer.ipAddress;


    if(hname == ipaddr)
    {
        emit sendMsg(s, "", "", "/request", uuid);
        qDebug() << "need name, sending namerequest to" << ipaddr;
        sendMsg(newPeer.socketDescriptor, "", "", "/namerequest", uuid);
        peerDetailsHash.insert(uuid, newPeer);
    }
    else
    {
        newPeer.hostname = hname;
        peerDetailsHash.insert(uuid, newPeer);
        emit updateListWidget(newPeer.listWidgetIndex, newPeer.identifier);
    }
}
