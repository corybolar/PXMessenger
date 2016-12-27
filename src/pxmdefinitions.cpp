#include "pxmdefinitions.h"
using namespace Peers;

PeerData& PeerData::operator= (PeerData &&p) noexcept
{
    if(this != &p)
    {
        bw = p.bw;
        p.bw.clear();
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

PeerData& PeerData::operator=(const PeerData &p)
{
    Peers::PeerData temp(p);
    *this = std::move(temp);
    return *this;
}

QString PeerData::toInfoString()
{
    return QString(QStringLiteral("Hostname: ") % hostname % QStringLiteral("\n")
                   % QStringLiteral("UUID: ") % identifier.toString() % QStringLiteral("\n")
                   % QStringLiteral("IP Address: ") % QString::fromLocal8Bit(inet_ntoa(ipAddressRaw.sin_addr))
                   % QStringLiteral(":") % QString::number(ntohs(ipAddressRaw.sin_port)) % QStringLiteral("\n")
                   % QStringLiteral("IsAuthenticated: ") % QString::fromLocal8Bit((isAuthenticated? "true" : "false")) % QStringLiteral("\n")
                   % QStringLiteral("preventAttempConnection: ") % QString::fromLocal8Bit((connectTo ? "true" : "false")) % QStringLiteral("\n")
                   % QStringLiteral("SocketDescriptor: ") % QString::number(socket) % QStringLiteral("\n")
                   % QStringLiteral("History Length: ") % QString::number(messages.count()) % QStringLiteral("\n")
                   % QStringLiteral("Bufferevent: ") % (bw->getBev() ? QString::asprintf("%8p",bw->getBev()) : QStringLiteral("NULL")) % QStringLiteral("\n"));
}

BevWrapper::~BevWrapper()
{
    if(locker)
    {
        locker->tryLock(500);
        if(bev)
        {
            bufferevent_free(bev);
            bev = nullptr;
        }
        locker->unlock();
        delete locker;
        locker = nullptr;
    }
    else if(bev)
    {
        bufferevent_free(bev);
        bev = nullptr;
    }
}

BevWrapper &BevWrapper::operator =(BevWrapper &&b) noexcept
{
    if(this != &b)
    {
        bev = b.bev;
        locker = b.locker;
        b.bev = nullptr;
        b.locker = nullptr;
    }
    return *this;
}

int BevWrapper::freeBev()
{
    if(bev)
    {
        lockBev();
        bufferevent_free(bev);
        bev = nullptr;
        unlockBev();
        return 0;
    }
    else
        return -1;
}
