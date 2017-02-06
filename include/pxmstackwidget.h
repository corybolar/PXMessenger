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
    History() {}
    History(QUuid id, QString text) : identifier(id), text(text) {}
    QString getText() {return text;};
    QUuid getUuid() {return identifier;}
    void append(const QString& str);
  };

// subclass this to allow the stackwidget to search for it by uuid
class MVBase {
  QUuid identifier;

 public:
  MVBase(const QUuid& uuid) : identifier(uuid) {}
  QUuid getIdentifier() const { return identifier; }
};
class LabelWidget : public QLabel, public MVBase {
  Q_OBJECT
 public:
  LabelWidget(QWidget* parent, const QUuid& uuid);
};

class TextWidget : public QTextBrowser, public MVBase {
  Q_OBJECT
 public:
  TextWidget(QWidget* parent, const QUuid& uuid)
      : QTextBrowser(parent), MVBase(uuid) {}
};

class StackedWidget : public QStackedWidget {
  Q_OBJECT
  QVector<History> history;
 public:
  StackedWidget(QWidget* parent);
  int append(QString str, QUuid& uuid);
  int switchToUuid(QUuid& uuid);
};
}

#endif  // PXMMESSAGEVIEWER_H
