#include "pxmdefinitions.h"

Peers::PeerData& Peers::PeerData::operator= (Peers::PeerData &&p) noexcept
{
    if(this != &p)
    {
        delete bw;
        bw = p.bw;
        p.bw = nullptr;
        identifier = p.identifier;
        ipAddressRaw = p.ipAddressRaw;
        hostname = p.hostname;
        messages = p.messages;
        socket = p.socket;
        connectTo = p.connectTo;
        isAuthenticated = p.isAuthenticated;
    }
    return *this;
}

Peers::PeerData& Peers::PeerData::operator=(const Peers::PeerData &p)
{
    Peers::PeerData temp(p);
    *this = std::move(temp);
    return *this;
}

QString Peers::PeerData::toString(QString pad)
{
    return QString(pad % QStringLiteral("Hostname: ") % hostname % QStringLiteral("\n")
                   % pad % QStringLiteral("UUID: ") % identifier.toString() % QStringLiteral("\n")
                   % pad % QStringLiteral("IP Address: ") % QString::fromLocal8Bit(inet_ntoa(ipAddressRaw.sin_addr))
                   % QStringLiteral(":") % QString::number(ntohs(ipAddressRaw.sin_port)) % QStringLiteral("\n")
                   % pad % QStringLiteral("IsAuthenticated: ") % QString::fromLocal8Bit((isAuthenticated? "true" : "false")) % QStringLiteral("\n")
                   % pad % QStringLiteral("preventAttempConnection: ") % QString::fromLocal8Bit((connectTo ? "true" : "false")) % QStringLiteral("\n")
                   % pad % QStringLiteral("SocketDescriptor: ") % QString::number(socket) % QStringLiteral("\n")
                   % pad % QStringLiteral("History Length: ") % QString::number(messages.count()) % QStringLiteral("\n")
                   % pad % QStringLiteral("Bufferevent: ") % (bw->getBev() ? QString::asprintf("%8p",bw->getBev()) : QStringLiteral("NULL")) % QStringLiteral("\n"));
}
