#ifndef PXMSTACKWIDGET_H
#define PXMSTACKWIDGET_H

#include <QTextBrowser>
#include <QStackedWidget>
#include <QUuid>
#include <QWidget>
#include <QLabel>
#include <QHash>
#include <QVector>
#include <QDebug>
#include <QLineEdit>
#include <QTimer>

namespace PXMMessageViewer {

  const QString infoTyping = " is typing...";
  const QString infoEntered = " has entered text.";
  const int typeTimerInterval = 1500;
  const QString infoBarStyle = "QLineEdit { background-color : rgb(255,255,128); color : black; }";
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
  QLineEdit* info;
  QTimer* typingTimer;
  bool textEntered = false;
  bool typing = false;
  void resizeEvent(QResizeEvent *event)
  {
    rlabel();
    QTextBrowser::resizeEvent(event);
  }

  void rlabel();

 public:
  explicit TextWidget(QWidget* parent, const QUuid& uuid)
      : QTextBrowser(parent), MVBase(uuid) {
    this->setOpenExternalLinks(true);
    this->setOpenLinks(true);
    this->setTextInteractionFlags(Qt::TextInteractionFlag::LinksAccessibleByMouse);
    info = new QLineEdit(this);
    info->setStyleSheet(infoBarStyle);
    info->setVisible(false);
    info->setReadOnly(true);
    info->setFocusPolicy(Qt::FocusPolicy::NoFocus);

    typingTimer = new QTimer(this);
    typingTimer->setInterval(typeTimerInterval);
    typingTimer->setSingleShot(true);
    QObject::connect(typingTimer, &QTimer::timeout, this, &TextWidget::timerCallback);
    //info->setFrameStyle(QFrame::StyledPanel | QFrame::Raised);
  }
  void showTyping(QString hostname);
  void showEntered(QString hostname);
  void clearInfoLine();
  void timerCallback();
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
  TextWidget *getItem(QUuid &uuid);
  int showTyping(QUuid &uuid, QString hostname);
  int showEntered(QUuid &uuid, QString hostname);
  int clearInfoLine(QUuid &uuid);
};

}

#endif  // PXMMESSAGEVIEWER_H
