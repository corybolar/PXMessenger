#ifndef PXMSTACKWIDGET_H
#define PXMSTACKWIDGET_H

#include <QTextBrowser>
#include <QStackedWidget>
#include <QUuid>
#include <QWidget>
#include <QLabel>
#include <QHash>
#include <QVector>

namespace PXMMessageViewer {
  class History {
    QUuid identifier;
    QString text;
  public:
    History() : identifier(QUuid()), text(QString()) {}
    History(QUuid id, QString text = QString()) : identifier(id), text(text) {}
    QString getText() const {return text;}
    QUuid getUuid() const {return identifier;}
    void append(const QString& str);
  };

// subclass this to allow the stackwidget to search for it by uuid
class MVBase {
  QUuid identifier;

 public:
  explicit MVBase(const QUuid& uuid) : identifier(uuid) {}
  QUuid getIdentifier() const { return identifier; }
};
class LabelWidget : public QLabel, public MVBase {
  Q_OBJECT
 public:
  explicit LabelWidget(QWidget* parent, const QUuid& uuid);
};

class TextWidget : public QTextBrowser, public MVBase {
  Q_OBJECT
 public:
  explicit TextWidget(QWidget* parent, const QUuid& uuid)
      : QTextBrowser(parent), MVBase(uuid) {}
};

class StackedWidget : public QStackedWidget {
  Q_OBJECT
  //QVector<History> history;
    QHash<QUuid, History> history;
    int removeLastWidget();
public:
  StackedWidget(QWidget* parent);
  int append(QString str, QUuid& uuid);
  int switchToUuid(QUuid& uuid);
  int newHistory(QUuid &uuid);
};
}

#endif  // PXMMESSAGEVIEWER_H
