#ifndef PXMMESSAGEVIEWER_H
#define PXMMESSAGEVIEWER_H

#include <QTextBrowser>
#include <QStackedWidget>
#include <QUuid>

namespace PXMMessageViewer {

class TextWidget : public QTextBrowser
{
    Q_OBJECT
    QUuid identifier;
public:
    TextWidget(QWidget *parent) : QTextBrowser(parent) {}
    TextWidget(QWidget *parent, const QUuid& uuid) :
        QTextBrowser(parent),
        identifier(uuid)
    {}
    QUuid getIdentifier() {return identifier;}
};

class StackedWidget : public QStackedWidget
{
    Q_OBJECT
public:
    StackedWidget(QWidget *parent) : QStackedWidget(parent) {}
    int append(QString str, QUuid& uuid);
    int switchToUuid(QUuid& uuid);
protected:
    void resizeEvent(QResizeEvent *event);
signals:
    void resizeLabel(QRect);
};

}

#endif // PXMMESSAGEVIEWER_H
