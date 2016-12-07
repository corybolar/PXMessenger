#ifndef PXMTEXTBROWSER_H
#define PXMTEXTBROWSER_H

#include <QTextBrowser>
#include <QResizeEvent>
#include <QWidget>

class PXMTextBrowser : public QTextBrowser
{
    Q_OBJECT
public:
    PXMTextBrowser(QWidget *parent) : QTextBrowser(parent) {};
    void resizeEvent(QResizeEvent *event);
signals:
    void resizeLabel(QRect);
};

#endif // PXMTEXTBROWSER_H
