#include "pxmstackwidget.h"
#include <QFile>
#include <QLabel>
#include <QStringBuilder>

using namespace PXMMessageViewer;

StackedWidget::StackedWidget(QWidget* parent) : QStackedWidget(parent)
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
        if (mvb && mvb->getIdentifier() == uuid) {
            TextWidget* tw = qobject_cast<TextWidget*>(this->widget(i));
            if (tw) {
                tw->append(str);
                return 0;
            } else {
                return -1;
            }
        }
    }
    // not in stackwidget
    for (int i = 0; i < history.count(); i++) {
        if (history.value(i).getUuid() == uuid) {
            history.value(i).append(str);
            return 0;
        }
    }
    return -1;
}
int StackedWidget::switchToUuid(QUuid& uuid)
{
    for (int i = 0; i < this->count(); i++) {
        MVBase* mvb = dynamic_cast<MVBase*>(this->widget(i));
        if (mvb->getIdentifier() == uuid) {
            QWidget* qw = this->widget(i);
            this->removeWidget(qw);
            this->insertWidget(0, qw);
            this->setCurrentIndex(0);
            return 0;
        }
    }
    // isnt in the list
    auto i = history.begin();
    while (i != history.end()) {
        if (i->getUuid() == uuid) {
            TextWidget* tw = new TextWidget(this, uuid);
            tw->setHtml(i->getText());
            this->insertWidget(0, tw);
            this->setCurrentIndex(0);
            QWidget* qw     = this->widget(this->count() - 1);
            TextWidget* old = qobject_cast<TextWidget*>(qw);
            History oldHist(old->getIdentifier(), old->toHtml());
            history.append(oldHist);
            this->removeWidget(this->widget(this->count() - 1));
            history.erase(i);
            return 0;
        } else
            i++;
    }
    return -1;
}

LabelWidget::LabelWidget(QWidget* parent, const QUuid& uuid) : QLabel(parent), MVBase(uuid)
{
    this->setBackgroundRole(QPalette::Base);
    this->setAutoFillBackground(true);
    this->setFrameStyle(QFrame::StyledPanel | QFrame::Raised);
}

void History::append(const QString& str)
{
    text.append(str);
}
