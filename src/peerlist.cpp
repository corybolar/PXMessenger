#include <peerlist.h>

PeerWorkerClass::PeerWorkerClass(QObject *parent, QString hostname, QString uuid, MessengerServer *server) : QObject(parent)
{
    localHostname = hostname;
    localUUID = uuid;
    realServer = server;
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
void PeerWorkerClass::newTcpConnection(evutil_socket_t s, void *bev)
{
    //bufferevent_free(bev);
    this->sendIdentityMsg(s);
}
void PeerWorkerClass::sendIdentityMsg(evutil_socket_t s)
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
    evutil_socket_t s = socket(AF_INET, SOCK_STREAM, 0);
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
void PeerWorkerClass::peerQuit(evutil_socket_t s)
{
    for(auto &itr : peerDetailsHash)
    {
        if(itr.socketDescriptor == s)
        {
            peerDetailsHash[itr.identifier].attemptingToConnect = 0;
            peerDetailsHash[itr.identifier].isConnected = false;
            peerDetailsHash[itr.identifier].socketDescriptor = -1;
            peerDetailsHash[itr.identifier].bev = NULL;
            //int s1 = socket(AF_INET, SOCK_STREAM, 0);
            //emit connectToPeer(s1, itr.ipAddress, itr.portNumber);
            emit setItalicsOnItem(itr.identifier, 1);
            return;
        }
    }
}
void PeerWorkerClass::sendIps(evutil_socket_t i)
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
void PeerWorkerClass::resultOfConnectionAttempt(evutil_socket_t socket, bool result)
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
        //emit updateMessServFDS(socket);
        struct bufferevent *bev;
        evutil_make_socket_nonblocking(socket);
        bev = bufferevent_socket_new(realServer->base, socket, BEV_OPT_CLOSE_ON_FREE);
        bufferevent_setcb(bev, realServer->tcpRead, NULL, realServer->tcpError, (void*)realServer);
        bufferevent_setwatermark(bev, EV_READ, 0, 1000);
        bufferevent_enable(bev, EV_READ);
        if(peerDetailsHash[uuid].bev != NULL)
            bufferevent_free(peerDetailsHash[uuid].bev);
        peerDetailsHash[uuid].bev = bev;
        //realServer->newConnectionRecieved(socket);
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
void PeerWorkerClass::updatePeerDetailsHash(QString hname, QString port, evutil_socket_t s, QUuid uuid, void *bevptr)
{
    if(peerDetailsHash.contains(uuid) && peerDetailsHash.value(uuid).isConnected)
    {
        return;
    }
    bufferevent *bev = (bufferevent*)bevptr;
    struct sockaddr_storage addr;
    socklen_t socklen = sizeof(addr);
    char ipaddr[INET6_ADDRSTRLEN] = {};
    char service[20];

    //Get ip address of sender
    getpeername(s, (struct sockaddr*)&addr, &socklen);
    getnameinfo((struct sockaddr*)&addr, socklen, ipaddr, sizeof(ipaddr), service, sizeof(service), NI_NUMERICHOST);
    if( !( peerDetailsHash.value(uuid).isConnected ) )
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
        peerDetailsHash[uuid].bev = bev;

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
        newPeer.bev = bev;

        peerDetailsHash.insert(uuid, newPeer);

        emit updateListWidget(uuid);
    }
}
