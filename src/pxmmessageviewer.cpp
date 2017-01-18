#include "pxmmessageviewer.h"

using namespace PXMMessageViewer;

void StackedWidget::resizeEvent(QResizeEvent *event)
{
    emit resizeLabel(this->geometry());
    QStackedWidget::resizeEvent(event);
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
