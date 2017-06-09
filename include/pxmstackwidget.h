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
  const QString infoBarStyle = "QLineEdit { background-color : rgb(255,255,160); color : black; }";
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
  //QTimer* typingTimer;
  bool typing;
  void resizeEvent(QResizeEvent *event)
  {
    rlabel();
    QTextBrowser::resizeEvent(event);
  }

  void rlabel();

 public:
  qint64 typingTime;
  QLineEdit* info;
  bool textEntered;
  explicit TextWidget(QWidget* parent, const QUuid& uuid);
  //Copy
  TextWidget (const TextWidget& other) :
    MVBase(other.getIdentifier()),
    //typingTimer(new QTimer(other.typingTimer)),
    typing(other.typing),
    info(new QLineEdit(other.info)),
    textEntered(other.textEntered)
  {

  }
  //Move
  TextWidget (TextWidget&& other) noexcept :
    MVBase(other.getIdentifier()),
    //typingTimer(other.typingTimer),
    typing(other.typing),
    info(other.info),
    textEntered(other.textEntered)
  {
    other.info = nullptr;
    //other.typingTimer = nullptr;
  }
  ~TextWidget() noexcept
  {
    //typingTimer->stop();
  }
  //Copy Assignment
  TextWidget& operator= (const TextWidget& other)
  {
    TextWidget tmp(other);
    *this = std::move(tmp);
    return *this;
  }
  //Move Assignment
  TextWidget& operator= (TextWidget&& other) noexcept
  {
    //typingTimer->deleteLater();
    info->deleteLater();
    //typingTimer = other.typingTimer;
    info = other.info;
    //other.typingTimer = nullptr;
    other.info = nullptr;
    textEntered = other.textEntered;
    typing = other.typing;
    return *this;
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
    QTimer *typingTimer;
public:
  StackedWidget(QWidget* parent);
  int append(QString str, QUuid& uuid);
  int switchToUuid(QUuid& uuid);
  int newHistory(QUuid &uuid);
  TextWidget *getItem(QUuid &uuid);
  int showTyping(QUuid &uuid, QString hostname);
  int showEntered(QUuid &uuid, QString hostname);
  int clearInfoLine(QUuid &uuid);
private slots:
  void timerCallback();
};

}

#endif  // PXMMESSAGEVIEWER_H
