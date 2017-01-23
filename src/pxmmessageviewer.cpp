#include "pxmmessageviewer.h"
#include <QFile>
#include <QLabel>
#include <QStringBuilder>

using namespace PXMMessageViewer;

StackedWidget::StackedWidget(QWidget* parent) : QStackedWidget(parent), intro(new TextWidget(this, QUuid::createUuid()))
{
    LabelWidget* lw = new LabelWidget(this, QUuid::createUuid());
    lw->setText("Select a friend on the right to begin chatting!");
    lw->setAlignment(Qt::AlignCenter);
    this->addWidget(lw);
}

int StackedWidget::append(QString str, QUuid& uuid)
{
    for (int i = 0; i < this->count(); i++) {
        MVBase* mvb = dynamic_cast<MVBase*>(this->widget(i));
        if (mvb) {
            if (mvb->getIdentifier() == uuid) {
                TextWidget* tw = qobject_cast<TextWidget*>(this->widget(i));
                if (tw) {
                    tw->append(str);
                    return 0;
                } else {
                    return -1;
                }
            }
        }
    }
    return -1;
}
int StackedWidget::switchToUuid(QUuid& uuid)
{
    for (int i = 0; i < this->count(); i++) {
        MVBase* mvb = dynamic_cast<MVBase*>(this->widget(i));
        if (mvb->getIdentifier() == uuid) {
            this->setCurrentIndex(i);
            return 0;
        }
    }
    return -1;
}
