#ifndef PXMMESSAGEVIEWER_H
#define PXMMESSAGEVIEWER_H

#include <QTextBrowser>
#include <QStackedWidget>
#include <QUuid>
#include <QWidget>
#include <QLabel>

namespace PXMMessageViewer {
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
  LabelWidget(QWidget* parent, const QUuid& uuid)
      : QLabel(parent), MVBase(uuid) {
    this->setBackgroundRole(QPalette::Base);
    this->setAutoFillBackground(true);
    this->setFrameStyle(QFrame::StyledPanel | QFrame::Raised);
  }
};

class TextWidget : public QTextBrowser, public MVBase {
  Q_OBJECT
 public:
  TextWidget(QWidget* parent, const QUuid& uuid)
      : QTextBrowser(parent), MVBase(uuid) {}
};

class StackedWidget : public QStackedWidget {
  Q_OBJECT
  TextWidget* intro;

 public:
  StackedWidget(QWidget* parent);
  int append(QString str, QUuid& uuid);
  int switchToUuid(QUuid& uuid);
};
}

#endif  // PXMMESSAGEVIEWER_H
