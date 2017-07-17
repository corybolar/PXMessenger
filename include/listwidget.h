#ifndef PXMLISTWIDGET_H
#define PXMLISTWIDGET_H

#include <QListWidgetItem>
#include <QListWidget>
#include <QString>
#include <QUuid>

#include <event2/event.h>

class PXMListWidgetItem : public QListWidgetItem
{
  QString hostname, ipAddress, textColor;
  QUuid uuid;
  sockaddr_in addr;
  bool isSSL, isConnected;

public:
  explicit PXMListWidgetItem(QListWidget *parent = nullptr);
  PXMListWidgetItem(QString hname, QUuid uuid, sockaddr_in addr, bool isSSL, bool isConnected, QListWidget *parent = nullptr);


  QString getHostname() const;
  void setHostname(const QString &value);

  QUuid getUuid() const;
  void setUuid(const QUuid &value);

  sockaddr_in getAddr() const;
  void setAddr(const sockaddr_in &value);

  bool getIsSSL() const;
  void setIsSSL(bool value);

  bool getIsConnected() const;
  void setIsConnected(bool value);

  void updatePopup();

  void setItalics();
  void removeItalics();
  void mirrorInfo(PXMListWidgetItem *lwi);
  QString getTextColor() const;
  void setTextColor(const QString &value);
};

class PXMListWidget : public QListWidget
{
  Q_OBJECT
  QUuid globalChatUUID, seperatorUUID;
  QColor alertColor;
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
  const int defaultAlertColor = 0xFFCD5C5C;
  unsigned int textColorsNext;
public:
  explicit PXMListWidget(QWidget *parent = nullptr);
  PXMListWidgetItem *itemAtUuid(const QUuid &uuid);
  void alertItem(const QUuid &uuid);
  void setAlertColor(const QColor &color);
  void unalertItem(const QUuid &uuid);
  void updateItem(PXMListWidgetItem *lwi);
  PXMListWidgetItem *currentItem();
  void setGlobalUUID(const QUuid &uuid);
signals:
  void append(QString str, QUuid& uuid);
  void newPeer(QUuid uuid);
};

#endif // PXMLISTWIDGET_H
