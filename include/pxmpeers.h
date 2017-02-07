#ifndef PXMPEERS_H
#define PXMPEERS_H

#include <QUuid>
#include <QLinkedList>
#include <QVector>
#include <QSharedPointer>
#include <QString>

#include <event2/util.h>
struct bufferevent;

Q_DECLARE_METATYPE(struct sockaddr_in)
Q_DECLARE_METATYPE(size_t)
Q_DECLARE_OPAQUE_POINTER(bufferevent*)
Q_DECLARE_METATYPE(bufferevent*)
Q_DECLARE_OPAQUE_POINTER(const bufferevent*)
Q_DECLARE_METATYPE(const bufferevent*)

class QMutex;
namespace Peers {
const QString selfColor = "#6495ED";  // Cornflower Blue
const QString peerColor = "#FF0000";  // Red
const QVector<QString> textColors = {
    "#808000",  // Olive
    "#FFA500",  // Orange
    "#FF00FF",  // Fuschia
    "#DC143C",  // Crimson
    "#FF69B4",  // HotPink
    "#708090",  // SlateGrey
    "#008000",  // Green
    "#00FF00"   // Lime
};
extern int textColorsNext;
class BevWrapper {
  bufferevent* bev;
  QMutex* locker;

 public:
  // Default Constructor
  BevWrapper();
  // Constructor with a bufferevent
  BevWrapper(bufferevent* buf);
  // Destructor
  ~BevWrapper();
  // Copy Constructor
  BevWrapper(const BevWrapper& b) : bev(b.bev), locker(b.locker) {}
  // Move Constructor
  BevWrapper(BevWrapper&& b) noexcept;
  // Move Assignment
  BevWrapper& operator=(BevWrapper&& b) noexcept;
  // Eqaul
  bool operator==(const BevWrapper& b) { return bev == b.bev; }
  // Not Equal
  bool operator!=(const BevWrapper& b) { return !(bev == b.bev); }

  void setBev(bufferevent* buf) { bev = buf; }
  bufferevent* getBev() const { return bev; }
  void lockBev();
  void unlockBev();
  int freeBev();
};

class PeerData {
 public:
  QUuid uuid;
  struct sockaddr_in addrRaw;
  QString hostname;
  QString textColor;
  QString progVersion;
  QSharedPointer<BevWrapper> bw;
  evutil_socket_t socket;
  bool connectTo;
  bool isAuthed;

  // Default Constructor
  PeerData();

  // Copy
  PeerData(const PeerData& pd);

  // Move
  PeerData(PeerData&& pd) noexcept;

  // Destructor
  ~PeerData() noexcept {}

  // Move assignment
  PeerData& operator=(PeerData&& pd) noexcept;

  // Copy assignment
  PeerData& operator=(const PeerData& pd);

  // Return data of this struct as a string padded with the value in 'pad'
  QString toInfoString();
};
}
Q_DECLARE_METATYPE(QSharedPointer<Peers::BevWrapper>)

#endif
