#pragma once
#ifndef PXMMESSAGEVIEWER_H
#define PXMMESSAGEVIEWER_H

#include <QTextBrowser>
#include <QStackedWidget>
#include <QUuid>

class QLabel;
namespace PXMMessageViewer {

class TextWidget : public QTextBrowser
{
    Q_OBJECT
    QUuid identifier;
public:
    TextWidget(QWidget *parent, const QUuid& uuid) :
        QTextBrowser(parent),
        identifier(uuid)
    {}
    QUuid getIdentifier() {return identifier;}
};

class StackedWidget : public QStackedWidget
{
    Q_OBJECT
    TextWidget *intro;
public:
    StackedWidget(QWidget *parent);
    int append(QString str, QUuid& uuid);
    int switchToUuid(QUuid& uuid);
};

}

#endif // PXMMESSAGEVIEWER_H
