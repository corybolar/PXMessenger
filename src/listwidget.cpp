#include "listwidget.h"
#include <arpa/inet.h>
#include <QStringBuilder>
#include <QGuiApplication>
#include <QFrame>

PXMListWidget::PXMListWidget(QWidget* parent) : QListWidget(parent)
{
    alertColor     = QColor(PXMListWidget::defaultAlertColor);
    seperatorUUID  = QUuid::createUuid();
    globalChatUUID = QUuid::createUuid();

    PXMListWidgetItem* global = new PXMListWidgetItem("Global Chat", globalChatUUID, sockaddr_in(), false, false, this);
    this->insertItem(0, global);
    PXMListWidgetItem* seperator = new PXMListWidgetItem("", seperatorUUID, sockaddr_in(), false, false, this);
    seperator->setSizeHint(QSize(200, 10));
    seperator->setFlags(Qt::NoItemFlags);
    this->insertItem(1, seperator);
    QFrame* fsep = new QFrame(this);
    fsep->setFrameStyle(QFrame::HLine | QFrame::Plain);
    fsep->setLineWidth(2);
    this->setItemWidget(seperator, fsep);

    textColorsNext = rand() % textColors.length();
}

void PXMListWidget::setGlobalUUID(const QUuid& uuid)
{
    itemAtUuid(globalChatUUID)->setUuid(uuid);
    globalChatUUID = uuid;
    emit newPeer(globalChatUUID);
}

PXMListWidgetItem* PXMListWidget::itemAtUuid(const QUuid& uuid)
{
    for (int i = 0; i < this->count(); i++) {
        PXMListWidgetItem* lwi = dynamic_cast<PXMListWidgetItem*>(this->item(i));
        if (!lwi) {
            continue;
        }
        if (lwi->getUuid() == uuid) {
            return lwi;
        }
    }
    return nullptr;
}

void PXMListWidget::alertItem(const QUuid& uuid)
{
    PXMListWidgetItem* pxi = itemAtUuid(uuid);
    QFont font             = pxi->font();
    font.setBold(true);
    font.setWeight(QFont::Weight::ExtraBold);
    pxi->setFont(font);
    pxi->setBackgroundColor(alertColor);
}
void PXMListWidget::unalertItem(const QUuid& uuid)
{
    PXMListWidgetItem* pxi = itemAtUuid(uuid);
    QFont font             = pxi->font();
    font.setBold(false);
    font.setWeight(QFont::Weight::Normal);
    pxi->setFont(font);
    pxi->setBackgroundColor(QGuiApplication::palette().base().color());
}

void PXMListWidget::setAlertColor(const QColor& color)
{
    QColor old = alertColor;
    alertColor = color;
    for (int i = 0; i < this->count(); i++) {
        if (this->item(i)->backgroundColor() == old)
            this->item(i)->setBackgroundColor(alertColor);
    }
}

void PXMListWidget::updateItem(PXMListWidgetItem* lwi)
{
    this->setUpdatesEnabled(false);
    PXMListWidgetItem* pxi = itemAtUuid(lwi->getUuid());

    PXMListWidgetItem* global = dynamic_cast<PXMListWidgetItem*>(this->takeItem(0));
    if (pxi) {
        if (pxi->getHostname() != lwi->getHostname()) {
            QUuid uuid = lwi->getUuid();
            emit append(pxi->getHostname() + " has changed their name to " + lwi->getHostname(), uuid);
        }
        pxi->mirrorInfo(lwi);
    } else {
        lwi->setTextColor(textColors.at(textColorsNext % textColors.length()));
        textColorsNext++;
        this->insertItem(0, lwi);
        emit newPeer(lwi->getUuid());
    }

    this->sortItems();
    this->insertItem(0, global);
    this->setUpdatesEnabled(true);
}

PXMListWidgetItem* PXMListWidget::currentItem()
{
    return dynamic_cast<PXMListWidgetItem*>(this->QListWidget::currentItem());
}

QString PXMListWidgetItem::getTextColor() const
{
    return textColor;
}

void PXMListWidgetItem::setTextColor(const QString& value)
{
    textColor = value;
}

PXMListWidgetItem::PXMListWidgetItem(QListWidget* parent) : QListWidgetItem(parent)
{
}

PXMListWidgetItem::PXMListWidgetItem(QString hname,
                                     QUuid uuid,
                                     sockaddr_in addr,
                                     bool isSSL,
                                     bool isConnected,
                                     QListWidget* parent)
    : QListWidgetItem(parent), hostname(hname), uuid(uuid), addr(addr), isSSL(isSSL), isConnected(isConnected)
{
    this->setText(hostname);
}
void PXMListWidgetItem::updatePopup()
{
    this->setStatusTip(hostname % "\n" % uuid.toString() % "\n" % "Listener: " % ipAddress % "\n" %
                       (isConnected ? "Connected" : "Disconnected") % (isSSL ? "Secure" : "Insecure"));
}

void PXMListWidgetItem::mirrorInfo(PXMListWidgetItem* lwi)
{
    hostname    = lwi->getHostname();
    addr        = lwi->getAddr();
    uuid        = lwi->getUuid();
    isConnected = lwi->getIsConnected();
    isSSL       = lwi->getIsSSL();
    textColor   = lwi->getTextColor();
}

void PXMListWidgetItem::setItalics()
{
    QFont f = this->font();
    f.setItalic(true);
    this->setFont(f);
}

void PXMListWidgetItem::removeItalics()
{
    QFont f = this->font();
    f.setItalic(false);
    this->setFont(f);
}
QString PXMListWidgetItem::getHostname() const
{
    return hostname;
}

void PXMListWidgetItem::setHostname(const QString& value)
{
    hostname = value;
    this->setText(hostname);
    updatePopup();
}

QUuid PXMListWidgetItem::getUuid() const
{
    return uuid;
}

void PXMListWidgetItem::setUuid(const QUuid& value)
{
    uuid = value;
    updatePopup();
}

sockaddr_in PXMListWidgetItem::getAddr() const
{
    return addr;
}

void PXMListWidgetItem::setAddr(const sockaddr_in& value)
{
    addr = value;
    ipAddress =
        QString::fromLocal8Bit(inet_ntoa(addr.sin_addr)) % QStringLiteral(":") % QString::number(ntohs(addr.sin_port));
    updatePopup();
}

bool PXMListWidgetItem::getIsSSL() const
{
    return isSSL;
}

void PXMListWidgetItem::setIsSSL(bool value)
{
    isSSL = value;
    updatePopup();
}

bool PXMListWidgetItem::getIsConnected() const
{
    return isConnected;
}

void PXMListWidgetItem::setIsConnected(bool value)
{
    isConnected = value;
    updatePopup();
}
