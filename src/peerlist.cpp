#include <peerlist.h>

PeerWorkerClass::PeerWorkerClass(QObject *parent, QString hostname, QString uuid) : QObject(parent)
{
    localHostname = hostname;
    localUUID = uuid;
}
void PeerWorkerClass::setLocalHostName(QString name)
{
    localHostname = name;
}
void PeerWorkerClass::setListenerPort(QString port)
{
    ourListenerPort = port;

}
void PeerWorkerClass::hostnameCheck(QString comp)
{
    QStringList temp = comp.split(':');
    QString ipaddr = temp[0];
    QString port = temp[1];
    QString uuid = temp[2];
    if(peerDetailsHash.contains(uuid) && peerDetailsHash.value(uuid).isConnected)
        return;
    else
        attemptConnection(port, ipaddr, uuid);
}
void PeerWorkerClass::newTcpConnection(int s)
{
    this->sendIdentityMsg(s);
}
void PeerWorkerClass::sendIdentityMsg(int s)
{
    emit sendMsg(s, "", localHostname + ":" + ourListenerPort, "/uuid", localUUID, "");
    emit sendMsg(s, "", "", "/request", localUUID, "");
}
void PeerWorkerClass::attemptConnection(QString portNumber, QString ipaddr, QString uuid)
{
    if(peerDetailsHash.value(uuid).attemptingToConnect || uuid.isNull())
    {
        return;
    }
    int s = socket(AF_INET, SOCK_STREAM, 0);
    if(peerDetailsHash.contains(uuid))
    {
        peerDetailsHash[uuid].identifier = uuid;
        peerDetailsHash[uuid].ipAddress = ipaddr;
        peerDetailsHash[uuid].socketDescriptor = s;
        peerDetailsHash[uuid].portNumber = portNumber;
        peerDetailsHash[uuid].attemptingToConnect = true;
    }
    else
    {
        peerDetails newPeer;
        newPeer.identifier = uuid;
        newPeer.ipAddress = ipaddr;
        newPeer.socketDescriptor = s;
        newPeer.portNumber = portNumber;
        newPeer.attemptingToConnect = true;
        peerDetailsHash.insert(newPeer.identifier, newPeer);
    }
    emit connectToPeer(s, ipaddr, portNumber);
}
void PeerWorkerClass::setPeerHostname(QString hname, QUuid uuid)
{
    if(peerDetailsHash.contains(uuid))
    {
        peerDetailsHash[uuid].hostname = hname;
        emit updateListWidget(uuid);
    }
    return;
}
void PeerWorkerClass::peerQuit(int s)
{
    for(auto &itr : peerDetailsHash)
    {
        if(itr.socketDescriptor == s)
        {
            peerDetailsHash[itr.identifier].attemptingToConnect = 0;
            peerDetailsHash[itr.identifier].isConnected = false;
            peerDetailsHash[itr.identifier].socketDescriptor = -1;
            //int s1 = socket(AF_INET, SOCK_STREAM, 0);
            //emit connectToPeer(s1, itr.ipAddress, itr.portNumber);
            emit setItalicsOnItem(itr.identifier, 1);
            return;
        }
    }
}
void PeerWorkerClass::sendIps(int i)
{
    QString type = "/ip:";
    QString msg = "";
    for(auto & itr : peerDetailsHash)
    {
        if(itr.isConnected && (itr.hostname != itr.ipAddress))
        {
            msg.append("[");
            msg.append(itr.ipAddress);
            msg.append(":");
            msg.append(itr.portNumber);
            msg.append(":");
            msg.append(itr.identifier.toString());
            msg.append("]");
        }
    }
    emit sendMsg(i, msg, localHostname, type, localUUID, "");
}
void PeerWorkerClass::resultOfConnectionAttempt(int socket, bool result)
{
    QUuid uuid;
    for(auto &itr : peerDetailsHash)
    {
        if(itr.socketDescriptor == socket)
        {
            uuid = itr.identifier;
        }
    }
    if(uuid.isNull())
        return;
    if(!result)
    {
        peerDetailsHash[uuid].attemptingToConnect = false;
        emit updateMessServFDS(socket);
        emit sendIdentityMsg(socket);
    }
    else
    {
        peerDetailsHash[uuid].attemptingToConnect = false;
        peerDetailsHash[uuid].isConnected = false;
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
            peerQuit(peerDetailsHash.value(uuid).socketDescriptor);
        }
        if(levelOfSuccess > 0)
        {
            msg.append("\nThe previous message was only paritally sent.  This was very bad\nContact the administrator of this program immediately\nNumber of bytes sent: " + QString::number(levelOfSuccess));
        }
        if(levelOfSuccess == 0)
        {

        }
        emit printToTextBrowser(msg, uuid, print);


        return;
    }
    if(levelOfSuccess<0)
        peerQuit(peerDetailsHash.value(uuid).socketDescriptor);
}
/**
 * @brief				This is the function called when mess_discover recieves a udp packet starting with "/name:"
 * 						here we check to see if the ip of the selected host is already in the peers array and if not
 * 						add it.  peers is then sorted and displayed to the gui.  QStrings are used for ease of
 * 						passing through the QT signal and slots mechanism.
 * @param hname			Hostname of peer to compare to existing hostnames
 * @param ipaddr		IP address of peer to compare to existing IP addresses
 */
void PeerWorkerClass::updatePeerDetailsHash(QString hname, QString ipaddr, QString port, int s, QUuid uuid)
{
    if(peerDetailsHash.contains(uuid) && peerDetailsHash.value(uuid).isConnected)
    {
        return;
    }
    else if( !( peerDetailsHash.value(uuid).isConnected ) )
    {
        qDebug() << "hostname:" << hname << "@ ip:" << ipaddr << "on port" << port << "reconnected!";

        peerDetailsHash[uuid].socketDescriptor = s;
        peerDetailsHash[uuid].isConnected = true;
        peerDetailsHash[uuid].attemptingToConnect = true;
        peerDetailsHash[uuid].isValid = true;
        peerDetailsHash[uuid].ipAddress = ipaddr;
        peerDetailsHash[uuid].identifier = uuid;
        peerDetailsHash[uuid].hostname = hname;
        peerDetailsHash[uuid].portNumber = port;

        emit setItalicsOnItem(uuid, 0);
        emit updateListWidget(uuid);
        return;
    }
    else
    {
        qDebug() << "hostname: " << hname << " @ ip:" << ipaddr;

        peerDetails newPeer;

        newPeer.socketDescriptor = s;
        newPeer.isConnected = true;
        newPeer.attemptingToConnect = true;
        newPeer.isValid = true;
        newPeer.ipAddress = ipaddr;
        newPeer.identifier = uuid;
        newPeer.hostname = hname;
        newPeer.portNumber = port;

        peerDetailsHash.insert(uuid, newPeer);

        emit updateListWidget(uuid);
    }
}
