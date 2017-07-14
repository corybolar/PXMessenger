#include "peers.h"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
Q_DECLARE_METATYPE(intptr_t)
#else
#include <arpa/inet.h>
#endif

#include <QMutex>
#include <QStringBuilder>
#include <QDateTime>

#include <event2/bufferevent.h>

using namespace Peers;

// int Peers::textColorsNext = 0;

PeerData::PeerData()
    : uuid(QUuid()),
      addrRaw(sockaddr_in()),
      hostname(QString()),
      progVersion(QString()),
      bw(QSharedPointer<BevWrapper>(new BevWrapper)),
      socket(-1),
      timeOfTyping(QDateTime::currentMSecsSinceEpoch()),
      connectTo(false),
      isAuthed(false)
{
    //   textColor = textColors.at(textColorsNext % textColors.length());
    //   textColorsNext++;
}

PeerData::PeerData(const PeerData& pd)
    : uuid(pd.uuid),
      addrRaw(pd.addrRaw),
      hostname(pd.hostname),
      // textColor(pd.textColor),
      progVersion(pd.progVersion),
      bw(pd.bw),
      socket(pd.socket),
      timeOfTyping(pd.timeOfTyping),
      connectTo(pd.connectTo),
      isAuthed(pd.isAuthed)
{
}

PeerData::PeerData(PeerData&& pd) noexcept
    : uuid(pd.uuid),
      addrRaw(pd.addrRaw),
      hostname(pd.hostname),
      // textColor(pd.textColor),
      progVersion(pd.progVersion),
      bw(pd.bw),
      socket(pd.socket),
      timeOfTyping(pd.timeOfTyping),
      connectTo(pd.connectTo),
      isAuthed(pd.isAuthed)
{
    pd.bw.clear();
}

PeerData& PeerData::operator=(PeerData&& p) noexcept
{
    if (this != &p) {
        bw = p.bw;
        p.bw.clear();
        uuid     = p.uuid;
        addrRaw  = p.addrRaw;
        hostname = p.hostname;
        // textColor   = p.textColor;
        progVersion  = p.progVersion;
        socket       = p.socket;
        timeOfTyping = p.timeOfTyping;
        connectTo    = p.connectTo;
        isAuthed     = p.isAuthed;
    }
    return *this;
}

PeerData& PeerData::operator=(const PeerData& p)
{
    Peers::PeerData temp(p);
    *this = std::move(temp);
    return *this;
}

QString PeerData::toInfoString()
{
    return QString(
        QStringLiteral("Hostname: ") % hostname % QStringLiteral("\nUUID: ") % uuid.toString() %
        QStringLiteral("\nSSL connection: ") % QString::fromLocal8Bit((bw->isSSL ? "true" : "false")) %
        QStringLiteral("\nProgram Version: ") % progVersion % QStringLiteral("\nIP Address: ") %
        QString::fromLocal8Bit(inet_ntoa(addrRaw.sin_addr)) % QStringLiteral(":") %
        QString::number(ntohs(addrRaw.sin_port)) % QStringLiteral("\nIsAuthenticated: ") %
        QString::fromLocal8Bit((isAuthed ? "true" : "false")) % QStringLiteral("\npreventAttemptConnection: ") %
        QString::fromLocal8Bit((connectTo ? "true" : "false")) % QStringLiteral("\nSocketDescriptor: ") %
        QString::number(socket) % QStringLiteral("\nBufferevent: ") %
        (bw->getBev() ? QString::asprintf("%8p", static_cast<void*>(bw->getBev())) : QStringLiteral("NULL")) %
        QStringLiteral("\n"));
}

BevWrapper::BevWrapper() : bev(nullptr), locker(new QMutex)
{
}

BevWrapper::BevWrapper(bufferevent* buf) : bev(buf), locker(new QMutex)
{
}
BevWrapper::BevWrapper(bufferevent* buf, bool ssl) : bev(buf), locker(new QMutex), isSSL(ssl)
{
}
BevWrapper::~BevWrapper()
{
    if (locker) {
        locker->tryLock(500);
        if (bev) {
            bufferevent_free(bev);
            bev = nullptr;
        }
        locker->unlock();
        delete locker;
        locker = nullptr;
    } else if (bev) {
        bufferevent_free(bev);
        bev = nullptr;
    }
}

BevWrapper::BevWrapper(BevWrapper&& b) noexcept : bev(b.bev), locker(b.locker), isSSL(b.isSSL)
{
    b.bev    = nullptr;
    b.locker = nullptr;
    b.isSSL  = false;
}

BevWrapper& BevWrapper::operator=(BevWrapper&& b) noexcept
{
    if (this != &b) {
        bev      = b.bev;
        locker   = b.locker;
        isSSL    = b.isSSL;
        b.bev    = nullptr;
        b.locker = nullptr;
        b.isSSL  = false;
    }
    return *this;
}

void BevWrapper::lockBev()
{
    locker->lock();
}

void BevWrapper::unlockBev()
{
    locker->unlock();
}

int BevWrapper::freeBev()
{
    if (bev) {
        lockBev();
        bufferevent_free(bev);
        bev = nullptr;
        unlockBev();
        return 0;
    } else {
        return -1;
    }
}
