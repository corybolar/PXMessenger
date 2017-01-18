#include "pxmmessageviewer.h"
#include <QLabel>

using namespace PXMMessageViewer;

StackedWidget::StackedWidget(QWidget *parent) : QStackedWidget(parent),
    intro(new TextWidget(this, QUuid::createUuid()))
{
    intro->setText("Select a friend on the right to begin messaging!");
    this->addWidget(intro);
}

int StackedWidget::append(QString str, QUuid &uuid)
{
    for(int i = 0; i < this->count(); i++)
    {
        TextWidget *tw = qobject_cast<TextWidget*>(this->widget(i));
        if(tw->getIdentifier() == uuid)
        {
            tw->append(str);
            return 0;
        }
    }
    return -1;
}
int StackedWidget::switchToUuid(QUuid &uuid)
{
    for(int i = 0; i < this->count(); i++)
    {
        TextWidget *tw = qobject_cast<TextWidget*>(this->widget(i));
        if(tw->getIdentifier() == uuid)
        {
            this->setCurrentIndex(i);
            return 0;
        }
    }
    return -1;
}
